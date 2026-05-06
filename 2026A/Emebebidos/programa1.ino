/*
  Colorímetro didáctico con LDR + LED blanco + filtros manuales

  Comandos por Serial:
  1 + Enter -> graficar señal LDR original
  2 + Enter -> graficar señal filtrada con Kalman
  3 + Enter -> graficar original + Kalman
  4 + Enter -> graficar original + Kalman + promedio móvil

  Compatible:
  - Arduino Uno R3
  - ESP32-WROOM-32
  - ESP32-S3-WROOM-1
*/

#if defined(ARDUINO_ARCH_ESP32)
  #define BOARD_ESP32
#endif

// =======================
// Pines
// =======================
#if defined(BOARD_ESP32)

  #if CONFIG_IDF_TARGET_ESP32S3
    const int LDR_PIN = 4;       // ESP32-S3-WROOM-1
    const int LED_PIN = 2;
  #else
    const int LDR_PIN = 34;      // ESP32-WROOM-32
    const int LED_PIN = 2;
  #endif

  const float ADC_MAX = 4095.0;

#else

  const int LDR_PIN = A0;        // Arduino Uno R3
  const int LED_PIN = 9;
  const float ADC_MAX = 1023.0;

#endif

// =======================
// Variables generales
// =======================
int modoGrafica = 1;
const int muestras = 20;
const unsigned long Ts_ms = 50;

// =======================
// Kalman discreto 1D
// =======================
float x_est = 0.0;
float P = 1.0;

float Q = 0.00005;   // Ruido del proceso
float R = 0.00200;   // Ruido de medición

bool kalmanIniciado = false;

// =======================
// Promedio móvil
// =======================
float bufferPromedio[muestras];
int indicePromedio = 0;
bool bufferLleno = false;

// =======================
// Funciones
// =======================
float leerLDR() {
  int adc = analogRead(LDR_PIN);
  float valor = adc / ADC_MAX;

  if (valor < 0.0) valor = 0.0;
  if (valor > 1.0) valor = 1.0;

  return valor;
}

float filtroKalman(float z) {
  if (!kalmanIniciado) {
    x_est = z;
    P = 1.0;
    kalmanIniciado = true;
    return x_est;
  }

  // Predicción
  float x_pred = x_est;
  float P_pred = P + Q;

  // Ganancia de Kalman
  float K = P_pred / (P_pred + R);

  // Corrección
  x_est = x_pred + K * (z - x_pred);
  P = (1.0 - K) * P_pred;

  return x_est;
}

float promedioMovil(float nuevaMuestra) {
  bufferPromedio[indicePromedio] = nuevaMuestra;
  indicePromedio++;

  if (indicePromedio >= muestras) {
    indicePromedio = 0;
    bufferLleno = true;
  }

  int limite = bufferLleno ? muestras : indicePromedio;

  float suma = 0.0;

  for (int i = 0; i < limite; i++) {
    suma += bufferPromedio[i];
  }

  return suma / limite;
}

void procesarComandoSerial() {
  if (Serial.available()) {
    char comando = Serial.read();

    if (comando == '1') {
      modoGrafica = 1;
      Serial.println("Original");
    }

    else if (comando == '2') {
      modoGrafica = 2;
      Serial.println("Kalman");
    }

    else if (comando == '3') {
      modoGrafica = 3;
      Serial.println("Original Kalman");
    }

    else if (comando == '4') {
      modoGrafica = 4;
      Serial.println("Original Kalman Promedio");
    }

    while (Serial.available()) {
      Serial.read();   // limpia \n o \r
    }
  }
}

void imprimirGrafica(float original, float kalman, float promedio) {
  if (modoGrafica == 1) {
    Serial.print("Original:");
    Serial.println(original, 6);
  }

  else if (modoGrafica == 2) {
    Serial.print("Kalman:");
    Serial.println(kalman, 6);
  }

  else if (modoGrafica == 3) {
    Serial.print("Original:");
    Serial.print(original, 6);
    Serial.print("\t");

    Serial.print("Kalman:");
    Serial.println(kalman, 6);
  }

  else if (modoGrafica == 4) {
    Serial.print("Original:");
    Serial.print(original, 6);
    Serial.print("\t");

    Serial.print("Kalman:");
    Serial.print(kalman, 6);
    Serial.print("\t");

    Serial.print("Promedio:");
    Serial.println(promedio, 6);
  }
}

// =======================
// Setup
// =======================
void setup() {
  Serial.begin(115200);
  delay(1000);

#if defined(BOARD_ESP32)
  analogReadResolution(12);
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);   // LED blanco siempre encendido

  for (int i = 0; i < muestras; i++) {
    bufferPromedio[i] = 0.0;
  }

  Serial.println("Colorimetro LDR con LED blanco, filtros manuales y Kalman");
  Serial.println("Comandos:");
  Serial.println("1 = Original");
  Serial.println("2 = Kalman");
  Serial.println("3 = Original + Kalman");
  Serial.println("4 = Original + Kalman + Promedio");
}

// =======================
// Loop
// =======================
void loop() {
  procesarComandoSerial();

  float original = leerLDR();
  float kalman = filtroKalman(original);
  float promedio = promedioMovil(original);

  imprimirGrafica(original, kalman, promedio);

  delay(Ts_ms);
}
