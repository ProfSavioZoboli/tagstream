#include <arduino.h>
#include "emprestimos.h"
#include "config.h"
#include <string>
#include <ArduinoJson.h>


typedef struct Equipamento_ {
  int numero;
  SituacaoEquipamento situacao;
} Equipamento;


Equipamento inventario[MAX_EQUIPAMENTOS];

int getNumeroEquipamento(int indice){
  return inventario[indice].numero;
}

SituacaoEquipamento getSituacaoEquipamento(int indice){
  return inventario[indice].situacao;
}


bool verificaDisponibilidade(int num_inicial, int num_final) {
  if (num_inicial > 0 && num_inicial < MAX_EQUIPAMENTOS && num_final > 0 && num_final < MAX_EQUIPAMENTOS) {
    for (int i = num_inicial; i <= num_final; i++) {
      Serial.print("O equipamento ");
      Serial.print(i);
      Serial.print(" está ");
      Serial.print(inventario[i].situacao);
      if (inventario[i].situacao != SituacaoEquipamento::EQP_LIVRE) {
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
      if (inventario[i].situacao != SituacaoEquipamento::EQP_OCUPADO && inventario[i].situacao != SituacaoEquipamento::EQP_MANUTENCAO) {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

void pegaEmprestado(int equipamento) {
  Serial.print("Equipamento");
  Serial.print(equipamento);
  Serial.print("foi emprestado");
  inventario[equipamento].situacao = SituacaoEquipamento::EQP_OCUPADO;
}

void devolver(int equipamento){
  Serial.print("Equipamento");
  Serial.print(equipamento);
  Serial.print("foi devolvido");
  inventario[equipamento].situacao = SituacaoEquipamento::EQP_LIVRE;
}

bool handleEmprestimo(char* query) {
  Serial.print("Recebeu a query: ");
  Serial.println(query);
  int primeiro;
  int ultimo;
  std::string str_query = query;
  if (str_query.find('A') != -1) {
    int index_a = str_query.find('A');
    primeiro = std::stoi(str_query.substr(0, index_a));
    ultimo = std::stoi(str_query.substr(index_a + 1));
  } else {
    primeiro = std::stoi(str_query);
    ultimo = primeiro;
  }
  Serial.print("Primeiro equipamento:");
  Serial.println(primeiro);
  Serial.print("Ultimo equipamento: ");
  Serial.println(ultimo);
  if (verificaDisponibilidade(primeiro, ultimo)) {
    for (int i = primeiro; i <= ultimo; i++) {
      pegaEmprestado(i);
    }
    return true;
  }else{
    return false;
  }
}

bool handleDevolucao(char* query){
  Serial.print("Recebeu a query: ");
  Serial.println(query);
  int primeiro;
  int ultimo;
  std::string str_query = query;
  if (str_query.find('A') != -1) {
    int index_a = str_query.find('A');
    primeiro = std::stoi(str_query.substr(0, index_a));
    ultimo = std::stoi(str_query.substr(index_a + 1));
  } else {
    primeiro = std::stoi(str_query);
    ultimo = primeiro;
  }
  Serial.print("Primeiro equipamento:");
  Serial.println(primeiro);
  Serial.print("Ultimo equipamento: ");
  Serial.println(ultimo);
  if (verificaIndisponibilidade(primeiro, ultimo)) {
    for (int i = primeiro; i <= ultimo; i++) {
      devolver(i);
    }
    return true;
  }else{
    return false;
  }
}

void atualizarInventario(JsonArray equipamentosArray) {
  Serial.println("Atualizando inventario de equipamentos a partir do array JSON...");

  // Opcional: Limpar o estado de todos os equipamentos antes de atualizar
  // for (int i = 0; i < MAX_EQUIPAMENTOS; i++) {
  //   inventario[i].situacao = LIVRE; // Ou outro estado padrão
  // }

  for (JsonObject equipamento : equipamentosArray) {
    if (!equipamento.containsKey("id") || !equipamento.containsKey("status")) {
      Serial.println("Aviso: Objeto de equipamento no array esta incompleto. Ignorando.");
      continue;  // Pula para o próximo item do array
    }

    int id = equipamento["id"];
    const char* situacaoStr = equipamento["status"];

    if (id > 0 && id < MAX_EQUIPAMENTOS) {
      inventario[id].numero = id;

      if (strcmp(situacaoStr, "OCUPADO") == 0) {
        Serial.print("O equipamento ");
        Serial.print(id);
        Serial.println(" está ocupado");
        inventario[id].situacao = SituacaoEquipamento::EQP_OCUPADO;
      } else if (strcmp(situacaoStr, "MANUTENCAO") == 0) {
        Serial.print("O equipamento ");
        Serial.print(id);
        Serial.println(" está em manutenção");
        inventario[id].situacao = SituacaoEquipamento::EQP_MANUTENCAO;
      } else {
        Serial.print("O equipamento ");
        Serial.print(id);
        Serial.println(" está livre");
        inventario[id].situacao = SituacaoEquipamento::EQP_LIVRE;
      }
    } else {
      Serial.print("Aviso: ID de equipamento invalido no array: ");
      Serial.println(id);
    }
  }

  //listarEquipamentos();
}

