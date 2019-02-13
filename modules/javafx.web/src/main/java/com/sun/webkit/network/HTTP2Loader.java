/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package com.sun.webkit.network;

import com.sun.javafx.logging.PlatformLogger.Level;
import com.sun.javafx.logging.PlatformLogger;
import com.sun.webkit.Invoker;
import com.sun.webkit.LoadListenerClient;
import com.sun.webkit.WebPage;
import java.io.EOFException;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.SequenceInputStream;
import java.io.UnsupportedEncodingException;
import java.lang.annotation.Native;
import java.net.ConnectException;
import java.net.CookieHandler;
import java.net.HttpRetryException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.NoRouteToHostException;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLDecoder;
import java.net.UnknownHostException;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse.BodyHandler;
import java.net.http.HttpResponse;
import java.net.http.HttpTimeoutException;
import java.nio.ByteBuffer;
import java.security.AccessControlException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.time.Duration;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Flow;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javax.net.ssl.SSLHandshakeException;
import static com.sun.webkit.network.URLs.newURL;
import static java.net.http.HttpClient.Redirect;
import static java.net.http.HttpClient.Version;
import static java.net.http.HttpResponse.BodyHandlers;
import static java.net.http.HttpResponse.BodySubscribers;

final class HTTP2Loader extends URLLoaderBase {

    private static final PlatformLogger logger =
            PlatformLogger.getLogger(URLLoader.class.getName());

    private final WebPage webPage;
    private final boolean asynchronous;
    private String url;
    private String method;
    private final String headers;
    private FormDataElement[] formDataElements;
    private final long data;
    private volatile boolean canceled = false;

    private final CompletableFuture<Void> response;
    // TODO: Check for security implications, otherwise
    // use one instance per WebPage instead of Singleton.
    private final static HttpClient HTTP_CLIENT = HttpClient.newBuilder()
                   .version(Version.HTTP_2)  // this is the default
                   .followRedirects(Redirect.NEVER) // WebCore handles redirection
                   .connectTimeout(Duration.ofSeconds(30)) // FIXME: Add a property to control the timeout
                   .cookieHandler(CookieHandler.getDefault())
                   .build();

    // Singleton instance of direct ByteBuffer to transfer downloaded bytes from
    // Java to native
    private static final int DEFAULT_BUFSIZE = 40 * 1024;
    private final static ByteBuffer BUFFER;
    static {
       int bufSize  = AccessController.doPrivileged(
                        (PrivilegedAction<Integer>) () ->
                            Integer.valueOf(System.getProperty("jdk.httpclient.bufsize", Integer.toString(DEFAULT_BUFSIZE))));
       BUFFER = ByteBuffer.allocateDirect(bufSize);
    }

    /**
     * Creates a new {@code HTTP2Loader}.
     */
    static HTTP2Loader create(WebPage webPage,
              ByteBufferPool byteBufferPool,
              boolean asynchronous,
              String url,
              String method,
              String headers,
              FormDataElement[] formDataElements,
              long data) {
        if (url.startsWith("http://") || url.startsWith("https://")) {
            return new HTTP2Loader(
                webPage,
                byteBufferPool,
                asynchronous,
                url,
                method,
                headers,
                formDataElements,
                data);
        }
        return null;
    }

    // following 2 methods can be generalized and keep a common
    // implementation with URLLoader.java
    private String[] getCustomHeaders() {
        final var loc = Locale.getDefault();
        String lang = "";
        if (!loc.equals(Locale.US) && !loc.equals(Locale.ENGLISH)) {
            lang = loc.getCountry().isEmpty() ?
                loc.getLanguage() + ",":
                loc.getLanguage() + "-" + loc.getCountry() + ",";
        }

        return new String[] { "Accept-Language", lang.toLowerCase() + "en-us;q=0.8,en;q=0.7",
                              // "Accept-Encoding", "gzip", // not yet supported
                              "Accept-Charset", "ISO-8859-1,utf-8;q=0.7,*;q=0.7",
        };
    }

    private String[] getRequestHeaders() {
        return Arrays.stream(headers.split("\n"))
                     .flatMap(s -> Stream.of(s.split(":", 2))) // split from first occurance of :
                     .toArray(String[]::new);
    }

    private URI toURI() throws MalformedURLException {
        URI uriObj;
        try {
            uriObj = new URI(this.url);
        } catch(URISyntaxException | IllegalArgumentException e) {
            // slow path
            try {
                var urlObj = newURL(this.url);
                uriObj = new URI(
                        urlObj.getProtocol(),
                        urlObj.getUserInfo(),
                        urlObj.getHost(),
                        urlObj.getPort(),
                        urlObj.getPath(),
                        urlObj.getQuery(),
                        urlObj.getRef());
            } catch(URISyntaxException | MalformedURLException | IllegalArgumentException ex) {
                throw new MalformedURLException(this.url);
            }
        }
        return uriObj;
    }

