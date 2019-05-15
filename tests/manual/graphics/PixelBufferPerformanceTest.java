import javafx.application.Application;
import javafx.geometry.Rectangle2D;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.TextField;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.image.PixelBuffer;
import javafx.scene.image.PixelFormat;
import javafx.scene.image.PixelWriter;
import javafx.scene.image.WritableImage;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.stage.Stage;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class PixelBufferPerformanceTest extends Application {

    private int scW = 1000;
    private int scH = 1000;
    private int imW = scW;
    private int imH = scH;
    private int cpyW = imW;
    private int cpyH = imH;

    private PixelBuffer pb;
    private ByteBuffer byteBuffer;
    private ArrayList<Color> colors = new ArrayList<>();
    private int count = 0;
    private List<ByteBuffer> copyBuffers = new ArrayList<>();

    private VBox wImgContainer = new VBox(8);
    private VBox pbImgContainer = new VBox(8);


    private ByteBuffer createBuffer(int w, int h, Color c) {
        ByteBuffer bf = ByteBuffer.allocateDirect(w * h * 4);
        byte red = (byte) Math.round(c.getRed() * 255.0);
        byte green = (byte) Math.round(c.getGreen() * 255.0);
        byte blue = (byte) Math.round(c.getBlue() * 255.0);
        byte alpha = (byte) 255;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                bf.put(blue); bf.put(green);
                bf.put(red); bf.put(alpha);
            }
        }
        bf.rewind();
        return bf;
    }

    private void createCopyBuffers() {
        for(Color clr : colors) {
            ByteBuffer buf = createBuffer(cpyW, cpyH, clr);
            copyBuffers.add(buf);
        }
    }

    private void updateDirectByteBuffer(ByteBuffer buf) {
        ByteBuffer src = copyBuffers.get(count++);
        buf.put(src);

        buf.rewind();
        src.rewind();
        if (count >= copyBuffers.size()) {
            count = 0;
        }
    }

    private WritableImage createWImageFromBuffer(int w, int h, Color c) {
        byteBuffer = createBuffer(w, h, c);
        WritableImage img = new WritableImage(w, h);
        PixelFormat<ByteBuffer> pf = PixelFormat.getByteBgraPreInstance();
        PixelWriter pw = img.getPixelWriter();
        pw.setPixels(0, 0, w, h, pf, byteBuffer, w * 4);
        return img;
    }

    private WritableImage createWImageFromPixelBuffer(int w, int h, Color c) {
        ByteBuffer byteBuffer = createBuffer(w, h, c);
        PixelFormat<ByteBuffer> pf = PixelFormat.getByteBgraPreInstance();
        pb = new PixelBuffer(w, h, byteBuffer, pf);
        return new WritableImage(pb);
    }

    private void loadWritableImage() {
        final Image bImage = createWImageFromBuffer(imW, imH, Color.BLUE);
        ImageView bIv = new ImageView(bImage);
        final TextField tf = new TextField("100");
        Button updateWritableImage = new Button("Update WritableImage");
        updateWritableImage.setOnAction(e -> {
            int numIter = Integer.parseInt(tf.getText());

            double t1 = System.nanoTime();
            for (int i = 0; i < numIter; i++) {
                updateDirectByteBuffer(byteBuffer);
                PixelWriter pw = ((WritableImage) bImage).getPixelWriter();
                PixelFormat<ByteBuffer> pf = PixelFormat.getByteBgraPreInstance();
                pw.setPixels(0, 0, cpyW, cpyH, pf, byteBuffer, cpyW * 4);
            }
            double t2 = System.nanoTime();
            double t3 = t2 - t1;
            System.out.println("WI: AVERAGE time to update the Image: [" + t3/(long)numIter + "]nano sec,  [" + (t3 / 1000000.0)/(double)numIter + "]ms");
        });
        wImgContainer.getChildren().addAll(updateWritableImage, tf, bIv);
    }

    private void loadPBImage() {
        Image pbImage = createWImageFromPixelBuffer(imW, imH, Color.BLUE);
        ImageView pbIv = new ImageView(pbImage);

        final TextField tf = new TextField("100");

        Button updatePixelBuffer = new Button("Update PixelBuffer");
        updatePixelBuffer.setOnAction(e -> {
            int numIter = Integer.parseInt(tf.getText());

            double t1 = System.nanoTime();
            for (int i = 0; i < numIter; i++) {
                pb.updateBuffer(pixBuf -> {
                    ByteBuffer buf = ((PixelBuffer<ByteBuffer>) pixBuf).getBuffer();
                    updateDirectByteBuffer(buf);
                    return new Rectangle2D(0, 0, cpyW, cpyH);
                });
            }

            double t2 = System.nanoTime();
            double t3 = t2 - t1;
            System.out.println("PB: AVERAGE time to update the Image: [" + t3/(long)numIter + "]nano sec,  [" + (t3 / 1000000.0)/(double)numIter + "]ms");

        });
        pbImgContainer.getChildren().addAll(updatePixelBuffer, tf, pbIv);
    }

    @Override public void start(Stage pbImageStage) {
        colors.add(Color.GREEN); colors.add(Color.BLUE); colors.add(Color.ORANGE);

        createCopyBuffers();
        VBox pbRoot = new VBox(12);
        Scene pbScene = new Scene(pbRoot, scW, scH);
        loadPBImage();
        pbRoot.getChildren().add(pbImgContainer);
        pbImageStage.setScene(pbScene);
        pbImageStage.setTitle("PixelBuffer");
        pbImageStage.setX(10); pbImageStage.setY(10);
        pbImageStage.show();

        VBox wImRoot = new VBox(12);
        Scene wImScene = new Scene(wImRoot, scW, scH);
        loadWritableImage();
        wImRoot.getChildren().add(wImgContainer);

        Stage wImStage = new Stage();
        wImStage.setScene(wImScene);
        wImStage.setTitle("WritableImage-PixelWriter");

        wImStage.setX(scW + 50); wImStage.setY(10);
        wImStage.show();
    }

    public static void main(String[] args) {
        Application.launch(args);
    }
}