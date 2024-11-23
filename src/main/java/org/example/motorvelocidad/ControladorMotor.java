package org.example.motorvelocidad;

import com.fazecast.jSerialComm.SerialPort;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.chart.LineChart;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;

import java.util.ArrayList;

public class ControladorMotor {
    // Comandos de strings
    private final String COMANDO_LAZO_ABIERTO = "la\n";
    private final String COMANDO_LAZO_CERRADO = "lc\n";
    private final String COMANDO_KP = "kp\n";
    private final String COMANDO_KI = "ki\n";
    private final String COMANDO_KD = "kd\n";

    // Serial
    private SerialPort arduinoPort;

    // Labels
    @FXML private Label kpLabel;
    @FXML private Label kiLabel;
    @FXML private Label kdLabel;
    @FXML private Label realimentacionLabel;
    @FXML private Label ingresarRPMLabel;
    @FXML private Label rpmMedidosLabel;

    // Campos de texto
    @FXML private TextField kpTextField;
    @FXML private TextField kiTextField;
    @FXML private TextField kdTextField;
    @FXML private TextField rpmTextField;

    // Otros componentes
    @FXML private ComboBox<String> puertosDisponiblesComboBox;
    @FXML private ComboBox<String> realimentacionElegidaComboBox;
    @FXML private LineChart<Number, Number> funcionTransferenciaChart;

    @FXML public void initialize() {
        puertosDisponiblesComboBox.setItems(obtenerPuertosDisponibles());
        realimentacionElegidaComboBox.setItems(obtenerRealimentaciones());
    }

    private ObservableList<String> obtenerPuertosDisponibles() {
        ArrayList<String> nombresPuertos = new ArrayList<>();
        SerialPort[] puertosDisponibles = SerialPort.getCommPorts();

        for (SerialPort puerto : puertosDisponibles) {
            nombresPuertos.add(puerto.getSystemPortName());
        }

        return FXCollections.observableArrayList(nombresPuertos);
    }

    private ObservableList<String> obtenerRealimentaciones() {
        ArrayList<String> nombresRealimentaciones = new ArrayList<>();
        nombresRealimentaciones.add("Lazo Abierto");
        nombresRealimentaciones.add("Lazo Cerrado");
        return FXCollections.observableArrayList(nombresRealimentaciones);
    }

    // Funciones
    @FXML private void onPuertoSeleccionado() {
        arduinoPort = SerialPort.getCommPort(puertosDisponiblesComboBox.getValue());
        realimentacionLabel.setDisable(false);
        ingresarRPMLabel.setDisable(false);
        rpmTextField.setDisable(false);
        realimentacionElegidaComboBox.setDisable(false);
    }

    @FXML private void onRPMEnviado() {
        String textoEnviado = "%s\n".formatted(rpmTextField.getText());
        arduinoPort.writeBytes(COMANDO_LAZO_ABIERTO.getBytes(), COMANDO_LAZO_ABIERTO.length());
        arduinoPort.writeBytes(textoEnviado.getBytes(), textoEnviado.length());
        arduinoPort.writeBytes(COMANDO_LAZO_CERRADO.getBytes(), COMANDO_LAZO_CERRADO.length());
    }

    @FXML private void onKpEnviado() {
        String textoEnviado = "%s\n".formatted(kpTextField.getText());
        arduinoPort.writeBytes(COMANDO_KP.getBytes(), COMANDO_KP.length());
        arduinoPort.writeBytes(textoEnviado.getBytes(), textoEnviado.length());
    }

    @FXML private void onKiEnviado() {
        String textoEnviado = "%s\n".formatted(kiTextField.getText());
        arduinoPort.writeBytes(COMANDO_KI.getBytes(), COMANDO_KI.length());
        arduinoPort.writeBytes(textoEnviado.getBytes(), textoEnviado.length());
    }

    @FXML private void onKdEnviado() {
        String textoEnviado = "%s\n".formatted(kdTextField.getText());
        arduinoPort.writeBytes(COMANDO_KD.getBytes(), COMANDO_KD.length());
        arduinoPort.writeBytes(textoEnviado.getBytes(), textoEnviado.length());
    }

    @FXML private void onRealimentacionElegida() {
        switch (realimentacionElegidaComboBox.getValue()) {
            case "Lazo Abierto" -> {
                arduinoPort.writeBytes(COMANDO_LAZO_ABIERTO.getBytes(), COMANDO_LAZO_ABIERTO.length());
                kpLabel.setDisable(true);
                kiLabel.setDisable(true);
                kdLabel.setDisable(true);
                kpTextField.setDisable(true);
                kiTextField.setDisable(true);
                kdTextField.setDisable(true);
            }
            case "Lazo Cerrado" -> {
                arduinoPort.writeBytes(COMANDO_LAZO_CERRADO.getBytes(), COMANDO_LAZO_CERRADO.length());
                kpLabel.setDisable(false);
                kiLabel.setDisable(false);
                kdLabel.setDisable(false);
                kpTextField.setDisable(false);
                kiTextField.setDisable(false);
                kdTextField.setDisable(false);
            }
        }
    }
}