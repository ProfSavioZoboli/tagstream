// Arquivo: mqtt_handler.h

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include "rfid_handler.h"

enum DadosEstado { DESATUALIZADO,
                   SUJO,
                   SINCRONIZADO,
                   SINCRONIZANDO };

bool setup_wifi();
bool reconnect_mqtt();
void syncListaUsuarios();
void sendOperacaoUsuario(Usuario* usuario, String operacao);
void callback(char* topic, byte* payload, unsigned int length);
bool hexStringToByteArray(const char* hexStr, byte* byteArray, int arraySize);
void byteArrayToHexString(byte* byteArray, int arraySize, char* outputBuffer, int bufferSize);
void syncListaEquipamentos();
void marcaDirty();

DadosEstado getEstadoAtual();
void setEstadoAtual(DadosEstado estado);

#endif