    private HttpRequest.BodyPublisher getFormDataPublisher() {
        if (this.formDataElements == null) {
            return HttpRequest.BodyPublishers.noBody();
        }

        final var formDataElementsStream = new Vector<InputStream>();
        final AtomicLong length = new AtomicLong();
        for (final var formData : formDataElements) {
            try {
                formData.open();
                length.addAndGet(formData.getSize());
                formDataElementsStream.add(formData.getInputStream());
            } catch(IOException ex) {
                return null;
            }
        }

        final var stream = new SequenceInputStream(formDataElementsStream.elements());
        final var streamBodyPublisher = HttpRequest.BodyPublishers.ofInputStream(() -> stream);
        // Forwarding implementation to send didSendData notification
        // to WebCore. Otherwise `formDataPublisher = streamBodyPublisher`
        // can do the job.
        final var formDataPublisher = new HttpRequest.BodyPublisher() {
            @Override
            public long contentLength() {
                // streaming or fixed length
                return length.longValue() <= Integer.MAX_VALUE ? length.longValue() : -1;
            }

            @Override
            public void subscribe(Flow.Subscriber<? super ByteBuffer> subscriber) {
                streamBodyPublisher.subscribe(new Flow.Subscriber<ByteBuffer>() {
                    @Override
                    public void onComplete() {
                        subscriber.onComplete();
                    }

                    @Override
                    public void onError(Throwable th) {
                        subscriber.onError(th);
                    }

                    @Override
                    public void onNext(ByteBuffer bytes) {
                        subscriber.onNext(bytes);
                        didSendData(bytes.limit(), length.longValue());
                    }

                    @Override
                    public void onSubscribe(Flow.Subscription subscription) {
                        subscriber.onSubscribe(subscription);
                    }
                });
            }
        };
        return formDataPublisher;
    }

    private HTTP2Loader(WebPage webPage,
              ByteBufferPool byteBufferPool,
              boolean asynchronous,
              String url,
              String method,
              String headers,
              FormDataElement[] formDataElements,
              long data)
    {
        this.webPage = webPage;
        this.asynchronous = asynchronous;
        this.url = url;
        this.method = method;
        this.headers = headers;
        this.formDataElements = formDataElements;
        this.data = data;

        URI uri;
        try {
            uri = toURI();
        } catch(MalformedURLException e) {
            this.response = null;
            didFail(e);
            return;
        }


        final var request = HttpRequest.newBuilder()
                               .uri(uri)
                               .headers(getRequestHeaders()) // headers from WebCore
                               .headers(getCustomHeaders()) // headers set by us
                               .version(Version.HTTP_2)  // this is the default
                               .method(method, getFormDataPublisher())
                               .build();

        final BodyHandler<Void> bodyHandler = rsp -> {
            if(!handleRedirectionIfNeeded(rsp)) {
                didReceiveResponse(rsp);
            }
            return BodySubscribers.fromSubscriber(new Flow.Subscriber<List<ByteBuffer>>() {
                private Flow.Subscription subscription;
                @Override
                public void onComplete() {
                    didFinishLoading();
                }

                @Override
                public void onError(Throwable th) {
                    // nop
                    // System.err.println("Errr:" + th);
                }

                @Override
                public void onNext(final List<ByteBuffer> bytes) {
                    didReceiveData(bytes);
                    requestIfNotCancelled();
                }

                @Override
                public void onSubscribe(Flow.Subscription subscription) {
                    this.subscription = subscription;
                    requestIfNotCancelled();
                }

                private void requestIfNotCancelled() {
                    if (canceled) {
                        subscription.cancel();
                    } else {
                        subscription.request(1);
                    }
                }
            });
        };

        // Run the HttpClient in the page's access control context
        this.response = AccessController.doPrivileged((PrivilegedAction<CompletableFuture<Void>>) () -> {
            return HTTP_CLIENT.sendAsync(request, bodyHandler)
                              .thenAccept($ -> {})
                              .exceptionally(ex -> didFail(ex.getCause()));
        }, webPage.getAccessControlContext());

        // Wait for the result
        if (!asynchronous) {
            this.response.join();
        }
    }

    /**
     * Cancels this loader.
     */
    @Override
    public void fwkCancel() {
        if (logger.isLoggable(Level.FINEST)) {
            logger.finest(String.format("data: [0x%016X]", data));
        }
        canceled = true;
    }

    private void callBackIfNotCancelled(final Runnable r) {
        if (asynchronous) {
            Invoker.getInvoker().invokeOnEventThread(() -> {
                if (!canceled) {
                    r.run();
                }
            });
        } else {
            r.run();
        }
    }

    private boolean handleRedirectionIfNeeded(final HttpResponse.ResponseInfo rsp) {
        switch(rsp.statusCode()) {
                case 301: // Moved Permanently
                case 302: // Found
                case 303: // See Other
                case 307: // Temporary Redirect
                    willSendRequest(rsp);
                    return true;

                case 304: // Not Modified
                    didReceiveResponse(rsp);
                    didFinishLoading();
                    return true;
        }
        return false;
    }

    private static long getContentLength(final HttpResponse.ResponseInfo rsp) {
        return rsp.headers().firstValueAsLong("content-length").orElse(-1);
    }

