/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 * Copyright (C) 2008, 2010 Collabora Ltd.
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2009, 2013 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2009, 2010, 2011, 2012 Igalia S.L.
 * Copyright (C) 2009 John Kjellberg <john.kjellberg@power.alstom.com>
 * Copyright (C) 2012 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ResourceHandle.h"

#if USE(SOUP)

#include "CredentialStorage.h"
#include "FileSystem.h"
#include "GUniquePtrSoup.h"
#include "HTTPParsers.h"
#include "LocalizedStrings.h"
#include "MIMETypeRegistry.h"
#include "NetworkStorageSession.h"
#include "NetworkingContext.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "SoupNetworkSession.h"
#include "TextEncoding.h"
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <sys/stat.h>
#include <sys/types.h>
#if !COMPILER(MSVC)
#include <unistd.h>
#endif
#include <wtf/CompletionHandler.h>
#include <wtf/CurrentTime.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/RunLoopSourcePriority.h>
#include <wtf/text/CString.h>

namespace WebCore {

static const size_t gDefaultReadBufferSize = 8192;

static bool createSoupRequestAndMessageForHandle(ResourceHandle*, const ResourceRequest&);
static void cleanupSoupRequestOperation(ResourceHandle*, bool isDestroying = false);
static void sendRequestCallback(GObject*, GAsyncResult*, gpointer);
static void readCallback(GObject*, GAsyncResult*, gpointer);
static void continueAfterDidReceiveResponse(ResourceHandle*);

ResourceHandleInternal::~ResourceHandleInternal() = default;

static SoupSession* sessionFromContext(NetworkingContext* context)
{
    if (!context || !context->isValid())
        return NetworkStorageSession::defaultStorageSession().getOrCreateSoupNetworkSession().soupSession();
    return context->storageSession().getOrCreateSoupNetworkSession().soupSession();
}

ResourceHandle::~ResourceHandle()
{
    cleanupSoupRequestOperation(this, true);
}

SoupSession* ResourceHandleInternal::soupSession()
{
    return m_session ? m_session->soupSession() : sessionFromContext(m_context.get());
}

RefPtr<ResourceHandle> ResourceHandle::create(SoupNetworkSession& session, const ResourceRequest& request, ResourceHandleClient* client, bool defersLoading, bool shouldContentSniff, bool shouldContentEncodingSniff)
{
    auto newHandle = adoptRef(*new ResourceHandle(session, request, client, defersLoading, shouldContentSniff, shouldContentEncodingSniff));

    if (newHandle->d->m_scheduledFailureType != NoFailure)
        return WTFMove(newHandle);

    if (newHandle->start())
        return WTFMove(newHandle);

    return nullptr;
}

ResourceHandle::ResourceHandle(SoupNetworkSession& session, const ResourceRequest& request, ResourceHandleClient* client, bool defersLoading, bool shouldContentSniff, bool shouldContentEncodingSniff)
    : d(std::make_unique<ResourceHandleInternal>(this, nullptr, request, client, defersLoading, shouldContentSniff && shouldContentSniffURL(request.url()), shouldContentEncodingSniff))
{
    if (!request.url().isValid()) {
        scheduleFailure(InvalidURLFailure);
        return;
    }

    if (!portAllowed(request.url())) {
        scheduleFailure(BlockedFailure);
        return;
    }

    d->m_session = &session;
}

bool ResourceHandle::cancelledOrClientless()
{
    if (!client())
        return true;

    return getInternal()->m_cancelled;
}

void ResourceHandle::ensureReadBuffer()
{
    ResourceHandleInternal* d = getInternal();

    if (d->m_soupBuffer)
        return;


    auto* buffer = static_cast<uint8_t*>(fastMalloc(gDefaultReadBufferSize));
    d->m_soupBuffer.reset(soup_buffer_new_with_owner(buffer, gDefaultReadBufferSize, buffer, fastFree));

    ASSERT(d->m_soupBuffer);
}

static bool isAuthenticationFailureStatusCode(int httpStatusCode)
{
    return httpStatusCode == SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED || httpStatusCode == SOUP_STATUS_UNAUTHORIZED;
}

static gboolean tlsConnectionAcceptCertificateCallback(GTlsConnection* connection, GTlsCertificate* certificate, GTlsCertificateFlags errors, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    if (!handle || handle->cancelledOrClientless())
        return FALSE;

    ResourceHandleInternal* d = handle->getInternal();

    auto* connectionMessage = g_object_get_data(G_OBJECT(connection), "wk-soup-message");
    if (connectionMessage != d->m_soupMessage.get())
        return FALSE;

    URL url(soup_request_get_uri(d->m_soupRequest.get()));
    auto error = SoupNetworkSession::checkTLSErrors(url, certificate, errors);
    if (!error)
        return TRUE;

    handle->client()->didFail(handle.get(), error.value());
    handle->cancel();
    return FALSE;
}

static void gotHeadersCallback(SoupMessage* message, gpointer data)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle || handle->cancelledOrClientless())
        return;

    ResourceHandleInternal* d = handle->getInternal();

    if (d->m_context && d->m_context->isValid()) {
        // We are a bit more conservative with the persistent credential storage than the session store,
        // since we are waiting until we know that this authentication succeeded before actually storing.
        // This is because we want to avoid hitting the disk twice (once to add and once to remove) for
        // incorrect credentials or polluting the keychain with invalid credentials.
        if (!isAuthenticationFailureStatusCode(message->status_code) && message->status_code < 500) {
            d->m_context->storageSession().saveCredentialToPersistentStorage(
                d->m_credentialDataToSaveInPersistentStore.protectionSpace,
                d->m_credentialDataToSaveInPersistentStore.credential);
        }
    }

    // The original response will be needed later to feed to willSendRequest in
    // doRedirect() in case we are redirected. For this reason, we store it here.
    d->m_response.updateFromSoupMessage(message);
}

