// Arquivo: feedback_handler.cpp

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "feedback_handler.h"
#include "config.h" // Para ter acesso aos pinos

// Declaração 'extern' para usar as variáveis globais do arquivo .ino
extern Adafruit_SSD1306 display;
extern enum SistemaEstado { OCIOSO, OCUPADO} sistemaEstadoAtual;

String msg_atual;

/*void configLedsEstado() {
  digitalWrite(LED_OCIOSO, LOW);
  digitalWrite(LED_OCUPADO, LOW);
  if (sistemaEstadoAtual == OCIOSO) {
    digitalWrite(LED_OCIOSO, HIGH);
  }
  if (sistemaEstadoAtual == OCUPADO) {
    digitalWrite(LED_OCUPADO, HIGH);
  }
}*/

void sinalSonoro(int rounds, int tom) {
  for (int i = 0; i < rounds; i++) {
    tone(BUZZER, tom, 100);
    delay(100);
  }
}

void showMensagem(String mensagem,bool update) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(mensagem, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
  display.println(mensagem);
  display.display();
  if(update){
    msg_atual = mensagem;
  }
}

void showTempMensagem(String mensagem, int ms){
  showMensagem(mensagem,false);
  delay(ms);
  showMensagem(msg_atual,false);
}