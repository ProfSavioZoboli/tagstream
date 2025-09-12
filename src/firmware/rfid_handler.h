// Arquivo: rfid_handler.h

#ifndef RFID_HANDLER_H
#define RFID_HANDLER_H

#include <Arduino.h> // Para o tipo 'byte'

// A struct Usuario precisa ser conhecida aqui
struct Usuario {
  byte uid[4]; // Usar um valor fixo é mais seguro
  char nome[32];
  int nivelAcesso;
};

// Declaração de funções
void readRFID();
Usuario* getUsuarioAutorizado(byte* uid, byte uidSize);
Usuario* getUsuario(byte* uid, byte uidSize);

#endif