#ifndef EMPRESTIMOS_H
#define EMPRESTIMOS_H

#include <ArduinoJson.h>

typedef struct Equipamento_ Equipamento;

void atualizarInventario(JsonArray equipamentos);

void listarEquipamentos();

bool handleEmprestimo(char* query);




#endif