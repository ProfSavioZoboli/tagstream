#include "display_handler.h"
#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Objeto display agora é privado deste módulo (static)
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variável interna para lembrar a última mensagem fixa
static String last_stable_message = "";

bool setupDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
        return false;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
    return true;
}

void displayShowMensagem(String mensagem, bool update_last_msg) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE); // Redundante se já definido no setup, mas seguro.

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(mensagem, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
    display.println(mensagem);
    display.display();

    if (update_last_msg) {
        last_stable_message = mensagem;
    }
}

void displayShowTempMensagem(String mensagem, int ms) {
    displayShowMensagem(mensagem, false); // Mostra sem atualizar a 'last_stable_message'
    delay(ms);
    displayShowMensagem(last_stable_message, false); // Restaura a anterior
}

void displayClear() {
    display.clearDisplay();
    display.display();
}