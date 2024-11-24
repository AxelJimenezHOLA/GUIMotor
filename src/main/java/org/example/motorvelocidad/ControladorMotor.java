package org.example.motorvelocidad;

import com.fazecast.jSerialComm.SerialPort;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.XYChart;
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

    // Buffer de string
    private final StringBuilder dataBuffer = new StringBuilder();

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

        // Inicializar la gráfica
        XYChart.Series<Number, Number> series = new XYChart.Series<>();
        series.setName("RPM");
        funcionTransferenciaChart.getData().add(series);

        // Leyendas de los colores
        ColoredOutput.colorPrintln("Mensajes en verde = Cadenas mandadas al Serial", ColoredOutput.ANSIColor.GREEN);
        ColoredOutput.colorPrintln("Mensajes en cian = Cadenas regresadas del Arduino", ColoredOutput.ANSIColor.CYAN);
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
        arduinoPort.setBaudRate(9600);
        arduinoPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 0, 0);

        if (arduinoPort.openPort()) {
            System.out.println("Puerto abierto exitosamente.");
            realimentacionLabel.setDisable(false);
            ingresarRPMLabel.setDisable(false);
            rpmTextField.setDisable(false);
            realimentacionElegidaComboBox.setDisable(false);

            // Iniciar un hilo para leer datos del Arduino
            new Thread(this::leerDatosArduino).start();
        } else {
            System.out.println("No se pudo abrir el puerto.");
        }
    }

    @FXML private void onRealimentacionElegida() {
        if (arduinoPort == null || !arduinoPort.isOpen()) {
            System.out.println("El puerto no está abierto.");
            return;
        }

        String comando = realimentacionElegidaComboBox.getValue().equals("Lazo Abierto") ? COMANDO_LAZO_ABIERTO : COMANDO_LAZO_CERRADO;
        enviarComando(comando);

        boolean estaEnLazoCerrado = comando.equals(COMANDO_LAZO_CERRADO);
        kpLabel.setDisable(!estaEnLazoCerrado);
        kiLabel.setDisable(!estaEnLazoCerrado);
        kdLabel.setDisable(!estaEnLazoCerrado);
        kpTextField.setDisable(!estaEnLazoCerrado);
        kiTextField.setDisable(!estaEnLazoCerrado);
        kdTextField.setDisable(!estaEnLazoCerrado);
    }

    private void leerDatosArduino() {
        while (arduinoPort.isOpen()) {
            try {
                if (arduinoPort.bytesAvailable() > 0) {
                    byte[] readBuffer = new byte[arduinoPort.bytesAvailable()];
                    int numRead = arduinoPort.readBytes(readBuffer, readBuffer.length);
                    String datos = new String(readBuffer, 0, numRead);
                    dataBuffer.append(datos);

                    int endIndex;
                    while ((endIndex = dataBuffer.indexOf("\n")) != -1) {
                        String linea = dataBuffer.substring(0, endIndex);
                        procesarDatosArduino(linea);
                        dataBuffer.delete(0, endIndex + 1);
                    }
                }
                Thread.sleep(20);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private void enviarComando(String comando) {
        if (arduinoPort != null && arduinoPort.isOpen()) {
            arduinoPort.writeBytes(comando.getBytes(), comando.length());
            ColoredOutput.colorPrintln(comando.trim(), ColoredOutput.ANSIColor.GREEN);
        } else {
            System.out.println("No se pudo enviar el comando. El puerto no está abierto.");
        }
    }

    private void enviarConstante(String comando, String valor) {
        enviarComando(comando);
        enviarComando(valor + "\n");
    }

    @FXML private void onKpEnviado() {
        enviarConstante(COMANDO_KP, kpTextField.getText());
        kpTextField.setText("");
    }

    @FXML private void onKiEnviado() {
        enviarConstante(COMANDO_KI, kiTextField.getText());
        kiTextField.setText("");
    }

    @FXML private void onKdEnviado() {
        enviarConstante(COMANDO_KD, kdTextField.getText());
        kdTextField.setText("");
    }

    @FXML private void onRPMEnviado() {
        enviarComando(rpmTextField.getText() + "\n");
        rpmTextField.setText("");
    }

    private void procesarDatosArduino(String datos) {
        String[] lineas = datos.split("\n");
        for (String linea : lineas) {
            linea = linea.trim();
            if (linea.isEmpty()) continue;

            if (linea.contains("\t")) {
                // Datos de RPM y tiempo
                procesarDatosRPM(linea);
            } else if (linea.startsWith("RPM nuevo:")) {
                // Confirmación de cambio de RPM
                ColoredOutput.colorPrintln(linea, ColoredOutput.ANSIColor.CYAN);
            } else if (linea.startsWith("Kp nuevo:") || linea.startsWith("Ki nuevo:") || linea.startsWith("Kd nuevo:")) {
                // Confirmación de cambio de constantes PID
                ColoredOutput.colorPrintln(linea, ColoredOutput.ANSIColor.CYAN);
            } else if (linea.equals("Lazo abierto elegido") || linea.equals("Lazo cerrado elegido")) {
                // Confirmación de cambio de modo de control
                ColoredOutput.colorPrintln(linea, ColoredOutput.ANSIColor.CYAN);
            } else {
                // Otros mensajes del Arduino
                ColoredOutput.colorPrintln(linea, ColoredOutput.ANSIColor.CYAN);
            }
        }
    }

    private void procesarDatosRPM(String linea) {
        String[] partes = linea.split("\t");
        if (partes.length == 2) {
            try {
                long tiempo = Long.parseLong(partes[0]);
                int rpm = Integer.parseInt(partes[1]);
                actualizarGrafica(tiempo, rpm);
            } catch (NumberFormatException e) {
                System.out.println("Error al parsear datos de RPM: " + linea);
            }
        }
    }

    private void actualizarGrafica(long tiempo, int rpm) {
        javafx.application.Platform.runLater(() -> {
            XYChart.Series<Number, Number> series = funcionTransferenciaChart.getData().getFirst();
            series.getData().add(new XYChart.Data<>(tiempo, rpm));

            // Limitar el número de puntos en la gráfica para evitar sobrecarga
            if (series.getData().size() > 100) {
                series.getData().removeFirst();
            }

            rpmMedidosLabel.setText(rpm + " RPM");
        });
    }

    public void close() {
        if (arduinoPort != null && arduinoPort.isOpen()) {
            arduinoPort.closePort();
        }
        System.out.println("Puerto cerrado.");
    }
}