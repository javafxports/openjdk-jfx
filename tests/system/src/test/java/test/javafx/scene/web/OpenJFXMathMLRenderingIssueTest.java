import javafx.application.Application;
import javafx.scene.Scene;
import javafx.scene.layout.StackPane;
import javafx.scene.web.HTMLEditor;
import javafx.scene.web.WebEngine;
import javafx.scene.web.WebView;
import javafx.stage.Stage;

public class OpenJFXMathMLRenderingIssueTest extends Application {

    final HTMLEditor htmlEditor = new HTMLEditor();
    final StackPane root = new StackPane();

    public void init() {
        htmlEditor.setHtmlText(content);
        WebView webView = (WebView) htmlEditor.lookup(".web-view");
        WebEngine we = webView.getEngine();
        root.getChildren().add(htmlEditor);
        htmlEditor.setPrefWidth(400);
        htmlEditor.setPrefHeight(150);
        webView.focusedProperty().addListener((observable, oldValue, newValue) -> {
            //webView.sethtmlEditor.setHtmlText(content);

            if (newValue) {
                webView.getBoundsInParent().toString();

                int height = (int) we.executeScript(
                        "elements = document.getElementsByTagName('mo');"
                        + "element = elements[0].clientHeight;"
                );
                if (height < 2) {
                    throw new RuntimeException("Bad");
                }

                System.out.println("Ok");
            }
        });
    }

    @Override
    public void start(Stage primaryStage) {

        primaryStage.setTitle("OpenJFX MathML Rendering Issue Test");
        primaryStage.setScene(new Scene(root));
        primaryStage.show();

        //htmlEditor.setHtmlText(content);
        //NodeList nl = we.getDocument().getElementsByTagName("mo");
        //webView.getHeight()
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
