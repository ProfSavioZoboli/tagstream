// Arquivo: mqtt_handler.h

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void setup_wifi();
void reconnect_mqtt();
void syncListaUsuarios();
void callback(char* topic, byte* payload, unsigned int length);
bool hexStringToByteArray(const char* hexStr, byte* byteArray, int arraySize);

#endif