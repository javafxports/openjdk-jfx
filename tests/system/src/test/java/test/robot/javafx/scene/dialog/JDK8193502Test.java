package test.robot.javafx.scene.dialog;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.ButtonType;
import javafx.scene.control.Dialog;
import javafx.scene.control.Label;
import javafx.scene.robot.Robot;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import javafx.stage.WindowEvent;
import javafx.scene.input.MouseButton;

import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import test.util.Util;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class JDK8193502Test {
	static final double DIALOG_WIDTH = 300.0d;
	static final double DIALOG_HEIGHT = DIALOG_WIDTH;
	static Robot robot;
	static Button button;
	static Button dialogButton;
	static Stage stage;
	static Scene scene;
	static Dialog<Void> dialog;
	static CountDownLatch startupLatch = new CountDownLatch(1);
	static CountDownLatch dialogShownLatch;
	static CountDownLatch dialogHideLatch;

	@Test(timeout = 15000)
	public void dialogSizeOnReShownTest() throws Exception {
		Thread.sleep(400);
		clickButton();
		hide();
		clickButton();
		Assert.assertEquals("Dialog width should remain the same", DIALOG_WIDTH, dialog.getDialogPane().getWidth(),
				0.0);
		Assert.assertEquals("Dialog height should remain the same", DIALOG_HEIGHT, dialog.getDialogPane().getHeight(),
				0.0);
		hide();
	}

	private void clickButton() throws Exception {
		dialogShownLatch = new CountDownLatch(1);
		mouseClick(button.getLayoutX() + button.getWidth() / 2, button.getLayoutY() + button.getHeight() / 2);

		Thread.sleep(400);
		waitForLatch(dialogShownLatch, 10, "Failed to show Dialog");
	}

	private void hide() throws Exception {
		dialogHideLatch = new CountDownLatch(1);
		Platform.runLater(() -> dialog.close());
		Thread.sleep(600);
		waitForLatch(dialogHideLatch, 10, "Failed to hide Dialog");
	}

	@BeforeClass
	public static void initFX() throws Exception {
		new Thread(() -> Application.launch(TestApp.class, (String[]) null)).start();
		waitForLatch(startupLatch, 10, "FX runtime failed to start.");
	}

	@AfterClass
	public static void exit() {
		Platform.runLater(() -> stage.hide());
		Platform.exit();
	}

	private void mouseClick(double x, double y) {
		Util.runAndWait(() -> {
			robot.mouseMove((int) (scene.getWindow().getX() + scene.getX() + x),
					(int) (scene.getWindow().getY() + scene.getY() + y));
			robot.mousePress(MouseButton.PRIMARY);
			robot.mouseRelease(MouseButton.PRIMARY);
		});
	}

	public static class TestApp extends Application {
		@Override
		public void start(Stage primaryStage) {
			robot = new Robot();
			stage = primaryStage;

			dialog = getTestDialog();
			button = new Button("Open Dialog");
			button.setOnAction(evt -> dialog.show());

			scene = new Scene(button, 200, 200);
			stage.setScene(scene);

			stage.initStyle(StageStyle.UNDECORATED);
			stage.addEventHandler(WindowEvent.WINDOW_SHOWN, e -> Platform.runLater(startupLatch::countDown));

			stage.show();
		}

		private static Dialog<Void> getTestDialog() {
			final Label dialogContent = new Label();
			dialogContent.setText("Dialog content");

			final Dialog<Void> testDialog = new Dialog<>();

			testDialog.getDialogPane().setContent(dialogContent);
			testDialog.getDialogPane().getButtonTypes().add(ButtonType.CLOSE);
			testDialog.getDialogPane().setPrefSize(DIALOG_WIDTH, DIALOG_HEIGHT);

			dialogButton = (Button) testDialog.getDialogPane().lookupButton(ButtonType.CLOSE);

			testDialog.getDialogPane().getScene().getWindow().addEventHandler(WindowEvent.WINDOW_SHOWN,
					e -> Platform.runLater(dialogShownLatch::countDown));

			testDialog.getDialogPane().getScene().getWindow().addEventHandler(WindowEvent.WINDOW_HIDDEN,
					e -> Platform.runLater(dialogHideLatch::countDown));

			return testDialog;
		}
	}

	public static void waitForLatch(CountDownLatch latch, int seconds, String msg) throws Exception {
		org.junit.Assert.assertTrue("Timeout: " + msg, latch.await(seconds, TimeUnit.SECONDS));
	}
}