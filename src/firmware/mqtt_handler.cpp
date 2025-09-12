// Arquivo: mqtt_handler.cpp

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <chrono>
#include "mqtt_handler.h"
#include "config.h"
#include "secrets.h"
#include "rfid_handler.h"  // Para ter acesso à struct Usuario

// Variáveis globais do .ino que este arquivo precisa conhecer
extern PubSubClient client;
extern Usuario usuariosAutorizados[MAX_TAGS];
extern int numeroDeTagsAutorizadas;
extern unsigned long long timestampDaListaLocal;
extern unsigned long long lastSync;
extern enum DadosEstado { DESATUALIZADO,
                          SUJO,
                          SINCRONIZADO,
                          SINCRONIZANDO } dadosEstadoAtual;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  float counter = 1.0;  // Mude para float
  while (WiFi.status() != WL_CONNECTED) {
    delay(500 * counter);
    counter += 0.1;
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    // Conecta usando um ID de cliente, usuário e senha
    if (client.connect("ESP32_RFID_Client_01", mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");
      // Reinscreve no tópico após a reconexão
      client.subscribe(mqtt_topic_res_usrs);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      /*
             * Códigos de erro do PubSubClient:
             * -4 : Falha de conexão (ex: timeout)
             * -2 : ID de cliente inválido
             * 1 : Versão de protocolo incorreta
             * 2 : ID de cliente rejeitado
             * 3 : Servidor indisponível
             * 4 : Usuário ou senha incorretos
             * 5 : Não autorizado
            */
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void syncListaUsuarios() {
  dadosEstadoAtual = DadosEstado::SINCRONIZANDO;
  client.publish(mqtt_topic_req_usrs, "");
  Serial.print("MQTT: Enviado requisicao no topico ");
  Serial.print(mqtt_topic_req_usrs);
  Serial.println(" para atualizacao da lista de usuarios");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  if (strcmp(topic, mqtt_topic_res_usrs) == 0) {
    Serial.println("Mensagem de usuários recebida. Processando...");

    // 1. Parsear o JSON recebido
    StaticJsonDocument<1024> doc;  // Aumente se a lista for muito grande
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
      Serial.print("Falha ao parsear JSON: ");
      Serial.println(error.c_str());
      dadosEstadoAtual = DadosEstado::DESATUALIZADO;
      return;
    }

    // 2. Extrair o timestamp e o objeto aninhado 'users'
    unsigned long long novoTimestamp = doc["last_updated"];
    JsonObject users = doc["users"];

    // 3. Lógica de Sincronização: verificar se os dados são realmente novos
    if (novoTimestamp <= timestampDaListaLocal) {
      Serial.println("Dados recebidos não são novos. Atualização ignorada.");
      dadosEstadoAtual = DadosEstado::SINCRONIZADO;
      auto now = std::chrono::system_clock::now();
      auto duration = now.time_since_epoch();
      lastSync = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

      return;
    }

    // 4. Verificar se o objeto 'users' realmente existe no JSON
    if (users.isNull()) {
      Serial.println("Erro: O JSON recebido não contém o objeto 'users'.");
      dadosEstadoAtual = DadosEstado::DESATUALIZADO;
      return;
    }

    // 5. Se os dados são novos, limpar a lista antiga e processar a nova
    Serial.println("Dados novos detectados. Atualizando a lista de usuários...");
    numeroDeTagsAutorizadas = 0;

    // Itera sobre cada par chave:valor DENTRO do objeto 'users'
    for (JsonPair kv : users) {
      if (numeroDeTagsAutorizadas >= MAX_TAGS) {
        Serial.println("Aviso: Número máximo de tags atingido. Alguns usuários foram ignorados.");
        break;
      }

      // O resto da lógica é idêntico ao que você já tinha, mas agora dentro deste novo contexto
      const char* uidHex = kv.key().c_str();
      JsonObject userData = kv.value().as<JsonObject>();

      const char* nome = userData["usuario"];
      int nivel = userData["nivel_acesso"];

      // Converte o UID e copia os dados para nossa struct
      hexStringToByteArray(uidHex, usuariosAutorizados[numeroDeTagsAutorizadas].uid, TAG_LENGTH);
      strncpy(usuariosAutorizados[numeroDeTagsAutorizadas].nome, nome, sizeof(usuariosAutorizados[0].nome) - 1);
      usuariosAutorizados[numeroDeTagsAutorizadas].nome[sizeof(usuariosAutorizados[0].nome) - 1] = '\0';  // Garante terminação nula
      usuariosAutorizados[numeroDeTagsAutorizadas].nivelAcesso = nivel;

      numeroDeTagsAutorizadas++;
    }

    // 6. Se a atualização foi bem-sucedida, guardar o novo timestamp
    timestampDaListaLocal = novoTimestamp;
    dadosEstadoAtual = DadosEstado::SINCRONIZADO;
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    lastSync = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    Serial.print("Lista de usuários atualizada com sucesso. Novo timestamp local: ");
    Serial.println(timestampDaListaLocal);
  }
}

bool hexStringToByteArray(const char* hexStr, byte* byteArray, int arraySize) {
  int len = strlen(hexStr);
  if (len != arraySize * 2) return false;

  for (int i = 0; i < arraySize; i++) {
    char hexPair[3] = { hexStr[i * 2], hexStr[i * 2 + 1], '\0' };
    byteArray[i] = (byte)strtol(hexPair, NULL, 16);
  }
  return true;
}