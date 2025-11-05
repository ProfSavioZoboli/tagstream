#ifndef LOG_REPOSITORY_H
#define LOG_REPOSITORY_H

#include <Arduino.h>
#include "models.h"

struct LogEntry {
    byte userUid[TAG_LENGTH];
    int equipId;
    SituacaoEquipamento novoStatus;
    unsigned long long timestamp;
};

void logRepoInit();

// Adiciona na fila
bool logRepoEnqueue(byte* userUid, int equipId, SituacaoEquipamento status, unsigned long long timestamp);

// Retorna quantos temos para enviar
int logRepoCountPending();

// Retorna um log específico da fila (0 = mais antigo) sem remover.
// Retorna false se o índice for inválido.
bool logRepoPeek(int index, LogEntry* output);

// Limpa toda a fila (chamar apenas após confirmação de envio do lote)
void logRepoClear();

void logRepoFactoryReset();

#endif