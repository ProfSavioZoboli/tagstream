#ifndef EMPRESTIMOS_H
#define EMPRESTIMOS_H

#include <ArduinoJson.h>
#include "config.h"
#include "models.h"

extern Equipamento inventario[MAX_EQUIPAMENTOS];

int getNumeroEquipamento(int indice);

SituacaoEquipamento getSituacaoEquipamento(int indice);

void atualizarInventario(JsonArray equipamentos);

void listarEquipamentos();

bool handleEmprestimo(char* query);

bool handleDevolucao(char* query);

int getQtdEquipamentos();





#endif