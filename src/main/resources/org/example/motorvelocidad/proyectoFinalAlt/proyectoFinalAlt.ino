// Constantes
const int PIN_ENCODER_A = 2;
const int PIN_ENCODER_B = 3;
const int PIN_ENABLE_A = 10;
const int PIN_ENABLE_B = 11;
const int PIN_PWM_SALIDA = 6;
const long PULSOS_POR_REVOLUCION = 647;
const int INTERVALO_MUESTREO = 15;  // ms
const int RPM_MAX = 1100;
const float FACTOR_VOLTAJE = 78.572;

// Enums
enum ModoControl {
  LAZO_ABIERTO,
  LAZO_CERRADO
};

enum ConstanteElegida {
  KP,
  KI,
  KD
};

// Variables globales
volatile int contadorPulsos = 0;
volatile byte estadoAnterior = 0;
volatile byte estadoActual = 0;

// Variables de tiempo
unsigned long tiempoActual = 0;
unsigned long tiempoAnterior = 0;
unsigned long tiempoRestado = 0;

// Variables de control
ModoControl modoControlActual = LAZO_ABIERTO;
ConstanteElegida constanteElegida = KP;
long kp = 0, ki = 0, kd = 0;
long sumaP = 0, sumaI = 0, sumaD = 0;
long errorAnterior = 0, errorAcumulado = 0;
long PID = 0;

// Variables de entrada y salida
volatile int RPMrequerido = 0;
long RPM = 0;
int pwmValor = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Programa de Arduino iniciado");
  inicializarPines();
}


void loop() {
  leerEntradaSerial();
  calcularPID();
  aplicarControlMotor();
  actualizarRPM();
}

void inicializarPines() {
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  pinMode(PIN_ENABLE_A, OUTPUT);
  pinMode(PIN_ENABLE_B, OUTPUT);
  pinMode(PIN_PWM_SALIDA, OUTPUT);
  digitalWrite(PIN_ENABLE_B, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), contarPulsos, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), contarPulsos, CHANGE);
}

void leerEntradaSerial() {
  if (Serial.available() <= 0) return;

  String entrada = Serial.readStringUntil('\n');
  entrada.trim();
  if (entrada.length() <= 0) return;

  if (esNumero(entrada)) {
    procesarEntradaNumerica(entrada.toInt());
  } else {
    procesarEntradaTexto(entrada);
  }
}

bool esNumero(String str) {
  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i)) && !(i == 0 && (str.charAt(i) == '-' || str.charAt(i) == '+'))) {
      return false;
    }
  }
  return true;
}

void procesarEntradaNumerica(int valor) {
  switch (modoControlActual) {
    case LAZO_ABIERTO:
      establecerRPM(valor);
      break;

    case LAZO_CERRADO:
      switch (constanteElegida) {
        case KP:
          kp = valor;
          break;

        case KI:
          ki = valor;
          break;

        case KD:
          kd = valor;
          break;
      }
  }
}

void establecerRPM(int nuevoRPM) {
  if (abs(nuevoRPM) <= RPM_MAX) RPMrequerido = nuevoRPM;
}

void procesarEntradaTexto(String comando) {
  if (comando.equals("lc")) {
    modoControlActual = LAZO_CERRADO;
  } else if (comando.equals("la")) {
    modoControlActual = LAZO_ABIERTO;
  } else if (comando.equals("kp")) {
    constanteElegida = KP;
  } else if (comando.equals("ki")) {
    constanteElegida = KI;
  } else if (comando.equals("kd")) {
    constanteElegida = KD;
  }
}

void calcularPID() {
  long error = RPMrequerido - RPM;
  sumaP = error * kp;
  errorAcumulado += error;
  sumaI = errorAcumulado * ki;
  long errorDt = error - errorAnterior;
  sumaD = errorDt * kd;
  PID = sumaP + sumaI + sumaD;
  errorAnterior = error;
}

void aplicarControlMotor() {
  if (modoControlActual == LAZO_ABIERTO) {
    pwmValor = map(RPMrequerido, -RPM_MAX, RPM_MAX, -255, 255);
  } else {
    pwmValor = constrain(PID, -255, 255);
  }

  if (pwmValor > 0) {
    analogWrite(PIN_ENABLE_A, pwmValor);
    analogWrite(PIN_ENABLE_B, 0);
  } else if (pwmValor < 0) {
    analogWrite(PIN_ENABLE_A, 0);
    analogWrite(PIN_ENABLE_B, -pwmValor);
  } else {
    analogWrite(PIN_ENABLE_A, 0);
    analogWrite(PIN_ENABLE_B, 0);
  }
}

void actualizarRPM() {
  tiempoActual = millis() - tiempoRestado;
  if ((tiempoActual - tiempoAnterior) >= INTERVALO_MUESTREO) {
    tiempoAnterior = tiempoActual;
    RPM = ((contadorPulsos / (float)PULSOS_POR_REVOLUCION) * (60000.0 / INTERVALO_MUESTREO));
    if (RPM != 0) {
      Serial.print(tiempoActual);
      Serial.print('\t');
      Serial.println(RPM);
    } else {
      tiempoRestado = millis();
    }
    contadorPulsos = 0;
  }
}

void contarPulsos() {
  estadoAnterior = estadoActual;
  estadoActual = (digitalRead(PIN_ENCODER_B) << 1) | digitalRead(PIN_ENCODER_A);

  if ((estadoAnterior == 0 && estadoActual == 1) || (estadoAnterior == 1 && estadoActual == 3) || (estadoAnterior == 3 && estadoActual == 2) || (estadoAnterior == 2 && estadoActual == 0)) {
    contadorPulsos++;
  } else if ((estadoAnterior == 0 && estadoActual == 2) || (estadoAnterior == 2 && estadoActual == 3) || (estadoAnterior == 3 && estadoActual == 1) || (estadoAnterior == 1 && estadoActual == 0)) {
    contadorPulsos--;
  }
}
