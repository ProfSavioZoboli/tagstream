// Arquivo: feedback_handler.h

#ifndef FEEDBACK_HANDLER_H
#define FEEDBACK_HANDLER_H

#include <Arduino.h> // Necessário para tipos como String

void configLedsEstado();
void sinalSonoro(int rounds, int tom);
void showMensagem(String mensagem);

#endif