static void applyAuthenticationToRequest(ResourceHandle* handle, ResourceRequest& request, bool redirect)
{
    // m_user/m_pass are credentials given manually, for instance, by the arguments passed to XMLHttpRequest.open().
    ResourceHandleInternal* d = handle->getInternal();

    String partition = request.cachePartition();

    if (handle->shouldUseCredentialStorage()) {
        if (d->m_user.isEmpty() && d->m_pass.isEmpty())
            d->m_initialCredential = CredentialStorage::defaultCredentialStorage().get(partition, request.url());
        else if (!redirect) {
            // If there is already a protection space known for the URL, update stored credentials
            // before sending a request. This makes it possible to implement logout by sending an
            // XMLHttpRequest with known incorrect credentials, and aborting it immediately (so that
            // an authentication dialog doesn't pop up).
            CredentialStorage::defaultCredentialStorage().set(partition, Credential(d->m_user, d->m_pass, CredentialPersistenceNone), request.url());
        }
    }

    String user = d->m_user;
    String password = d->m_pass;
    if (!d->m_initialCredential.isEmpty()) {
        user = d->m_initialCredential.user();
        password = d->m_initialCredential.password();
    }

    if (user.isEmpty() && password.isEmpty()) {
        // In case credential is not available from the handle and credential storage should not to be used,
        // disable authentication manager so that credentials stored in libsoup are not used.
        d->m_useAuthenticationManager = handle->shouldUseCredentialStorage();
        return;
    }

    // We always put the credentials into the URL. In the CFNetwork-port HTTP family credentials are applied in
    // the didReceiveAuthenticationChallenge callback, but libsoup requires us to use this method to override
    // any previously remembered credentials. It has its own per-session credential storage.
    URL urlWithCredentials(request.url());
    urlWithCredentials.setUser(user);
    urlWithCredentials.setPass(password);
    request.setURL(urlWithCredentials);
}

// Called each time the message is going to be sent again except the first time.
// This happens when libsoup handles HTTP authentication.
static void restartedCallback(SoupMessage*, gpointer data)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle || handle->cancelledOrClientless())
        return;

    handle->m_requestTime = MonotonicTime::now();
}

static bool shouldRedirect(ResourceHandle* handle)
{
    ResourceHandleInternal* d = handle->getInternal();
    SoupMessage* message = d->m_soupMessage.get();

    // Some 3xx status codes aren't actually redirects.
    if (message->status_code == 300 || message->status_code == 304 || message->status_code == 305 || message->status_code == 306)
        return false;

    if (!soup_message_headers_get_one(message->response_headers, "Location"))
        return false;

    return true;
}

static bool shouldRedirectAsGET(SoupMessage* message, URL& newURL, bool crossOrigin)
{
    if (message->method == SOUP_METHOD_GET || message->method == SOUP_METHOD_HEAD)
        return false;

    if (!newURL.protocolIsInHTTPFamily())
        return true;

    switch (message->status_code) {
    case SOUP_STATUS_SEE_OTHER:
        return true;
    case SOUP_STATUS_FOUND:
    case SOUP_STATUS_MOVED_PERMANENTLY:
        if (message->method == SOUP_METHOD_POST)
            return true;
        break;
    }

    if (crossOrigin && message->method == SOUP_METHOD_DELETE)
        return true;

    return false;
}

static void continueAfterWillSendRequest(ResourceHandle* handle, ResourceRequest&& request)
{
    // willSendRequest might cancel the load.
    if (handle->cancelledOrClientless())
        return;

    ResourceHandleInternal* d = handle->getInternal();
    if (protocolHostAndPortAreEqual(request.url(), d->m_response.url()))
        applyAuthenticationToRequest(handle, request, true);

    if (!createSoupRequestAndMessageForHandle(handle, request)) {
        d->client()->cannotShowURL(handle);
        return;
    }

    handle->sendPendingRequest();
}

