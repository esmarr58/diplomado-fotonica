/*
  Colorímetro didáctico guiado por Serial
  LED blanco + LDR + filtros manuales R, G, B

  Comandos:
  c -> calibrar
  m -> medir muestra
  v -> ver calibración
  r -> reiniciar calibración

  Compatible:
  - ESP32 / ESP32-S3
  - Arduino Uno
*/

#if defined(ARDUINO_ARCH_ESP32)
  #define BOARD_ESP32
#endif

#if defined(BOARD_ESP32)

  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    const int LDR_PIN = 4;
    const int LED_PIN = 2;
  #else
    const int LDR_PIN = 34;
    const int LED_PIN = 2;
  #endif

  const float ADC_MAX = 4095.0;

#else

  const int LDR_PIN = A0;
  const int LED_PIN = 9;
  const float ADC_MAX = 1023.0;

#endif

const int NUM_MUESTRAS = 50;
const int TIEMPO_ENTRE_MUESTRAS_MS = 10;

enum Canal {
  ROJO = 0,
  VERDE = 1,
  AZUL = 2,
  SIN_FILTRO = 3
};

const char* nombreCanal[4] = {
  "ROJO",
  "VERDE",
  "AZUL",
  "SIN FILTRO"
};

float valorMin[4];
float valorMax[4];

bool sistemaCalibrado = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

#if defined(BOARD_ESP32)
  analogReadResolution(12);
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  reiniciarCalibracion();

  Serial.println();
  Serial.println("====================================");
  Serial.println(" COLORIMETRO DIDACTICO GUIADO");
  Serial.println(" LED blanco + LDR + filtros RGB");
  Serial.println("====================================");
  mostrarMenu();
}

void loop() {
  if (Serial.available()) {
    char comando = Serial.read();

    while (Serial.available()) {
      Serial.read();
    }

    if (comando == 'c' || comando == 'C') {
      calibrarSistema();
    }

    else if (comando == 'm' || comando == 'M') {
      medirMuestra();
    }

    else if (comando == 'v' || comando == 'V') {
      mostrarCalibracion();
    }

    else if (comando == 'r' || comando == 'R') {
      reiniciarCalibracion();
      Serial.println("Calibracion reiniciada.");
    }

    mostrarMenu();
  }
}

void mostrarMenu() {
  Serial.println();
  Serial.println("Comandos disponibles:");
  Serial.println("c = calibrar sistema");
  Serial.println("m = medir muestra");
  Serial.println("v = ver calibracion");
  Serial.println("r = reiniciar calibracion");
  Serial.println();
  Serial.print("Ingrese comando: ");
}

void esperarEnter() {
  while (Serial.available()) {
    Serial.read();
  }

  while (!Serial.available()) {
    delay(10);
  }

  while (Serial.available()) {
    Serial.read();
  }
}

float leerPromedioLDR() {
  long suma = 0;

  for (int i = 0; i < NUM_MUESTRAS; i++) {
    suma += analogRead(LDR_PIN);
    delay(TIEMPO_ENTRE_MUESTRAS_MS);
  }

  return suma / float(NUM_MUESTRAS);
}

void reiniciarCalibracion() {
  for (int i = 0; i < 4; i++) {
    valorMin[i] = 0;
    valorMax[i] = ADC_MAX;
  }

  sistemaCalibrado = false;
}

