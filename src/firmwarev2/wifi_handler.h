#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <Arduino.h>

// Inicializa a conexão WiFi. Retorna true se conectado com sucesso após tentativas.
bool setupWiFi();

// Verifica se o WiFi ainda está conectado. Se não, tenta reconectar.
// Retorna true se estiver conectado.
bool checkWiFiConnection();

// Retorna o IP atual como String (útil para debug/display)
String getWiFiIP();

#endif