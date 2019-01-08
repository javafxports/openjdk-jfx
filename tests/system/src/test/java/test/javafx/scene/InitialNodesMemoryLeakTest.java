package test.javafx.scene;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Group;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.stage.WindowEvent;
import junit.framework.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import test.util.Util;

import java.lang.ref.WeakReference;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.fail;

public class InitialNodesMemoryLeakTest {

    static CountDownLatch startupLatch;
    static WeakReference<Group> toCleanUp;

    public static class TestApp extends Application {

        @Override
        public void start(Stage stage) throws Exception {
            toCleanUp = new WeakReference<>(new Group());

            Group root = new Group(toCleanUp.get());

            stage.setScene(new Scene(root));

            stage.addEventHandler(WindowEvent.WINDOW_SHOWN, e -> {
                Platform.runLater(() -> {
                    root.getChildren().clear();
                    startupLatch.countDown();
                });
            });

            stage.show();

        }
    }

    @BeforeClass
    public static void initFX() {
        startupLatch = new CountDownLatch(1);
        new Thread(() -> Application.launch(InitialNodesMemoryLeakTest.TestApp.class, (String[])null)).start();
        try {
            if (!startupLatch.await(15, TimeUnit.SECONDS)) {
                fail("Timeout waiting for FX runtime to start");
            }
        } catch (InterruptedException ex) {
            fail("Unexpected exception: " + ex);
        }
    }

    @Test
    public void testCleanup() throws Exception {
        System.out.println("got: " + toCleanUp.get());

        for (int j = 0; j < 10; j++) {
            System.gc();
            System.runFinalization();

            if (toCleanUp.get() == null) {
                break;
            }

            Util.sleep(500);
        }

        Assert.assertTrue("Couldn't collect Node", toCleanUp.get() == null);
    }
}