void calibrarSistema() {
  Serial.println();
  Serial.println("====================================");
  Serial.println(" INICIO DE CALIBRACION");
  Serial.println("====================================");

  Serial.println();
  Serial.println("Paso 1: mediciones con LED APAGADO");
  digitalWrite(LED_PIN, LOW);
  delay(500);

  for (int i = 0; i < 4; i++) {
    Serial.println();
    Serial.print("Coloque filtro: ");
    Serial.println(nombreCanal[i]);
    Serial.println("Presione ENTER para medir...");
    esperarEnter();

    valorMin[i] = leerPromedioLDR();

    Serial.print("Valor minimo ");
    Serial.print(nombreCanal[i]);
    Serial.print(" = ");
    Serial.println(valorMin[i]);
  }

  Serial.println();
  Serial.println("Paso 2: mediciones con LED ENCENDIDO");
  digitalWrite(LED_PIN, HIGH);
  delay(500);

  for (int i = 0; i < 4; i++) {
    Serial.println();
    Serial.print("Coloque filtro: ");
    Serial.println(nombreCanal[i]);
    Serial.println("Presione ENTER para medir...");
    esperarEnter();

    valorMax[i] = leerPromedioLDR();

    Serial.print("Valor maximo ");
    Serial.print(nombreCanal[i]);
    Serial.print(" = ");
    Serial.println(valorMax[i]);
  }

  digitalWrite(LED_PIN, LOW);

  sistemaCalibrado = true;

  Serial.println();
  Serial.println("Calibracion terminada.");
  mostrarCalibracion();
}

int normalizarRGB(float lectura, int canal) {
  float minimo = valorMin[canal];
  float maximo = valorMax[canal];

  if (maximo == minimo) {
    return 0;
  }

  float valor = (lectura - minimo) * 255.0 / (maximo - minimo);

  if (valor < 0) valor = 0;
  if (valor > 255) valor = 255;

  return int(valor + 0.5);
}

void medirMuestra() {
  if (!sistemaCalibrado) {
    Serial.println();
    Serial.println("ERROR: primero debe calibrar el sistema con comando c.");
    return;
  }

  Serial.println();
  Serial.println("====================================");
  Serial.println(" MEDICION DE MUESTRA");
  Serial.println("====================================");

  int rgb[3];

  digitalWrite(LED_PIN, HIGH);
  delay(500);

  for (int i = 0; i < 3; i++) {
    Serial.println();
    Serial.print("Coloque el filtro ");
    Serial.print(nombreCanal[i]);
    Serial.println(" frente a la LDR.");

    Serial.println("Coloque tambien la muestra a medir.");
    Serial.println("Presione ENTER para medir...");
    esperarEnter();

    float lectura = leerPromedioLDR();
    rgb[i] = normalizarRGB(lectura, i);

    Serial.print("Lectura ADC ");
    Serial.print(nombreCanal[i]);
    Serial.print(" = ");
    Serial.println(lectura);

    Serial.print("Valor normalizado ");
    Serial.print(nombreCanal[i]);
    Serial.print(" = ");
    Serial.println(rgb[i]);
  }

  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.println("Resultado estimado:");
  Serial.print("RGB(");
  Serial.print(rgb[ROJO]);
  Serial.print(", ");
  Serial.print(rgb[VERDE]);
  Serial.print(", ");
  Serial.print(rgb[AZUL]);
  Serial.println(")");

  Serial.println();

  clasificarColor(rgb[ROJO], rgb[VERDE], rgb[AZUL]);
}

void mostrarCalibracion() {
  Serial.println();
  Serial.println("====================================");
  Serial.println(" VALORES DE CALIBRACION");
  Serial.println("====================================");

  for (int i = 0; i < 4; i++) {
    Serial.print(nombreCanal[i]);
    Serial.print(" | min = ");
    Serial.print(valorMin[i]);
    Serial.print(" | max = ");
    Serial.println(valorMax[i]);
  }

  Serial.print("Estado: ");
  Serial.println(sistemaCalibrado ? "calibrado" : "no calibrado");
}

void clasificarColor(int r, int g, int b) {
  Serial.print("Interpretacion simple: ");

  if (r < 40 && g < 40 && b < 40) {
    Serial.println("negro / muy oscuro");
  }

  else if (r > 210 && g > 210 && b > 210) {
    Serial.println("blanco / muy claro"); 
  }

  else if (r > g && r > b) {
    Serial.println("dominante rojo");
  }

  else if (g > r && g > b) {
    Serial.println("dominante verde");
  }

  else if (b > r && b > g) {
    Serial.println("dominante azul");
  }

  else {
    Serial.println("color mixto o ambiguo");
  }
}