    private static String getContentType(final HttpResponse.ResponseInfo rsp) {
        return rsp.headers().firstValue("content-type").orElse("application/octet-stream");
    }

    private static String getHeadersAsString(final HttpResponse.ResponseInfo rsp) {
        return rsp.headers()
                  .map()
                  .entrySet()
                  .stream()
                  .map(e -> String.format("%s:%s", e.getKey(), e.getValue().stream().collect(Collectors.joining(","))))
                  .collect(Collectors.joining("\n")) + "\n";
    }

    private void willSendRequest(final HttpResponse.ResponseInfo rsp) {
        callBackIfNotCancelled(() -> {
            twkWillSendRequest(
                    rsp.statusCode(),
                    getContentType(rsp),
                    "",
                    getContentLength(rsp),
                    getHeadersAsString(rsp),
                    this.url,
                    data);
        });
    }

    private void didReceiveResponse(final HttpResponse.ResponseInfo rsp) {
        callBackIfNotCancelled(() -> {
            twkDidReceiveResponse(
                    rsp.statusCode(),
                    getContentType(rsp),
                    "",
                    getContentLength(rsp),
                    getHeadersAsString(rsp),
                    this.url,
                    data);
        });
    }

    private ByteBuffer copyToDirectBuffer(final ByteBuffer bb) {
        ByteBuffer dbb = BUFFER;
        // Though the chance of reaching here is rare, handle the
        // case by allocating a tmp direct buffer.
        if (bb.limit() > dbb.capacity()) {
            dbb = ByteBuffer.allocateDirect(bb.limit());
        }
        return dbb.clear().put(bb).flip();
    }

    private void didReceiveData(final List<ByteBuffer> bytes) {
        callBackIfNotCancelled(() -> bytes.stream()
                                          .map(this::copyToDirectBuffer)
                                          .forEach(this::notifyDidReceiveData)
        );
    }

    private void notifyDidReceiveData(ByteBuffer byteBuffer) {
        if (logger.isLoggable(Level.FINEST)) {
            logger.finest(String.format(
                    "byteBuffer: [%s], "
                    + "position: [%s], "
                    + "remaining: [%s], "
                    + "data: [0x%016X]",
                    byteBuffer,
                    byteBuffer.position(),
                    byteBuffer.remaining(),
                    data));
        }
        twkDidReceiveData(byteBuffer, byteBuffer.position(), byteBuffer.remaining(), data);
    }

    private void didFinishLoading() {
        callBackIfNotCancelled(() -> twkDidFinishLoading(data));
    }

    private Void didFail(final Throwable th) {
        callBackIfNotCancelled(() ->  {
            // FIXME: simply copied from URLLoade.java, it should be
            // retwritten using if..else rather than throw.
            int errorCode;
            try {
                throw th;
            } catch (MalformedURLException ex) {
                errorCode = LoadListenerClient.MALFORMED_URL;
            } catch (AccessControlException ex) {
                errorCode = LoadListenerClient.PERMISSION_DENIED;
            } catch (UnknownHostException ex) {
                errorCode = LoadListenerClient.UNKNOWN_HOST;
            } catch (NoRouteToHostException ex) {
                errorCode = LoadListenerClient.NO_ROUTE_TO_HOST;
            } catch (ConnectException ex) {
                errorCode = LoadListenerClient.CONNECTION_REFUSED;
            } catch (SocketException ex) {
                errorCode = LoadListenerClient.CONNECTION_RESET;
            } catch (SSLHandshakeException ex) {
                errorCode = LoadListenerClient.SSL_HANDSHAKE;
            } catch (SocketTimeoutException | HttpTimeoutException ex) {
                errorCode = LoadListenerClient.CONNECTION_TIMED_OUT;
            } catch (FileNotFoundException ex) {
                errorCode = LoadListenerClient.FILE_NOT_FOUND;
            } catch (Throwable ex) {
                errorCode = LoadListenerClient.UNKNOWN_ERROR;
            }
            notifyDidFail(errorCode, url, th.getMessage());
        });
        return null;
    }

    private void notifyDidFail(int errorCode, String url, String message) {
        if (logger.isLoggable(Level.FINEST)) {
            logger.finest(String.format(
                    "errorCode: [%d], "
                    + "url: [%s], "
                    + "message: [%s], "
                    + "data: [0x%016X]",
                    errorCode,
                    url,
                    message,
                    data));
        }
        twkDidFail(errorCode, url, message, data);
    }

    private void didSendData(final long totalBytesSent,
                             final long totalBytesToBeSent)
    {
        callBackIfNotCancelled(() -> notifyDidSendData(totalBytesSent, totalBytesToBeSent));
    }

    private void notifyDidSendData(long totalBytesSent,
                                   long totalBytesToBeSent)
    {
        if (logger.isLoggable(Level.FINEST)) {
            logger.finest(String.format(
                    "totalBytesSent: [%d], "
                    + "totalBytesToBeSent: [%d], "
                    + "data: [0x%016X]",
                    totalBytesSent,
                    totalBytesToBeSent,
                    data));
        }
        twkDidSendData(totalBytesSent, totalBytesToBeSent, data);
    }
}