static void doRedirect(ResourceHandle* handle)
{
    ResourceHandleInternal* d = handle->getInternal();
    static const int maxRedirects = 20;

    if (d->m_redirectCount++ > maxRedirects) {
        d->client()->didFail(handle, ResourceError::transportError(d->m_soupRequest.get(), SOUP_STATUS_TOO_MANY_REDIRECTS, "Too many redirects"));
        cleanupSoupRequestOperation(handle);
        return;
    }

    ResourceRequest newRequest = handle->firstRequest();
    SoupMessage* message = d->m_soupMessage.get();
    const char* location = soup_message_headers_get_one(message->response_headers, "Location");
    URL newURL = URL(URL(soup_message_get_uri(message)), location);
    bool crossOrigin = !protocolHostAndPortAreEqual(handle->firstRequest().url(), newURL);
    newRequest.setURL(newURL);

    if (newRequest.httpMethod() != "GET") {
        // Change newRequest method to GET if change was made during a previous redirection
        // or if current redirection says so
        if (message->method == SOUP_METHOD_GET || shouldRedirectAsGET(message, newURL, crossOrigin)) {
            newRequest.setHTTPMethod("GET");
            newRequest.setHTTPBody(nullptr);
            newRequest.clearHTTPContentType();
        }
    }

    // Should not set Referer after a redirect from a secure resource to non-secure one.
    if (!newURL.protocolIs("https") && protocolIs(newRequest.httpReferrer(), "https") && handle->context()->shouldClearReferrerOnHTTPSToHTTPRedirect())
        newRequest.clearHTTPReferrer();

    d->m_user = newURL.user();
    d->m_pass = newURL.pass();
    newRequest.removeCredentials();

    if (crossOrigin) {
        // If the network layer carries over authentication headers from the original request
        // in a cross-origin redirect, we want to clear those headers here.
        newRequest.clearHTTPAuthorization();
        newRequest.clearHTTPOrigin();

        // TODO: We are losing any username and password specified in the redirect URL, as this is the
        // same behavior as the CFNet port. We should investigate if this is really what we want.
    }

    cleanupSoupRequestOperation(handle);

    ResourceResponse responseCopy = d->m_response;
    d->client()->willSendRequestAsync(handle, WTFMove(newRequest), WTFMove(responseCopy), [handle = makeRef(*handle)] (ResourceRequest&& request) {
        continueAfterWillSendRequest(handle.ptr(), WTFMove(request));
    });
}

