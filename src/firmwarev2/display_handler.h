#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <Arduino.h>

// Inicializa o display OLED. Retorna true se sucesso.
bool setupDisplay();

// Exibe uma mensagem fixa no centro do ecrã.
// 'update_last_msg': se true, guarda esta mensagem como a "padrão" para retornar após mensagens temporárias.
void displayShowMensagem(String mensagem, bool update_last_msg = true);

// Exibe uma mensagem por um tempo determinado e depois retorna à mensagem anterior.
void displayShowTempMensagem(String mensagem, int ms);

// Limpa o ecrã
void displayClear();

#endif