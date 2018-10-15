/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

import com.sun.javafx.logging.PlatformLogger;
import com.sun.javafx.logging.PlatformLogger.Level;
import com.sun.webkit.Invoker;
import com.sun.webkit.LoadListenerClient;
import com.sun.webkit.WebPage;
import static com.sun.webkit.network.URLs.newURL;
import java.io.EOFException;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.lang.annotation.Native;
import java.net.ConnectException;
import java.net.HttpRetryException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.NoRouteToHostException;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLDecoder;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.security.AccessControlException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.zip.GZIPInputStream;
import java.util.zip.InflaterInputStream;
import javax.net.ssl.SSLHandshakeException;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse.BodyHandler;
import java.net.http.HttpResponse;
import java.nio.ByteBuffer;
import java.time.Duration;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.Flow;
import static java.net.http.HttpClient.Redirect;
import static java.net.http.HttpClient.Version;
import static java.net.http.HttpResponse.BodyHandlers;
import static java.net.http.HttpResponse.BodySubscribers;
/**
 * A runnable that loads a resource specified by a URL.
 */
final class HTTP2Loader extends URLLoaderBase {

    private static final PlatformLogger logger =
            PlatformLogger.getLogger(URLLoader.class.getName());
    private static final int MAX_BUF_COUNT = 3;


    private final WebPage webPage;
    private final boolean asynchronous;
    private String url;
    private String method;
    private final String headers;
    private FormDataElement[] formDataElements;
    private final long data;
    private volatile boolean canceled = false;

    // TODO: Check for security implications, otherwise
    // use one instance per WebPage instead of Singleton.
    private static HttpClient HTTP_CLIENT = HttpClient.newBuilder()
                   .version(Version.HTTP_2)  // this is the default
                   // .followRedirects(Redirect.NORMAL)
                   .connectTimeout(Duration.ofSeconds(30))
                   .build();

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
        // FIXME: As of now only asynchronous requests are supported.
        if (formDataElements == null && asynchronous && (url.startsWith("http://") || url.startsWith("https://"))) {
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

        final String parsedHeaders[] = Arrays.stream(headers.split("\n"))
                                // .filter(s -> !s.matches("(?i)^origin:.*|^referer:.*")) // Depends on JDK-8203850
                                .flatMap(s -> { int i = s.indexOf(":"); return Stream.of(s.substring(0, i), s.substring(i + 2));})
                                .toArray(String[]::new);
        URI uriObj;
        try {
            uriObj = newURL(url).toURI();
        } catch(URISyntaxException | MalformedURLException e) {
            uriObj = URI.create(url);
        }
        final var requestBuilder = HttpRequest.newBuilder()
                       .uri(uriObj)
                       .headers(parsedHeaders)
                       .version(Version.HTTP_2);  // this is the default

        // TODO: POST with formDataElements has to be handled
        requestBuilder.method(method, HttpRequest.BodyPublishers.noBody());

        final var request = requestBuilder.build();

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
                      System.err.println("Errr:" + th);
                  }

                  @Override
                  public void onNext(final List<ByteBuffer> b) {
                      didReceiveData(b);
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
        });};

        var res = HTTP_CLIENT.sendAsync(request, bodyHandler)
                  .thenAccept(response -> { })
                  .exceptionally(ex -> {
                       System.err.println("@@@@ Exception:" + ex + ", cause0:" + ex.getCause() + ", cause1:" + ex.getCause().getCause() + ", url:" + url); return null; });

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

    private void callBack(Runnable runnable) {
        if (asynchronous) {
            Invoker.getInvoker().invokeOnEventThread(runnable);
        } else {
            runnable.run();
        }
    }

    private URL asURL(final String uri) throws MalformedURLException {
        URL newUrl;
        try {
            newUrl = newURL(uri);
        } catch (MalformedURLException mue) {
            newUrl = newURL(new URL(this.url), uri);
        }
        return newUrl;
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
        return rsp.headers().map().entrySet().stream().map(e -> String.format("%s:%s", e.getKey(), e.getValue().stream().collect(Collectors.joining(",")))).collect(Collectors.joining("\n")) + "\n";
    }

    private void willSendRequest(final HttpResponse.ResponseInfo rsp) {
        callBack(() -> {
            if (!canceled) {
                twkWillSendRequest(
                        rsp.statusCode(),
                        getContentType(rsp),
                        "",
                        getContentLength(rsp),
                        getHeadersAsString(rsp),
                        this.url,
                        data);
            }
        });
    }

    private void didReceiveResponse(final HttpResponse.ResponseInfo rsp) {
        callBack(() -> {
            if (!canceled) {
                twkDidReceiveResponse(
                        rsp.statusCode(),
                        getContentType(rsp),
                        "",
                        getContentLength(rsp),
                        getHeadersAsString(rsp),
                        this.url,
                        data);
            }
        });
    }

    private void didReceiveData(final List<ByteBuffer> b) {
        callBack(() -> {
            b.stream().filter((bb) -> !canceled).map(bb -> ByteBuffer.allocateDirect(bb.capacity()).put(bb)).forEach(bb -> twkDidReceiveData(bb.flip(), bb.position(), bb.remaining(), data));
        });
    }

    private void didFinishLoading() {
        callBack(() -> {
            if (!canceled) {
                twkDidFinishLoading(data);
            }
        });
    }
}