static void redirectSkipCallback(GObject*, GAsyncResult* asyncResult, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    if (handle->cancelledOrClientless()) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    GUniqueOutPtr<GError> error;
    ResourceHandleInternal* d = handle->getInternal();
    gssize bytesSkipped = g_input_stream_skip_finish(d->m_inputStream.get(), asyncResult, &error.outPtr());
    if (error) {
        handle->client()->didFail(handle.get(), ResourceError::genericGError(error.get(), d->m_soupRequest.get()));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (bytesSkipped > 0) {
        g_input_stream_skip_async(d->m_inputStream.get(), gDefaultReadBufferSize, RunLoopSourcePriority::AsyncIONetwork,
            d->m_cancellable.get(), redirectSkipCallback, handle.get());
        return;
    }

    g_input_stream_close(d->m_inputStream.get(), 0, 0);
    doRedirect(handle.get());
}

static void wroteBodyDataCallback(SoupMessage*, SoupBuffer* buffer, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;

    ASSERT(buffer);
    ResourceHandleInternal* d = handle->getInternal();
    d->m_bodyDataSent += buffer->length;

    if (handle->cancelledOrClientless())
        return;

    handle->client()->didSendData(handle.get(), d->m_bodyDataSent, d->m_bodySize);
}

static void cleanupSoupRequestOperation(ResourceHandle* handle, bool isDestroying)
{
    ResourceHandleInternal* d = handle->getInternal();

    d->m_soupRequest.clear();
    d->m_inputStream.clear();
    d->m_multipartInputStream.clear();
    d->m_cancellable.clear();
    d->m_soupBuffer.reset();

    if (d->m_soupMessage) {
        g_signal_handlers_disconnect_matched(d->m_soupMessage.get(), G_SIGNAL_MATCH_DATA,
                                             0, 0, 0, 0, handle);
        g_object_set_data(G_OBJECT(d->m_soupMessage.get()), "handle", 0);
        d->m_soupMessage.clear();
    }

    d->m_timeoutSource.stop();

    if (!isDestroying)
        handle->deref();
}

size_t ResourceHandle::currentStreamPosition() const
{
    GInputStream* baseStream = d->m_inputStream.get();
    while (!G_IS_SEEKABLE(baseStream) && G_IS_FILTER_INPUT_STREAM(baseStream))
        baseStream = g_filter_input_stream_get_base_stream(G_FILTER_INPUT_STREAM(baseStream));

    if (!G_IS_SEEKABLE(baseStream))
        return 0;

    return g_seekable_tell(G_SEEKABLE(baseStream));
}

static void nextMultipartResponsePartCallback(GObject* /*source*/, GAsyncResult* result, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    if (handle->cancelledOrClientless()) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    ResourceHandleInternal* d = handle->getInternal();
    ASSERT(!d->m_inputStream);

    GUniqueOutPtr<GError> error;
    d->m_inputStream = adoptGRef(soup_multipart_input_stream_next_part_finish(d->m_multipartInputStream.get(), result, &error.outPtr()));

    if (error) {
        handle->client()->didFail(handle.get(), ResourceError::httpError(d->m_soupMessage.get(), error.get(), d->m_soupRequest.get()));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (!d->m_inputStream) {
        handle->client()->didFinishLoading(handle.get());
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    d->m_response = ResourceResponse();
    d->m_response.setURL(handle->firstRequest().url());
    d->m_response.updateFromSoupMessageHeaders(soup_multipart_input_stream_get_headers(d->m_multipartInputStream.get()));

    d->m_previousPosition = 0;

    handle->didReceiveResponse(ResourceResponse(d->m_response), [handle = makeRef(*handle)] {
        continueAfterDidReceiveResponse(handle.ptr());
    });
}

static void sendRequestCallback(GObject*, GAsyncResult* result, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    if (handle->cancelledOrClientless()) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    ResourceHandleInternal* d = handle->getInternal();
    SoupMessage* soupMessage = d->m_soupMessage.get();


    if (d->m_defersLoading) {
        d->m_deferredResult = result;
        return;
    }

    GUniqueOutPtr<GError> error;
    GRefPtr<GInputStream> inputStream = adoptGRef(soup_request_send_finish(d->m_soupRequest.get(), result, &error.outPtr()));
    if (error) {
        handle->client()->didFail(handle.get(), ResourceError::httpError(soupMessage, error.get(), d->m_soupRequest.get()));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (soupMessage) {
        if (handle->shouldContentSniff() && soupMessage->status_code != SOUP_STATUS_NOT_MODIFIED) {
            const char* sniffedType = soup_request_get_content_type(d->m_soupRequest.get());
            d->m_response.setSniffedContentType(sniffedType);
        }
        d->m_response.updateFromSoupMessage(soupMessage);

        if (SOUP_STATUS_IS_REDIRECTION(soupMessage->status_code) && shouldRedirect(handle.get())) {
            d->m_inputStream = inputStream;
            g_input_stream_skip_async(d->m_inputStream.get(), gDefaultReadBufferSize, RunLoopSourcePriority::AsyncIONetwork,
                d->m_cancellable.get(), redirectSkipCallback, handle.get());
            return;
        }
    } else {
        d->m_response.setURL(handle->firstRequest().url());
        const gchar* contentType = soup_request_get_content_type(d->m_soupRequest.get());
        d->m_response.setMimeType(extractMIMETypeFromMediaType(contentType));
        d->m_response.setTextEncodingName(extractCharsetFromMediaType(contentType));
        d->m_response.setExpectedContentLength(soup_request_get_content_length(d->m_soupRequest.get()));
    }

    d->m_response.deprecatedNetworkLoadMetrics().responseStart = MonotonicTime::now() - handle->m_requestTime;

    if (soupMessage && d->m_response.isMultipart())
        d->m_multipartInputStream = adoptGRef(soup_multipart_input_stream_new(soupMessage, inputStream.get()));
    else
        d->m_inputStream = inputStream;

    handle->didReceiveResponse(ResourceResponse(d->m_response), [handle = makeRef(*handle)] {
        continueAfterDidReceiveResponse(handle.ptr());
    });
}

void ResourceHandle::platformContinueSynchronousDidReceiveResponse()
{
    continueAfterDidReceiveResponse(this);
}

static void continueAfterDidReceiveResponse(ResourceHandle* handle)
{
    if (handle->cancelledOrClientless()) {
        cleanupSoupRequestOperation(handle);
        return;
    }

    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_soupMessage && d->m_multipartInputStream && !d->m_inputStream) {
        soup_multipart_input_stream_next_part_async(d->m_multipartInputStream.get(), RunLoopSourcePriority::AsyncIONetwork,
            d->m_cancellable.get(), nextMultipartResponsePartCallback, handle);
        return;
    }

    ASSERT(d->m_inputStream);
    handle->ensureReadBuffer();
    g_input_stream_read_async(d->m_inputStream.get(), const_cast<char*>(d->m_soupBuffer->data), d->m_soupBuffer->length,
        RunLoopSourcePriority::AsyncIONetwork, d->m_cancellable.get(), readCallback, handle);
}

void ResourceHandle::didStartRequest()
{
    getInternal()->m_response.deprecatedNetworkLoadMetrics().requestStart = MonotonicTime::now() - m_requestTime;
}

#if SOUP_CHECK_VERSION(2, 49, 91)
static void startingCallback(SoupMessage*, ResourceHandle* handle)
{
    handle->didStartRequest();
}
#endif // SOUP_CHECK_VERSION(2, 49, 91)

static void networkEventCallback(SoupMessage*, GSocketClientEvent event, GIOStream* stream, gpointer data)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;

    if (handle->cancelledOrClientless())
        return;

    ResourceHandleInternal* d = handle->getInternal();
    Seconds deltaTime = MonotonicTime::now() - handle->m_requestTime;
    switch (event) {
    case G_SOCKET_CLIENT_RESOLVING:
        d->m_response.deprecatedNetworkLoadMetrics().domainLookupStart = deltaTime;
        break;
    case G_SOCKET_CLIENT_RESOLVED:
        d->m_response.deprecatedNetworkLoadMetrics().domainLookupEnd = deltaTime;
        break;
    case G_SOCKET_CLIENT_CONNECTING:
        d->m_response.deprecatedNetworkLoadMetrics().connectStart = deltaTime;
        if (d->m_response.deprecatedNetworkLoadMetrics().domainLookupStart != Seconds(-1)) {
            // WebCore/inspector/front-end/RequestTimingView.js assumes
            // that DNS time is included in connection time so must
            // substract here the DNS delta that will be added later (see
            // WebInspector.RequestTimingView.createTimingTable in the
            // file above for more details).
            d->m_response.deprecatedNetworkLoadMetrics().connectStart -=
                d->m_response.deprecatedNetworkLoadMetrics().domainLookupEnd - d->m_response.deprecatedNetworkLoadMetrics().domainLookupStart;
        }
        break;
    case G_SOCKET_CLIENT_CONNECTED:
        // Web Timing considers that connection time involves dns, proxy & TLS negotiation...
        // so we better pick G_SOCKET_CLIENT_COMPLETE for connectEnd
        break;
    case G_SOCKET_CLIENT_PROXY_NEGOTIATING:
        break;
    case G_SOCKET_CLIENT_PROXY_NEGOTIATED:
        break;
    case G_SOCKET_CLIENT_TLS_HANDSHAKING:
        d->m_response.deprecatedNetworkLoadMetrics().secureConnectionStart = deltaTime;
        ASSERT(G_IS_TLS_CONNECTION(stream));
        g_object_set_data(G_OBJECT(stream), "wk-soup-message", d->m_soupMessage.get());
        g_signal_connect(stream, "accept-certificate", G_CALLBACK(tlsConnectionAcceptCertificateCallback), handle);
        break;
    case G_SOCKET_CLIENT_TLS_HANDSHAKED:
        break;
    case G_SOCKET_CLIENT_COMPLETE:
        d->m_response.deprecatedNetworkLoadMetrics().connectEnd = deltaTime;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

static bool createSoupMessageForHandleAndRequest(ResourceHandle* handle, const ResourceRequest& request)
{
    ASSERT(handle);

    ResourceHandleInternal* d = handle->getInternal();
    ASSERT(d->m_soupRequest);

    d->m_soupMessage = adoptGRef(soup_request_http_get_message(SOUP_REQUEST_HTTP(d->m_soupRequest.get())));
    if (!d->m_soupMessage)
        return false;

    SoupMessage* soupMessage = d->m_soupMessage.get();
    request.updateSoupMessage(soupMessage);
    d->m_bodySize = soupMessage->request_body->length;

    g_object_set_data(G_OBJECT(soupMessage), "handle", handle);
    if (!handle->shouldContentSniff())
        soup_message_disable_feature(soupMessage, SOUP_TYPE_CONTENT_SNIFFER);
    if (!d->m_useAuthenticationManager)
        soup_message_disable_feature(soupMessage, SOUP_TYPE_AUTH_MANAGER);

    // Make sure we have an Accept header for subresources; some sites
    // want this to serve some of their subresources
    if (!soup_message_headers_get_one(soupMessage->request_headers, "Accept"))
        soup_message_headers_append(soupMessage->request_headers, "Accept", "*/*");

    // In the case of XHR .send() and .send("") explicitly tell libsoup to send a zero content-lenght header
    // for consistency with other backends (e.g. Chromium's) and other UA implementations like FF. It's done
    // in the backend here instead of in XHR code since in XHR CORS checking prevents us from this kind of
    // late header manipulation.
    if ((request.httpMethod() == "POST" || request.httpMethod() == "PUT") && !d->m_bodySize)
        soup_message_headers_set_content_length(soupMessage->request_headers, 0);

    g_signal_connect(d->m_soupMessage.get(), "got-headers", G_CALLBACK(gotHeadersCallback), handle);
    g_signal_connect(d->m_soupMessage.get(), "wrote-body-data", G_CALLBACK(wroteBodyDataCallback), handle);

    unsigned flags = SOUP_MESSAGE_NO_REDIRECT;
    soup_message_set_flags(d->m_soupMessage.get(), static_cast<SoupMessageFlags>(soup_message_get_flags(d->m_soupMessage.get()) | flags));

#if SOUP_CHECK_VERSION(2, 49, 91)
    g_signal_connect(d->m_soupMessage.get(), "starting", G_CALLBACK(startingCallback), handle);
#endif
    g_signal_connect(d->m_soupMessage.get(), "network-event", G_CALLBACK(networkEventCallback), handle);
    g_signal_connect(d->m_soupMessage.get(), "restarted", G_CALLBACK(restartedCallback), handle);

#if SOUP_CHECK_VERSION(2, 43, 1)
    soup_message_set_priority(d->m_soupMessage.get(), toSoupMessagePriority(request.priority()));
#endif

    return true;
}

static bool createSoupRequestAndMessageForHandle(ResourceHandle* handle, const ResourceRequest& request)
{
    ResourceHandleInternal* d = handle->getInternal();

    GUniquePtr<SoupURI> soupURI = request.createSoupURI();
    if (!soupURI)
        return false;

    GUniqueOutPtr<GError> error;
    d->m_soupRequest = adoptGRef(soup_session_request_uri(d->soupSession(), soupURI.get(), &error.outPtr()));
    if (error) {
        d->m_soupRequest.clear();
        return false;
    }

    // SoupMessages are only applicable to HTTP-family requests.
    if (request.url().protocolIsInHTTPFamily() && !createSoupMessageForHandleAndRequest(handle, request)) {
        d->m_soupRequest.clear();
        return false;
    }

    request.updateSoupRequest(d->m_soupRequest.get());

    return true;
}

bool ResourceHandle::start()
{
    ASSERT(!d->m_soupMessage);

    // The frame could be null if the ResourceHandle is not associated to any
    // Frame, e.g. if we are downloading a file.
    // If the frame is not null but the page is null this must be an attempted
    // load from an unload handler, so let's just block it.
    // If both the frame and the page are not null the context is valid.
    if (d->m_context && !d->m_context->isValid())
        return false;

    // Only allow the POST and GET methods for non-HTTP requests.
    const ResourceRequest& request = firstRequest();
    if (!request.url().protocolIsInHTTPFamily() && request.httpMethod() != "GET" && request.httpMethod() != "POST") {
        this->scheduleFailure(InvalidURLFailure); // Error must not be reported immediately
        return true;
    }

    applyAuthenticationToRequest(this, firstRequest(), false);

    if (!createSoupRequestAndMessageForHandle(this, request)) {
        this->scheduleFailure(InvalidURLFailure); // Error must not be reported immediately
        return true;
    }

    // Send the request only if it's not been explicitly deferred.
    if (!d->m_defersLoading)
        sendPendingRequest();

    return true;
}

RefPtr<ResourceHandle> ResourceHandle::releaseForDownload(ResourceHandleClient* downloadClient)
{
    // We don't adopt the ref, as it will be released by cleanupSoupRequestOperation, which should always run.
    ResourceHandle* newHandle = new ResourceHandle(d->m_context.get(), firstRequest(), nullptr, d->m_defersLoading, d->m_shouldContentSniff, d->m_shouldContentEncodingSniff);
    newHandle->relaxAdoptionRequirement();
    std::swap(d, newHandle->d);

    g_signal_handlers_disconnect_matched(newHandle->d->m_soupMessage.get(), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
    g_object_set_data(G_OBJECT(newHandle->d->m_soupMessage.get()), "handle", newHandle);

    newHandle->d->m_client = downloadClient;
    continueAfterDidReceiveResponse(newHandle);

    return newHandle;
}

void ResourceHandle::timeoutFired()
{
    client()->didFail(this, ResourceError::timeoutError(firstRequest().url()));
    cancel();
}

void ResourceHandle::sendPendingRequest()
{
    m_requestTime = MonotonicTime::now();

    if (d->m_firstRequest.timeoutInterval() > 0)
        d->m_timeoutSource.startOneShot(1_s * d->m_firstRequest.timeoutInterval());

    // Balanced by a deref() in cleanupSoupRequestOperation, which should always run.
    ref();

    d->m_cancellable = adoptGRef(g_cancellable_new());
    soup_request_send_async(d->m_soupRequest.get(), d->m_cancellable.get(), sendRequestCallback, this);
}

void ResourceHandle::cancel()
{
    d->m_cancelled = true;
    if (d->m_soupMessage)
        soup_session_cancel_message(d->soupSession(), d->m_soupMessage.get(), SOUP_STATUS_CANCELLED);
    else if (d->m_cancellable)
        g_cancellable_cancel(d->m_cancellable.get());
}

bool ResourceHandle::shouldUseCredentialStorage()
{
    return (!client() || client()->shouldUseCredentialStorage(this)) && firstRequest().url().protocolIsInHTTPFamily();
}

void ResourceHandle::continueDidReceiveAuthenticationChallenge(const Credential& credentialFromPersistentStorage)
{
    ASSERT(!d->m_currentWebChallenge.isNull());
    AuthenticationChallenge& challenge = d->m_currentWebChallenge;

    ASSERT(d->m_soupMessage);
    if (!credentialFromPersistentStorage.isEmpty())
        challenge.setProposedCredential(credentialFromPersistentStorage);

    if (!client()) {
        soup_session_unpause_message(d->soupSession(), d->m_soupMessage.get());
        clearAuthentication();
        return;
    }

    client()->didReceiveAuthenticationChallenge(this, challenge);
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    ASSERT(d->m_currentWebChallenge.isNull());

    String partition = firstRequest().cachePartition();

    // FIXME: Per the specification, the user shouldn't be asked for credentials if there were incorrect ones provided explicitly.
    bool useCredentialStorage = shouldUseCredentialStorage();
    if (useCredentialStorage) {
        if (!d->m_initialCredential.isEmpty() || challenge.previousFailureCount()) {
            // The stored credential wasn't accepted, stop using it. There is a race condition
            // here, since a different credential might have already been stored by another
            // ResourceHandle, but the observable effect should be very minor, if any.
            CredentialStorage::defaultCredentialStorage().remove(partition, challenge.protectionSpace());
        }

        if (!challenge.previousFailureCount()) {
            Credential credential = CredentialStorage::defaultCredentialStorage().get(partition, challenge.protectionSpace());
            if (!credential.isEmpty() && credential != d->m_initialCredential) {
                ASSERT(credential.persistence() == CredentialPersistenceNone);

                // Store the credential back, possibly adding it as a default for this directory.
                if (isAuthenticationFailureStatusCode(challenge.failureResponse().httpStatusCode()))
                    CredentialStorage::defaultCredentialStorage().set(partition, credential, challenge.protectionSpace(), challenge.failureResponse().url());

                soup_auth_authenticate(challenge.soupAuth(), credential.user().utf8().data(), credential.password().utf8().data());
                return;
            }
        }
    }

    d->m_currentWebChallenge = challenge;
    soup_session_pause_message(d->soupSession(), d->m_soupMessage.get());

    // We could also do this before we even start the request, but that would be at the expense
    // of all request latency, versus a one-time latency for the small subset of requests that
    // use HTTP authentication. In the end, this doesn't matter much, because persistent credentials
    // will become session credentials after the first use.
    if (useCredentialStorage && d->m_context && d->m_context->isValid()) {
        d->m_context->storageSession().getCredentialFromPersistentStorage(challenge.protectionSpace(), d->m_cancellable.get(), [this, protectedThis = makeRef(*this)] (Credential&& credential) {
            continueDidReceiveAuthenticationChallenge(WTFMove(credential));
        });
        return;
    }

    continueDidReceiveAuthenticationChallenge(Credential());
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    ASSERT(!challenge.isNull());
    if (challenge != d->m_currentWebChallenge)
        return;
    soup_session_unpause_message(d->soupSession(), d->m_soupMessage.get());

    clearAuthentication();
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
    ASSERT(!challenge.isNull());
    if (challenge != d->m_currentWebChallenge)
        return;

    // FIXME: Support empty credentials. Currently, an empty credential cannot be stored in WebCore credential storage, as that's empty value for its map.
    if (credential.isEmpty()) {
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    String partition = firstRequest().cachePartition();

    if (shouldUseCredentialStorage()) {
        // Eventually we will manage per-session credentials only internally or use some newly-exposed API from libsoup,
        // because once we authenticate via libsoup, there is no way to ignore it for a particular request. Right now,
        // we place the credentials in the store even though libsoup will never fire the authenticate signal again for
        // this protection space.
        if (credential.persistence() == CredentialPersistenceForSession || credential.persistence() == CredentialPersistencePermanent)
            CredentialStorage::defaultCredentialStorage().set(partition, credential, challenge.protectionSpace(), challenge.failureResponse().url());

        if (credential.persistence() == CredentialPersistencePermanent) {
            d->m_credentialDataToSaveInPersistentStore.credential = credential;
            d->m_credentialDataToSaveInPersistentStore.protectionSpace = challenge.protectionSpace();
        }
    }

    ASSERT(d->m_soupMessage);
    soup_auth_authenticate(challenge.soupAuth(), credential.user().utf8().data(), credential.password().utf8().data());
    soup_session_unpause_message(d->soupSession(), d->m_soupMessage.get());

    clearAuthentication();
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    ASSERT(!challenge.isNull());
    if (challenge != d->m_currentWebChallenge)
        return;

    if (cancelledOrClientless()) {
        clearAuthentication();
        return;
    }

    ASSERT(d->m_soupMessage);
    soup_session_unpause_message(d->soupSession(), d->m_soupMessage.get());

    if (client())
        client()->receivedCancellation(this, challenge);

    clearAuthentication();
}

void ResourceHandle::receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&)
{
    ASSERT_NOT_REACHED();
}

void ResourceHandle::receivedChallengeRejection(const AuthenticationChallenge& challenge)
{
    // This is only used by layout tests, soup based ports don't implement this.
    notImplemented();
    receivedRequestToContinueWithoutCredential(challenge);
}

static bool waitingToSendRequest(ResourceHandle* handle)
{
    // We need to check for d->m_soupRequest because the request may have raised a failure
    // (for example invalid URLs). We cannot  simply check for d->m_scheduledFailure because
    // it's cleared as soon as the failure event is fired.
    return handle->getInternal()->m_soupRequest && !handle->getInternal()->m_cancellable;
}

void ResourceHandle::platformSetDefersLoading(bool defersLoading)
{
    if (cancelledOrClientless())
        return;

    // Except when canceling a possible timeout timer, we only need to take action here to UN-defer loading.
    if (defersLoading) {
        d->m_timeoutSource.stop();
        return;
    }

    if (waitingToSendRequest(this)) {
        sendPendingRequest();
        return;
    }

    if (d->m_deferredResult) {
        GRefPtr<GAsyncResult> asyncResult = adoptGRef(d->m_deferredResult.leakRef());

        if (d->m_inputStream)
            readCallback(G_OBJECT(d->m_inputStream.get()), asyncResult.get(), this);
        else
            sendRequestCallback(G_OBJECT(d->m_soupRequest.get()), asyncResult.get(), this);
    }
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext*, const ResourceRequest&, StoredCredentialsPolicy, ResourceError&, ResourceResponse&, Vector<char>&)
{
    ASSERT_NOT_REACHED();
}

static void readCallback(GObject*, GAsyncResult* asyncResult, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    if (handle->cancelledOrClientless()) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_defersLoading) {
        d->m_deferredResult = asyncResult;
        return;
    }

    GUniqueOutPtr<GError> error;
    gssize bytesRead = g_input_stream_read_finish(d->m_inputStream.get(), asyncResult, &error.outPtr());

    if (error) {
        handle->client()->didFail(handle.get(), ResourceError::genericGError(error.get(), d->m_soupRequest.get()));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (!bytesRead) {
        // If this is a multipart message, we'll look for another part.
        if (d->m_soupMessage && d->m_multipartInputStream) {
            d->m_inputStream.clear();
            soup_multipart_input_stream_next_part_async(d->m_multipartInputStream.get(), RunLoopSourcePriority::AsyncIONetwork,
                d->m_cancellable.get(), nextMultipartResponsePartCallback, handle.get());
            return;
        }

        g_input_stream_close(d->m_inputStream.get(), 0, 0);

        handle->client()->didFinishLoading(handle.get());
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    // It's mandatory to have sent a response before sending data
    ASSERT(!d->m_response.isNull());

    size_t currentPosition = handle->currentStreamPosition();
    size_t encodedDataLength = currentPosition ? currentPosition - d->m_previousPosition : bytesRead;

    ASSERT(d->m_soupBuffer);
    d->m_soupBuffer->length = bytesRead; // The buffer might be larger than the number of bytes read. SharedBuffer looks at the length property.
    handle->client()->didReceiveBuffer(handle.get(), SharedBuffer::wrapSoupBuffer(d->m_soupBuffer.release()), encodedDataLength);

    d->m_previousPosition = currentPosition;

    // didReceiveBuffer may cancel the load, which may release the last reference.
    if (handle->cancelledOrClientless()) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    handle->ensureReadBuffer();
    g_input_stream_read_async(d->m_inputStream.get(), const_cast<char*>(d->m_soupBuffer->data), d->m_soupBuffer->length, RunLoopSourcePriority::AsyncIONetwork,
        d->m_cancellable.get(), readCallback, handle.get());
}

}

#endif
