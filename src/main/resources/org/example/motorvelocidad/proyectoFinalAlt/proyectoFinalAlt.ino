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
    procesarEntradaNumerica(entrada.toFloat());
  } else {
    procesarEntradaTexto(entrada);
  }
}

bool esNumero(String str) {
  bool puntoEncontrado = false;
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (!isDigit(c) && c != '.' && !(i == 0 && (c == '-' || c == '+'))) {
      return false;
    }
    if (c == '.') {
      if (puntoEncontrado) return false;
      puntoEncontrado = true;
    }
  }
  return true;
}

void procesarEntradaNumerica(float valor) {
  switch (modoControlActual) {
    case LAZO_ABIERTO:
      establecerRPM((int)valor);  // Convertir RPM a entero, pues no admite decimales
      break;

    case LAZO_CERRADO:
      if (rpmPermitido) {
        establecerRPM((int)valor);  // RPM sigue siendo entero
        calcularPID();
      } else {
        switch (constanteElegida) {
          case KP:
            kp = valor;
            Serial.print("KP establecido en: ");
            Serial.println(kp, 3);
            rpmPermitido = true;
            break;

          case KI:
            ki = valor;
            Serial.print("KI establecido en: ");
            Serial.println(ki, 3);
            rpmPermitido = true;
            break;

          case KD:
            kd = valor;
            Serial.print("KD establecido en: ");
            Serial.println(kd, 3);
            rpmPermitido = true;
            break;
        }
      }
      break;
  }
}


void establecerRPM(int nuevoRPM) {
  if (abs(nuevoRPM) <= RPM_MAX) {
    if (nuevoRPM == 0) {
      // Si el RPM requerido es 0, detener el motor y reiniciar los errores
      RPMrequerido = 0;
      errorAcumulado = 0;
      errorAnterior = 0;
      PID = 0;
      analogWrite(PIN_ENABLE_A, 0);
      analogWrite(PIN_ENABLE_B, 0);
      Serial.println("RPM = 0, motor detenido y errores reseteados.");
    } else {
      // Si se establece un nuevo valor de RPM, actualizar normalmente
      if (RPMrequerido == 0) {
        // Reiniciar errores al cambiar de 0 a un nuevo valor
        errorAcumulado = 0;
        errorAnterior = 0;
      }
      RPMrequerido = nuevoRPM;
      Serial.print("Nuevo RPM requerido: ");
      Serial.println(RPMrequerido);
    }
  }
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

  // Aplicar banda muerta
  if (abs(error) < BANDA_MUERTA) {
    PID = 0;
    errorAcumulado = 0;
    return;
  }

  // Proporcional
  sumaP = error * kp;

  // Integral con anti-windup
  errorAcumulado += error;
  sumaI = errorAcumulado * ki;
  if (sumaI > LIMITE_INTEGRAL) {
    sumaI = LIMITE_INTEGRAL;
    errorAcumulado = LIMITE_INTEGRAL / ki;
  } else if (sumaI < -LIMITE_INTEGRAL) {
    sumaI = -LIMITE_INTEGRAL;
    errorAcumulado = -LIMITE_INTEGRAL / ki;
  }

  // Derivativo
  long errorDt = error - errorAnterior;
  sumaD = errorDt * kd;

  // Resultado final del PID
  PID = sumaP + sumaI + sumaD;

  // Actualizar error anterior
  errorAnterior = error;
}

void aplicarControlMotor() {
  if (modoControlActual == LAZO_CERRADO) {
    pwmValor = constrain(PID, -255, 255);
  } else if (modoControlActual == LAZO_ABIERTO) {
    pwmValor = map(RPMrequerido, -RPM_MAX, RPM_MAX, -255, 255);
  } else {
    pwmValor = 0;
  }

  // Evitar cambios bruscos de dirección
  if ((ultimaDireccion > 0 && pwmValor < -HISTERESIS) ||
      (ultimaDireccion < 0 && pwmValor > HISTERESIS)) {
    pwmValor = 0;
    return;
  }
  // Actualizar la dirección del motor
  if (pwmValor > 0) {
    analogWrite(PIN_ENABLE_A, pwmValor);
    analogWrite(PIN_ENABLE_B, 0);
    ultimaDireccion = 1;
  } else if (pwmValor < 0) {
    analogWrite(PIN_ENABLE_A, 0);
    analogWrite(PIN_ENABLE_B, abs(pwmValor));
    ultimaDireccion = -1;
  } else {
    analogWrite(PIN_ENABLE_A, 0);
    analogWrite(PIN_ENABLE_B, 0);
    ultimaDireccion = 0;
  }
}

void actualizarRPM() {
  tiempoActual = millis() - tiempoRestado;
  if ((tiempoActual - tiempoAnterior) >= INTERVALO_MUESTREO) {
    tiempoAnterior = tiempoActual;
    float rpmCalculado = ((contadorPulsos / (float)PULSOS_POR_REVOLUCION) * (60000.0 / INTERVALO_MUESTREO));
    RPM = lowPassFilter(rpmCalculado, RPM, 0.1); // Suaviza la señal
    Serial.print(tiempoActual);
    Serial.print('\t');
    Serial.println(RPM);
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