#include <WiFi.h>
#include "wifi_handler.h"
#include "secrets.h"
#include "config.h"  // Para acesso a configurações se necessário (ex: timeouts)
#include "time.h"

// Ajuste estes valores conforme necessário ou mova para config.h
const int WIFI_MAX_ATTEMPTS = 10;
const int WIFI_RECONNECT_INTERVAL = 5000;

static unsigned long last_wifi_check = 0;

const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;  // Se tiver horário de verão, ajuste para 3600
const char* ntpServer = "pool.ntp.org";

bool setupWiFi() {
  Serial.println();
  Serial.print("Conectando WiFi a: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);  // Garante modo estação
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi conectado! IP: ");
    Serial.println(WiFi.localIP());

    // --- INÍCIO NTP ---
    Serial.print("Configurando hora via NTP...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // Espera um pouco pela sincronização inicial (opcional, mas recomendado)
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 2000)) {  // Tenta por 2 segundos
      Serial.println(" (ainda nao sincronizou, seguira em background)"); 
    } else {
      Serial.println(" OK!");
    }
    // ------------------

    return true;
  } else {
    Serial.println("Falha na conexão WiFi.");
    return false;
  }
}

bool checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  unsigned long now = millis();
  if (now - last_wifi_check > WIFI_RECONNECT_INTERVAL) {
    last_wifi_check = now;
    Serial.println("WiFi desconectado. Tentando reconectar...");
    WiFi.disconnect();
    setupWiFi();
    // Não bloqueamos aqui para não travar o loop principal por muito tempo
  }
  return (WiFi.status() == WL_CONNECTED);
}

String getWiFiIP() {
  return WiFi.localIP().toString();
}