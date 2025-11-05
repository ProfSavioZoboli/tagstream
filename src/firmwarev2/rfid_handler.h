// Arquivo: rfid_handler.h

#ifndef RFID_HANDLER_H
#define RFID_HANDLER_H

#include <Arduino.h> // Para o tipo 'byte'
#include "models.h"

Usuario* getUsuarioAtual();

// Declaração de funções
void setupRFID();
void readRFID();
void rejectUsuario();
void iniciaEmprestimo(Usuario* usuario);
void finalizaEmprestimo(Usuario* usuario);
Usuario* getUsuarioAutorizado(byte* uid, byte uidSize);
Usuario* getUsuario(byte* uid, byte uidSize);


#endif