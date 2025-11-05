#include <Arduino.h>
#include "emprestimos.h"
#include "config.h"
#include <string>
#include <ArduinoJson.h>
#include "equipment_repository.h"
#include "utils.h"
#include "mqtt_handler.h" // Para enviar o log
#include "rfid_handler.h" // Para saber quem é o usuário
#include "log_repository.h"

// REMOVIDO: Equipamento inventario[MAX_EQUIPAMENTOS];

int getNumeroEquipamento(int indice){
  return indice; 
}

SituacaoEquipamento getSituacaoEquipamento(int indice){
  return equipRepoGetStatus(indice);
}

bool verificaDisponibilidade(int num_inicial, int num_final) {
  if (num_inicial > 0 && num_inicial < MAX_EQUIPAMENTOS && num_final > 0 && num_final < MAX_EQUIPAMENTOS) {
    for (int i = num_inicial; i <= num_final; i++) {
      SituacaoEquipamento sit = equipRepoGetStatus(i);
      Serial.print("O equipamento ");
      Serial.print(i);
      Serial.print(" esta ");
      // Serial.print(sit); // Necessitaria de conversão para string legível se quiser debug bonito
      if (sit != EQP_LIVRE) {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

bool verificaIndisponibilidade(int num_inicial, int num_final){
  if (num_inicial > 0 && num_inicial < MAX_EQUIPAMENTOS && num_final > 0 && num_final < MAX_EQUIPAMENTOS) {
    for (int i = num_inicial; i <= num_final; i++) {
      SituacaoEquipamento sit = equipRepoGetStatus(i);
      if (sit != EQP_OCUPADO && sit != EQP_MANUTENCAO) {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

void pegaEmprestado(int equipamento) {
  Serial.print("Equipamento "); Serial.print(equipamento); Serial.println(" foi emprestado");
  equipRepoUpdateStatus(equipamento, EQP_OCUPADO, true);

  // ENFILEIRA O LOG
  Usuario* u = getUsuarioAtual();
  if (u != nullptr) {
      logRepoEnqueue(u->uid, equipamento, EQP_OCUPADO, getTimestampAtual());
  }
}

void devolver(int equipamento){
  Serial.print("Equipamento "); Serial.print(equipamento); Serial.println(" foi devolvido");
  equipRepoUpdateStatus(equipamento, EQP_LIVRE, true);

  // ENFILEIRA O LOG
  Usuario* u = getUsuarioAtual();
  if (u != nullptr) {
      logRepoEnqueue(u->uid, equipamento, EQP_LIVRE, getTimestampAtual());
  }
}


void atualizarInventario(JsonArray equipamentosArray) {
  Serial.println("Atualizando inventario via JSON...");
  
  for (JsonObject equipamento : equipamentosArray) {
    if (!equipamento.containsKey("id") || !equipamento.containsKey("status")) continue;

    int id = equipamento["id"];
    const char* situacaoStr = equipamento["status"];

    if (id > 0 && id < MAX_EQUIPAMENTOS) {
      SituacaoEquipamento novaSituacao = EQP_LIVRE;
      if (strcmp(situacaoStr, "OCUPADO") == 0) novaSituacao = EQP_OCUPADO;
      else if (strcmp(situacaoStr, "MANUTENCAO") == 0) novaSituacao = EQP_MANUTENCAO;
      
      // Atualiza na memória sem salvar a cada iteração (false)
      equipRepoUpdateStatus(id, novaSituacao, false);
    }
  }
  // Salva tudo de uma vez no final para economizar ciclos de escrita na Flash
  equipRepoSave();
  Serial.println("Inventario atualizado e salvo.");
}

// Adicione isto ao final de firmwarev2/emprestimos.cpp

bool handleEmprestimo(char* query) {
    int start = atoi(query);
    // Procura pelo separador 'A'. Se não achar, o fim é igual ao início (apenas 1 item)
    char* separator = strchr(query, 'A');
    int end = (separator != nullptr) ? atoi(separator + 1) : start;

    // Validações de segurança
    if (start <= 0 || end >= MAX_EQUIPAMENTOS || start > end) {
        Serial.println("Emprestimo: Intervalo invalido.");
        return false;
    }

    // Passo 1: VERIFICAÇÃO (Política "Tudo ou Nada")
    for (int i = start; i <= end; i++) {
        if (equipRepoGetStatus(i) != EQP_LIVRE) {
            Serial.print("Emprestimo falhou: EQ "); Serial.print(i); Serial.println(" nao esta livre.");
            return false; // Se um estiver indisponível, cancela tudo
        }
    }

    // Passo 2: EXECUÇÃO
    for (int i = start; i <= end; i++) {
        pegaEmprestado(i);
    }
    return true;
}

bool handleDevolucao(char* query) {
    int start = atoi(query);
    char* separator = strchr(query, 'A');
    int end = (separator != nullptr) ? atoi(separator + 1) : start;

    if (start <= 0 || end >= MAX_EQUIPAMENTOS || start > end) return false;

    // Verifica se todos podem ser devolvidos (não estão livres)
    for (int i = start; i <= end; i++) {
        if (equipRepoGetStatus(i) == EQP_LIVRE) {
             Serial.print("Devolucao falhou: EQ "); Serial.print(i); Serial.println(" ja esta livre.");
             return false;
        }
    }

    // Executa devolução em lote
    for (int i = start; i <= end; i++) {
        devolver(i);
    }
    return true;
}

// Funções auxiliares de listagem e contagem também parecem estar faltando corpo se você as usa:
int getQtdEquipamentos() {
    return MAX_EQUIPAMENTOS;
}

void listarEquipamentos() {
    Serial.println("--- LISTA DE EQUIPAMENTOS ---");
    for (int i = 1; i < MAX_EQUIPAMENTOS; i++) {
        Serial.print("EQ ");
        Serial.print(i);
        Serial.print(": ");
        // situacaoParaString está em utils.h, certifique-se de incluir se usar aqui
        // Serial.println(situacaoParaString(equipRepoGetStatus(i))); 
        Serial.println(equipRepoGetStatus(i));
    }
    Serial.println("-----------------------------");
}