package org.example.motorvelocidad;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.IOException;

public class AplicacionMotor extends Application {
    private ControladorMotor controlador;

    @Override
    public void start(Stage stage) throws IOException {
        FXMLLoader fxmlLoader = new FXMLLoader(AplicacionMotor.class.getResource("motor-GUI.fxml"));
        Scene scene = new Scene(fxmlLoader.load());
        controlador = fxmlLoader.getController();

        stage.setTitle("Gráfica de motor");
        stage.setResizable(false);
        stage.setScene(scene);
        stage.setOnCloseRequest(event -> {
            if (controlador != null) {
                controlador.close();
            }
        });
        stage.show();
    }

    @Override
    public void stop() throws Exception {
        if (controlador != null) {
            controlador.close();
        }
        super.stop();
    }

    public static void main(String[] args) {
        launch();
    }
}