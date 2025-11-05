#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include "config.h" // Para TAG_LENGTH se necessário, ou defina aqui se for exclusivo do modelo

// --- ENUMS DE ESTADO DO SISTEMA ---
// Movido do firmware.ino
enum SistemaEstado {
    SIS_OCIOSO,
    SIS_OCUPADO
};

// Movido do firmware.ino
enum UsuarioEstado {
    NONE,
    AUTORIZADO,
    NAUTORIZADO
};

// Movido do firmware.ino
enum OperacaoSistema {
    OP_NENHUMA,
    OP_EMPRESTIMO,
    OP_DEVOLUCAO,
    OP_MANUTENCAO
};

// --- ENUMS DE DADOS ---
// Movido do mqtt_handler.h
enum DadosEstado {
    DESATUALIZADO,
    SUJO,
    SINCRONIZADO,
    SINCRONIZANDO
};

// Movido do emprestimos.h
enum SituacaoEquipamento {
    EQP_OCUPADO,
    EQP_LIVRE,
    EQP_MANUTENCAO
};

// --- ESTRUCTS DE DADOS ---
// Movido do rfid_handler.h
struct Usuario {
  byte uid[TAG_LENGTH]; 
  char nome[32];
  // Dica de parceiro: Em C++ puro, preferiríamos std::array<byte, 4> e std::string,
  // mas para ESP32/Arduino, manter byte[] e char[] é aceitável para simplicidade e uso de memória.
};

// Movido e unificado do emprestimos.h/.cpp
struct Equipamento {
  int numero;
  SituacaoEquipamento situacao;
};

#endif