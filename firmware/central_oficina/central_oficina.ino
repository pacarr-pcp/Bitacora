// Log SIMET v2 - Central de la oficina
// Placa: NodeMCU 1.0 (ESP-12E Module)
//
// Cada 30 s consulta a la app si hay una llamada desde portería. Si la hay:
//   - suena el buzzer durante 3 s
//   - las matrices muestran "PORTERIA" hasta que se presione el pulsador
// Al presionar el pulsador avisa a la app que la llamada fue atendida.
//
// Conexiones:
//   Matrices MAX7219 (4 en cadena): DIN -> D7, CLK -> D5, CS -> D4, VCC -> VIN, GND -> GND
//   Buzzer activo 5 V: transistor NPN, base -> D6 (con 1 kΩ), buzzer entre VIN y colector
//   Pulsador: entre D3 y GND  (no presionarlo mientras se enciende la placa)
//
// Antes de subir: copia secrets.example.h como secrets.h y completa el Wi-Fi.
// Librerías: MD_Parola (instala también MD_MAX72XX). Soporte de placa: esp8266.

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include "secrets.h"

// --- Matrices ---
#define TIPO_MODULO MD_MAX72XX::FC16_HW   // el mismo que funcionó en la prueba
#define NUM_MODULOS 4
#define PIN_CS      D4
#define BRILLO      3                     // 0 a 15

// --- Pines ---
#define PIN_BUZZER  D6
#define PIN_BOTON   D3

// --- Tiempos ---
const unsigned long INTERVALO_CONSULTA_MS = 30000;  // cada cuánto se pregunta a la app
const unsigned long DURACION_PITIDO_MS    = 3000;   // pitido inicial
const unsigned long ANTIREBOTE_MS         = 50;

MD_Parola pantalla = MD_Parola(TIPO_MODULO, PIN_CS, NUM_MODULOS);

enum Estado { REPOSO, ALARMA };
Estado estado = REPOSO;

String idActual = "";          // llamada que está sonando
String ultimoIdAtendido = "";  // para no volver a sonar por la misma llamada
unsigned long ultimaConsulta = 0;
unsigned long finPitido = 0;

// ---------------------------------------------------------------------------
// Red
// ---------------------------------------------------------------------------

void conectarWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando a Wi-Fi");
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " conectado" : " sin conexión");
}

// Hace un GET a la app y devuelve el texto de respuesta ("" si falla).
String pedir(const String& accion) {
  if (WiFi.status() != WL_CONNECTED) return "";
  WiFiClientSecure cliente;
  cliente.setInsecure();                       // sin validar el certificado (simple)
  cliente.setBufferSizes(1024, 512);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Apps Script redirige
  http.setTimeout(10000);
  String respuesta = "";
  if (http.begin(cliente, String(WEBAPP_URL) + "?accion=" + accion)) {
    int codigo = http.GET();
    if (codigo == 200) respuesta = http.getString();
    else Serial.printf("HTTP %d\n", codigo);
    http.end();
  }
  respuesta.trim();
  return respuesta;
}

void avisarAtendida(const String& id) {
  String r = pedir("atender&id=" + id);
  Serial.println(r == "OK" ? "Llamada marcada como atendida" : "No se pudo avisar (se reintentará)");
}

// ---------------------------------------------------------------------------
// Alarma
// ---------------------------------------------------------------------------

void iniciarAlarma(const String& id) {
  idActual = id;
  estado = ALARMA;
  finPitido = millis() + DURACION_PITIDO_MS;
  digitalWrite(PIN_BUZZER, HIGH);
  pantalla.displayClear();
  pantalla.displayScroll("PORTERIA", PA_LEFT, PA_SCROLL_LEFT, 40);
  Serial.println("ALARMA: llamada de portería " + id);
}

void detenerAlarma() {
  digitalWrite(PIN_BUZZER, LOW);
  pantalla.displayClear();
  estado = REPOSO;
  ultimoIdAtendido = idActual;
  Serial.println("Alarma detenida");
  avisarAtendida(idActual);
}

bool botonPresionado() {
  if (digitalRead(PIN_BOTON) != LOW) return false;
  delay(ANTIREBOTE_MS);
  return digitalRead(PIN_BOTON) == LOW;
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_BOTON, INPUT_PULLUP);

  pantalla.begin();
  pantalla.setIntensity(BRILLO);
  pantalla.displayClear();

  conectarWifi();
  ultimaConsulta = millis() - INTERVALO_CONSULTA_MS;  // consulta de inmediato al partir
}

void loop() {
  if (estado == REPOSO) {
    if (millis() - ultimaConsulta >= INTERVALO_CONSULTA_MS) {
      ultimaConsulta = millis();
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Sin Wi-Fi, reintentando...");
        WiFi.reconnect();
      } else {
        String r = pedir("estado");
        Serial.println("Estado: " + r);
        if (r.startsWith("SI:")) {
          String id = r.substring(3);
          if (id == ultimoIdAtendido) avisarAtendida(id);  // el aviso anterior no llegó
          else iniciarAlarma(id);
        }
      }
    }
  } else {  // ALARMA
    if (digitalRead(PIN_BUZZER) == HIGH && millis() >= finPitido) digitalWrite(PIN_BUZZER, LOW);
    if (pantalla.displayAnimate()) pantalla.displayReset();  // repite el texto
    if (botonPresionado()) detenerAlarma();
  }
}
