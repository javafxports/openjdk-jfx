/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.layout.StackPane;
import javafx.scene.web.HTMLEditor;
import javafx.scene.web.WebEngine;
import javafx.scene.web.WebView;
import javafx.stage.Stage;

/**
 * @test
 * @bug JDK-8147476 Rendering issues with MathML token elements
 * @bug JDK-8089878 HTMLEditor messes up MathML markup press
 */
public class OpenJFXMathMLRenderingIssueTest extends Application {

    final HTMLEditor htmlEditor = new HTMLEditor();
    final StackPane root = new StackPane();
    final WebView webView = (WebView) htmlEditor.lookup(".web-view");

    public void init() {
        htmlEditor.setHtmlText(content);
        root.getChildren().add(htmlEditor);
        htmlEditor.setPrefWidth(400);
        htmlEditor.setPrefHeight(150);
    }

    @Override
    public void start(Stage primaryStage) {

        webView.focusedProperty().addListener((observable, oldValue, newValue) -> {

            if (newValue) {
                webView.getBoundsInParent().toString();

                WebEngine we = webView.getEngine();
                int height = (int) we.executeScript(
                        "elements = document.getElementsByTagName('mo');"
                        + "element = elements[0].clientHeight;"
                );

                if (height < 2) {
                    Platform.exit();
                    throw new RuntimeException(" : MathML rendering issues see JDK-8089878 and JDK-8147476.\n");
                }

                System.out.println("[Ok]\n");
                Platform.exit();
            }
        });

        primaryStage.setTitle("OpenJFX MathML Rendering Issue Test");
        primaryStage.setScene(new Scene(root));
        primaryStage.show();

    }
    /**
     * @param args the command line arguments
     */
    /*public static void main(String[] args) {
        launch(args);
    }*/
    String BodyContent
            = "<math display=\"block\">"
            + "   <mrow>"
            + "      <mi>x</mi>"
            + "      <mo>=</mo>"
            + "      <mfrac>"
            + "         <mrow>"
            + "            <mo>−</mo>"
            + "            <mi>b</mi>"
            + "            <mo>±</mo>"
            + "            <msqrt>"
            + "               <mrow>"
            + "                  <msup>"
            + "                     <mi>b</mi>"
            + "                     <mn>2</mn>"
            + "                  </msup>"
            + "                  <mo>−</mo>"
            + "                  <mn>4</mn>"
            + "                  <mi>a</mi>"
            + "                  <mi>c</mi>"
            + "               </mrow>"
            + "            </msqrt>"
            + "         </mrow>"
            + "         <mrow>"
            + "            <mn>2</mn>"
            + "            <mi>a</mi>"
            + "         </mrow>"
            + "      </mfrac>"
            + "   </mrow>"
            + "</math>";

    String content = "<!doctype html>"
            + "<html>"
            + "   <head>"
            + "      <meta charset=\"UTF-8\">"
            + "      <title>OpenJFX and MathML</title>"
            + "   </head>"
            + "   <body>"
            + "      <p>"
            + BodyContent
            + "      </p>"
            + "   </body>"
            + "</html>";

}
