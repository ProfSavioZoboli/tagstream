#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mqtt_handler.h"
#include "config.h"
#include "secrets.h"
#include "wifi_handler.h"       // Novo: Para verificar conexão antes de tentar MQTT
#include "user_repository.h"    // Novo: Para manipular dados de usuários
#include "utils.h"
#include "emprestimos.h"
#include "log_repository.h"

// --- OBJETOS PRIVADOS (ENCAPSULADOS) ---
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

// --- ESTADO INTERNO DO MÓDULO MQTT ---
static DadosEstado dadosEstadoAtual = DESATUALIZADO;
static unsigned long long lastSync = 0;
static long long ultima_tentativa_reconnect = 0;

// Protótipo interno da callback (não precisa estar no .h)
void mqtt_callback(char* topic, byte* payload, unsigned int length);

void setupMQTT() {
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqtt_callback);
    // Aumentado para suportar listas maiores de JSON
    mqttClient.setBufferSize(2048); 
}

void mqttLoop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

bool mqttIsConnected() {
    return mqttClient.connected();
}

bool reconnect_mqtt() {
    // Se não tem WiFi, nem tenta conectar MQTT
    if (!checkWiFiConnection()) {
        return false;
    }

    long long now = getTimestampAtual();
    if (ultima_tentativa_reconnect != 0 && (now - ultima_tentativa_reconnect) < 5000) {
        // Evita flood de tentativas muito rápidas (5s intervalo)
        return false;
    }
    ultima_tentativa_reconnect = now;

    Serial.print("Tentando conectar ao MQTT...");
    // Tenta conectar com ID único
    if (mqttClient.connect("ESP32_RFID_Client_V2", mqtt_user, mqtt_pass)) {
        Serial.println("conectado!");
        // Reinscreve nos tópicos necessários
        mqttClient.subscribe(mqtt_topic_res_usrs);
        mqttClient.subscribe(mqtt_topic_res_eqps);
        mqttClient.subscribe(mqtt_topic_ack_eqps);
        mqttClient.subscribe(mqtt_topic_ack_usrs);
        return true;
    } else {
        Serial.print("falhou, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" (tentara novamente depois)");
        return false;
    }
}

void checkMqttSync() {
    if (!mqttClient.connected()) return;

    long long milis = getTimestampAtual();
    // Timeout de sincronização
    // Se estamos esperando há mais de 30 min, assume que falhou e tenta de novo.
    if ((milis - lastSync) > (1000 * 60 * 5) && dadosEstadoAtual == SINCRONIZANDO) {
         Serial.println("MQTT: Timeout de sincronizacao. Tentando novamente...");
         dadosEstadoAtual = DESATUALIZADO;
    }

    if (dadosEstadoAtual == DESATUALIZADO) {
        Serial.println("MQTT: Estado DESATUALIZADO. Solicitando dados...");
        syncListaUsuarios();
        newListaEquipamentos();
        dadosEstadoAtual = SINCRONIZANDO;
        lastSync = getTimestampAtual(); // <--- A CORREÇÃO CRÍTICA ESTÁ AQUI
    } 
    else if (dadosEstadoAtual == SUJO) {
        Serial.println("MQTT: Estado SUJO. Enviando atualizacao de equipamentos...");
        syncListaEquipamentos();
        dadosEstadoAtual = SINCRONIZANDO;
        lastSync = getTimestampAtual();
    }
}

void newListaEquipamentos(){
  mqttClient.publish(mqtt_topic_req_eqps,"");
  Serial.println("[MQTT] Solicitada nova lista de equipamentos");
}

void syncListaUsuarios() {
    // Check-sync usando o timestamp do repositório
    unsigned long long localTs = userRepoGetTimestamp();
    
    StaticJsonDocument<64> doc;
    doc["meu_timestamp"] = localTs;
    
    char jsonBuffer[64];
    serializeJson(doc, jsonBuffer);

    mqttClient.publish(mqtt_topic_check_usrs, jsonBuffer); 
    Serial.print("MQTT: Verificando usuarios. Timestamp local: ");
    Serial.println((unsigned long)localTs); // Cast para printar básico
}

void sendOperacaoUsuario(Usuario* usuario, String operacao) {
  if (usuario == nullptr) return;

  StaticJsonDocument<256> doc;
  JsonObject usuarioObj = doc.createNestedObject("usuario");
  
  char uidHex[TAG_LENGTH * 2 + 1];
  byteArrayToHexString(usuario->uid, TAG_LENGTH, uidHex, sizeof(uidHex));

  usuarioObj["uid"] = uidHex;
  usuarioObj["nome"] = usuario->nome;
  usuarioObj["operacao"] = operacao;
  // Adiciona timestamp da operação para registro preciso no servidor
  doc["timestamp"] = getTimestampAtual();

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  mqttClient.publish(mqtt_topic_log_usuarioLogado, jsonBuffer);
  Serial.print("MQTT Log enviado: ");
  Serial.println(operacao);
}


void flushLogQueue() {
    int qtd = logRepoCountPending();
    if (qtd == 0) return;

    if (!mqttClient.connected()) {
        Serial.println("MQTT: Sem conexao para enviar logs.");
        return;
    }

    Serial.print("MQTT: Preparando envio de lote com ");
    Serial.print(qtd);
    Serial.println(" logs...");

    // Calcula capacidade necessária do JSON Document
    // Ajuste este valor se começar a ter erros de memória (JSON muito grande)
    const size_t capacity = JSON_ARRAY_SIZE(qtd) + qtd * JSON_OBJECT_SIZE(5) + 200;
    DynamicJsonDocument doc(capacity);
    JsonArray array = doc.to<JsonArray>();

    // --- DECLARAÇÃO ÚNICA DAS VARIÁVEIS AUXILIARES ---
    LogEntry entry;
    char uidHex[TAG_LENGTH * 2 + 1];
    char timeStr[25]; 
    // -------------------------------------------------

    for (int i = 0; i < qtd; i++) {
        if (logRepoPeek(i, &entry)) {
            JsonObject obj = array.createNestedObject();
            
            byteArrayToHexString(entry.userUid, TAG_LENGTH, uidHex, sizeof(uidHex));
            obj["uid"] = uidHex;
            obj["eq"] = entry.equipId;
            obj["st"] = (int)entry.novoStatus;
            
            // Conversão de timestamp (milis) para string ISO
            // Nota: Se o evento ocorreu antes do NTP, a data será 1970.
            time_t rawTime = (time_t)(entry.timestamp / 1000);
            struct tm* timeinfo = localtime(&rawTime);
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%S", timeinfo);
            obj["ts"] = timeStr;
        }
    }

    String jsonOutput;
    serializeJson(array, jsonOutput);

    // Usando o mesmo tópico de movimentação para o lote
    if (mqttClient.publish(mqtt_topic_log_movimentacao, jsonOutput.c_str())) {
        Serial.println("MQTT: Lote enviado com sucesso!");
        logRepoClear(); 
    } else {
        Serial.println("MQTT: FALHA ao enviar lote (possivelmente muito grande para o buffer MQTT).");
    }
}

void syncListaEquipamentos() {
    // (Lógica mantida, apenas ajustada para usar mqttClient interno)
    const int JSON_DOC_SIZE = 2048;
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    JsonArray jsonArray = doc.to<JsonArray>();

    for (int i = 0; i < MAX_EQUIPAMENTOS; i++) {
      JsonObject eq = jsonArray.createNestedObject();
      eq["numero"] = getNumeroEquipamento(i);
      eq["situacao"] = getSituacaoEquipamento(i);
    }

    char jsonBuffer[JSON_DOC_SIZE];
    serializeJson(doc, jsonBuffer);
    mqttClient.publish(mqtt_topic_sync_eqps, jsonBuffer);
    Serial.println("MQTT: Sync equipamentos enviado.");
}

// Callback interna - agora totalmente desacoplada de globais
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT Msg ["); Serial.print(topic); Serial.println("]");

  if (strcmp(topic, mqtt_topic_res_usrs) == 0) {
      StaticJsonDocument<2048> doc; 
      DeserializationError error = deserializeJson(doc, payload, length);
      if (error) {
          Serial.print("Erro JSON usuarios: "); Serial.println(error.c_str());
          dadosEstadoAtual = DESATUALIZADO;
          return;
      }

      if (doc["status"] == "ATUALIZADO") {
          Serial.println("Usuarios ja estao atualizados.");
          dadosEstadoAtual = SINCRONIZADO;
          lastSync = getTimestampAtual();
          return;
      }

      if (!doc.containsKey("novo_timestamp") || !doc.containsKey("usuarios")) {
          Serial.println("Payload de usuarios invalido.");
          return;
      }

      unsigned long long novoTs = doc["novo_timestamp"];
      unsigned long long currentTs = userRepoGetTimestamp();

      if (novoTs <= currentTs && currentTs != 0) {
           Serial.println("Timestamp recebido e antigo ou igual. Ignorando.");
           dadosEstadoAtual = SINCRONIZADO;
           return;
      }

      Serial.println("Atualizando repositorio local de usuarios...");
      userRepoClear(); // Limpa antes de adicionar os novos

      JsonArray arr = doc["usuarios"].as<JsonArray>();
      for (JsonObject user : arr) {
          const char* uid = user["codigo"];
          const char* nome = user["nome"];
          if (uid && nome) {
              userRepoAdd(uid, nome);
          }
      }
      
      // Salva tudo na flash e atualiza timestamp
      userRepoSetTimestamp(novoTs);
      userRepoSave();
      
      dadosEstadoAtual = SINCRONIZADO;
      lastSync = getTimestampAtual();
      Serial.print("Repositorio atualizado. Total usuarios: ");
      Serial.println(userRepoGetCount());

  } else if (strcmp(topic, mqtt_topic_res_eqps) == 0) {
      // ... (Lógica de equipamentos mantida igual, chama atualizarInventario)
      StaticJsonDocument<2048> doc;
      deserializeJson(doc, payload, length);
      atualizarInventario(doc.as<JsonArray>());
      
  } else if (strcmp(topic, mqtt_topic_ack_eqps) == 0) {
      if (dadosEstadoAtual == SINCRONIZANDO) {
          dadosEstadoAtual = SINCRONIZADO;
          lastSync = getTimestampAtual();
          Serial.println("MQTT: Equipamentos sincronizados (ACK).");
      }
  } else if (strcmp(topic, mqtt_topic_ack_usrs) == 0) {
      if (dadosEstadoAtual == SINCRONIZANDO) {
          dadosEstadoAtual = SINCRONIZADO;
          lastSync = getTimestampAtual();
          Serial.println("MQTT: Usuários sincronizados (ACK).");
      }
  }
}

void marcaDirty() {
  dadosEstadoAtual = SUJO;
}

DadosEstado getEstadoAtual() {
  return dadosEstadoAtual;
}