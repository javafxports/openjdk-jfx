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
final class NewHTTPLoader extends URLLoaderBase {

    @Native public static final int ALLOW_UNASSIGNED = java.net.IDN.ALLOW_UNASSIGNED;

    private static final PlatformLogger logger =
            PlatformLogger.getLogger(URLLoader.class.getName());
    private static final int MAX_REDIRECTS = 10;
    private static final int MAX_BUF_COUNT = 3;
    private static final String GET = "GET";
    private static final String HEAD = "HEAD";
    private static final String DELETE = "DELETE";


    private final WebPage webPage;
    private final boolean asynchronous;
    private String url;
    private String method;
    private final String headers;
    private FormDataElement[] formDataElements;
    private final long data;
    private volatile boolean canceled = false;

    private static HttpClient HTTP_CLIENT = HttpClient.newBuilder()
                   .version(Version.HTTP_2)  // this is the default
                   .followRedirects(Redirect.NORMAL)
                   .build();

    /**
     * Creates a new {@code URLLoader}.
     */
    NewHTTPLoader(WebPage webPage,
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
                                .filter(s -> !s.toLowerCase().startsWith("referer:"))
                                .map(s -> { int i = s.indexOf(":"); return new String[] {s.substring(0, i), s.substring(i + 2)};})
                                .flatMap(Arrays::stream)
                                .toArray(String[]::new);
        final var request = HttpRequest.newBuilder()
                       .uri(URI.create(url))
                       .headers(parsedHeaders)
                       .timeout(Duration.ofSeconds(10))
                       .build();

        final BodyHandler<Void> bh = rsp -> {
            // System.err.println("status:" + rsp.statusCode() + ", rsp.headers:" + rsp.headers());
            callBack(() -> {
                twkDidReceiveResponse(
                        rsp.statusCode(),
                        rsp.headers().firstValue("content-type").orElse("application/octet-stream"),
                        "",
                        rsp.headers().firstValueAsLong("content-length").orElse(-1),
                        rsp.headers().map().entrySet().stream().map(e -> String.format("%s:%s", e.getKey(), e.getValue().stream().collect(Collectors.joining(",")))).collect(Collectors.joining("\n")),
                        this.url,
                        data);
            });
            return BodySubscribers.fromSubscriber(new Flow.Subscriber<List<ByteBuffer>>() {
                  private Flow.Subscription su;
                  @Override
                  public void onComplete() {
                      // System.err.println("Done");
                      callBack(() -> twkDidFinishLoading(data));
                  }

                  @Override
                  public void onError(Throwable th) {
                      System.err.println("Errr:" + th);
                  }

                  @Override
                  public void onNext(final List<ByteBuffer> b) {
                      // System.err.println("onNext:" + Thread.currentThread() + b);
                      su.request(1);
                      callBack(() -> {
                          b.stream().map(bb -> { final var newbb = ByteBuffer.allocateDirect(bb.capacity()); return newbb.put(bb);}).forEach(bb -> twkDidReceiveData(bb.flip(), bb.position(), bb.remaining(), data));
                      });
                  }

                  @Override
                  public void onSubscribe(Flow.Subscription su) {
                      this.su = su;
                      // System.err.println("onSubscribe:" + su);
                      su.request(1);
                  }
        });};

        var res = HTTP_CLIENT.sendAsync(request, bh)
                  .thenAccept(response -> {
                       // System.err.println("Response status code: " + response.statusCode());
                       // System.err.println("Response headers: " + response.headers());
                       // System.err.println("Response body: " + response.body());
                       // System.err.println("Thread:" + Thread.currentThread());}
                    })
                  .exceptionally(ex -> {
                       System.err.println("@@@@ Exception:" + ex + ", cause0:" + ex.getCause() + ", cause1:" + ex.getCause().getCause()); return null; });

        // try {
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

    protected void callBack(Runnable runnable) {
        if (asynchronous) {
            Invoker.getInvoker().invokeOnEventThread(runnable);
        } else {
            runnable.run();
        }
    }
}
