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

//Variables anti histeresis
const int BANDA_MUERTA = 10; // RPM
const int HISTERESIS = 5; // RPM
int ultimaDireccion = 0; // -1 para negativo, 1 para positivo, 0 para detenido

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
float kp = 0, ki = 0, kd = 0;
float sumaP = 0, sumaI = 0, sumaD = 0;
float errorAnterior = 0, errorAcumulado = 0;
float PID = 0;

bool rpmPermitido=true;

// Variables de entrada y salida
volatile int RPMrequerido = 0;
volatile int RPM = 0;
int pwmValor = 0;
const long LIMITE_INTEGRAL = 255; // Limite para anti-windup

float lowPassFilter(float input, float output, float alpha) {
  return alpha * input + (1 - alpha) * output;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Programa de Arduino iniciado");
  inicializarPines();
}

void loop() {
  leerEntradaSerial();
  if (modoControlActual == LAZO_CERRADO) {
    calcularPID();
  }
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
    if(rpmPermitido){
      establecerRPM(valor);
      calcularPID();
    }else{
      switch (constanteElegida) {
        case KP:
          kp = valor;
          Serial.print("KP establecido en: ");
          Serial.println(kp);
          rpmPermitido=true;
          break;

        case KI:
          ki = valor;
          Serial.print("KI establecido en: ");
          Serial.println(ki);
          rpmPermitido=true;
          break;

        case KD:
          kd = valor;
          Serial.print("KD establecido en: ");
          Serial.println(kd);
          rpmPermitido=true;
          break;
      }
    }
    break;
  }
}

void establecerRPM(int nuevoRPM) {
  if (abs(nuevoRPM) <= RPM_MAX) RPMrequerido = nuevoRPM;
  Serial.println(RPMrequerido);
}

void procesarEntradaTexto(String comando) {
  if (comando.equals("lc")) {
    modoControlActual = LAZO_CERRADO;
    Serial.println("Modo de control: Lazo cerrado");
  } else if (comando.equals("la")) {
    modoControlActual = LAZO_ABIERTO;
    Serial.println("Modo de control: Lazo abierto");
  } else if (comando.equals("kp")) {
    constanteElegida = KP;
    rpmPermitido=false;
    Serial.println("Constante seleccionada: KP");
  } else if (comando.equals("ki")) {
    constanteElegida = KI;
    rpmPermitido=false;
    Serial.println("Constante seleccionada: KI");
  } else if (comando.equals("kd")) {
    constanteElegida = KD;
    rpmPermitido=false;
    Serial.println("Constante seleccionada: KD");
  }
}

void calcularPID() {
  long error = RPMrequerido - RPM;
  if (RPMrequerido == 0) {
    error = 0;
    errorAcumulado = 0;
    sumaP = 0;
    sumaI = 0;
    sumaD = 0;
  } else {
    sumaP = error * kp;
    // Aplicar anti-windup al término integral
    errorAcumulado += error;
    sumaI = errorAcumulado * ki;
    if (sumaI > LIMITE_INTEGRAL) {
      sumaI = LIMITE_INTEGRAL;
      errorAcumulado = LIMITE_INTEGRAL / ki;
    } else if (sumaI < -LIMITE_INTEGRAL) {
      sumaI = -LIMITE_INTEGRAL;
      errorAcumulado = -LIMITE_INTEGRAL / ki;
    }
    long errorDt = error - errorAnterior;
    sumaD = errorDt * kd;
  }
  PID = sumaP + sumaI + sumaD;
  errorAnterior = error;
}

void aplicarControlMotor() {
  // Revisar el modo de control
  if (modoControlActual == LAZO_CERRADO) {
    // En lazo cerrado, usar el valor de PID para calcular el PWM
    pwmValor = constrain(PID, -255, 255);
  } else if (modoControlActual == LAZO_ABIERTO) {
    // En lazo abierto, calcular el PWM según el RPM requerido directamente
    pwmValor = map(RPMrequerido, -RPM_MAX, RPM_MAX, -255, 255);
  } else {
    pwmValor = 0; // Apagar el motor si no hay un modo definido
  }

  // Aplicar el PWM al motor según su dirección
  if (pwmValor > 0) {
    // Movimiento en sentido positivo
    analogWrite(PIN_ENABLE_A, pwmValor);
    analogWrite(PIN_ENABLE_B, 0);
  } else if (pwmValor < 0) {
    // Movimiento en sentido negativo
    analogWrite(PIN_ENABLE_A, 0);
    analogWrite(PIN_ENABLE_B, -pwmValor);
  } else {
    // Detener el motor si el PWM es 0
    analogWrite(PIN_ENABLE_A, 0);
    analogWrite(PIN_ENABLE_B, 0);
  }
}



void actualizarRPM() {
  tiempoActual = millis() - tiempoRestado;
  if ((tiempoActual - tiempoAnterior) >= INTERVALO_MUESTREO) {
    tiempoAnterior = tiempoActual;
    RPM = ((contadorPulsos / (float)PULSOS_POR_REVOLUCION) * (60000.0 / INTERVALO_MUESTREO));
    if (RPM == 0) {
      tiempoRestado = millis();
    }
    else{
    Serial.print(tiempoActual);
    Serial.print('\t');
    Serial.println(RPM);
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