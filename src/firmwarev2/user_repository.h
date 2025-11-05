#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <Arduino.h>
#include "models.h"

// Inicializa o repositório e carrega dados da Flash
void userRepoInit();

// Salva os dados atuais na Flash
void userRepoSave();

// Adiciona um usuário à lista (se houver espaço).
// Retorna true se sucesso, false se lista cheia.
bool userRepoAdd(const char* uidHex, const char* nome);

// Limpa todos os usuários da memória (não salva automaticamente na flash, 
// chame userRepoSave depois se quiser persistir a limpeza)
void userRepoClear();

// Busca um usuário pelo UID (bytes). Retorna ponteiro ou nullptr se não achar.
Usuario* userRepoFind(byte* uid, byte uidSize);

// Retorna a quantidade atual de usuários
int userRepoGetCount();

// Retorna o timestamp da última atualização da lista local
unsigned long long userRepoGetTimestamp();

// Atualiza o timestamp da lista
void userRepoSetTimestamp(unsigned long long newTimestamp);

void userRepoFactoryReset();

#endif