#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include "models.h"

// Configurações iniciais do cliente MQTT
void setupMQTT();

// Deve ser chamado no loop principal para manter a conexão viva
void mqttLoop();

// Retorna true se está conectado ao broker
bool mqttIsConnected();

// Tenta reconectar (gerencia seus próprios timeouts internos)
bool reconnect_mqtt();

// Verifica se precisa sincronizar dados periodicamente
void checkMqttSync();

// --- Funções de Negócio ---
void syncListaUsuarios();
void sendOperacaoUsuario(Usuario* usuario, String operacao);
void syncListaEquipamentos();
void marcaDirty();
void newListaEquipamentos();

void flushLogQueue();

DadosEstado getEstadoAtual();

#endif