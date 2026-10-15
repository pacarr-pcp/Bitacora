// Log SIMET v2 - Prueba de matrices LED 8x8 (4 módulos MAX7219 en cadena)
// Placa: NodeMCU 1.0 (ESP-12E Module)
//
// Conexiones (SPI por hardware):
//   Matriz 1 -> NodeMCU:  VCC -> VIN (5V)   GND -> GND
//                         DIN -> D7         CLK -> D5        CS -> D4
//   Matrices 2, 3 y 4: DOUT de cada una -> DIN de la siguiente; CLK, CS, VCC y GND en paralelo.
//
// Librería: "MD_Parola" (Gestor de bibliotecas; instala también MD_MAX72XX).

#include <SPI.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

// Si el texto sale espejado, al revés o desordenado, prueba otro tipo:
// FC16_HW, GENERIC_HW, PAROLA_HW o ICSTATION_HW
#define TIPO_MODULO MD_MAX72XX::FC16_HW
#define NUM_MODULOS 4
#define PIN_CS      D4
#define BRILLO      2        // 0 (mínimo) a 15 (máximo). Bajo = menos consumo.

MD_Parola pantalla = MD_Parola(TIPO_MODULO, PIN_CS, NUM_MODULOS);

void setup() {
  Serial.begin(115200);
  pantalla.begin();
  pantalla.setIntensity(BRILLO);
  pantalla.displayClear();

  // Prueba 1: enciende todos los LED durante 2 segundos
  MD_MAX72XX* mx = pantalla.getGraphicObject();
  mx->clear();
  for (uint8_t fila = 0; fila < 8; fila++)
    for (uint16_t col = 0; col < NUM_MODULOS * 8; col++)
      mx->setPoint(fila, col, true);
  mx->update();
  Serial.println("Prueba 1: todos los LED encendidos");
  delay(2000);
  mx->clear();

  // Prueba 2: texto que se desplaza
  pantalla.displayScroll("LOG SIMET - PRUEBA", PA_LEFT, PA_SCROLL_LEFT, 40);
  Serial.println("Prueba 2: texto en movimiento");
}

void loop() {
  if (pantalla.displayAnimate()) {
    pantalla.displayReset();   // repite el mensaje
  }
}
