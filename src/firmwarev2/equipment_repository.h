#ifndef EQUIPMENT_REPOSITORY_H
#define EQUIPMENT_REPOSITORY_H

#include <Arduino.h>
#include "models.h"

// Inicializa e carrega inventário da Flash.
// Retorna true se carregou dados existentes, false se iniciou vazio/padrão.
bool equipRepoInit();

// Salva o estado atual de todo o inventário na Flash
void equipRepoSave();

// Retorna o status atual de um equipamento
SituacaoEquipamento equipRepoGetStatus(int numero);

// Atualiza o status e, opcionalmente, salva na flash imediatamente.
// Retorna false se o número do equipamento for inválido.
bool equipRepoUpdateStatus(int numero, SituacaoEquipamento novaSituacao, bool salvarAgora = true);

// Sobrescreve todo o inventário (usado na sincronização MQTT)
void equipRepoUpdateAll(const Equipamento* novoInventario);

void equipRepoFactoryReset();

#endif