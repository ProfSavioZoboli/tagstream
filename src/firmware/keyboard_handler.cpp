#include "keyboard_handler.h"
#include <Arduino.h>





// --- Configuração do Hardware ---

const byte ROWS = 4; // Quatro linhas
const byte COLS = 4; // Quatro colunas

// Mapa de caracteres correspondente à matriz do teclado
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Pinos do Arduino conectados às linhas e colunas
byte rowPins[ROWS] = {35, 32, 33, 25}; // Pinos para L4, L3, L2, L1
byte colPins[COLS] = {26, 27, 14, 12}; // Pinos para C4, C3, C2, C1

// --- Implementação das Funções ---

// Função para configurar os pinos do teclado
void setupTeclado() {
  // Configura os pinos das linhas como saída
  for (int r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH); // Garante que todas as linhas comecem desativadas (em HIGH)
  }

  // Configura os pinos das colunas como entrada com resistor de pull-up interno
  for (int c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }
}

char getTeclaPressionada() {
  // Itera por cada linha
  for (int r = 0; r < ROWS; r++) {
    // Ativa a linha atual
    digitalWrite(rowPins[r], LOW);

    // DEBUG: Informa qual linha está sendo varrida
    Serial.print("Varrendo Linha "); Serial.print(r);
    Serial.print(" (Pino "); Serial.print(rowPins[r]); Serial.println(")");

    // Itera por cada coluna
    for (int c = 0; c < COLS; c++) {
      int estadoColuna = digitalRead(colPins[c]);
      
      // DEBUG: Mostra o estado de cada coluna
      Serial.print("  - Lendo Coluna "); Serial.print(c);
      Serial.print(" (Pino "); Serial.print(colPins[c]);
      Serial.print("): Estado = "); Serial.println(estadoColuna);

      if (estadoColuna == LOW) {
        Serial.println("!!! TECLA DETECTADA !!!"); // DEBUG
        delay(50); 
        digitalWrite(rowPins[r], HIGH);
        return keys[r][c];
      }
    }

    // Desativa a linha atual
    digitalWrite(rowPins[r], HIGH);
    Serial.println("--------------------"); // Separador para a próxima varredura
  }

  return '\0'; 
}