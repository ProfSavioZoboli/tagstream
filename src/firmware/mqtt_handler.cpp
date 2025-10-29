// Arquivo: mqtt_handler.cpp

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <chrono>
#include "mqtt_handler.h"
#include "config.h"
#include "secrets.h"
#include "rfid_handler.h"  // Para ter acesso à struct Usuario
#include "utils.h"
#include "emprestimos.h"

// Variáveis globais do .ino que este arquivo precisa conhecer
extern PubSubClient client;
extern Usuario usuariosAutorizados[MAX_TAGS];
extern int numeroDeTagsAutorizadas;
extern unsigned long long timestampDaListaLocal;
extern unsigned long long lastSync;
extern DadosEstado dadosEstadoAtual;



const int max_tentativas = 10;
long long ultima_tentativa = 0;

bool setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int tentativa = 0;
  while (WiFi.status() != WL_CONNECTED && tentativa < max_tentativas) {
    delay(500);
    Serial.print('.');
    tentativa++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  /*if (WiFi.status() != WL_CONNECTED) {
    
    Serial.print("Conectando a rede failover ");
    Serial.println(ssid_failover);
    WiFi.begin(ssid_failover, password_failover);
    int tentativa = 0;
    while (WiFi.status() != WL_CONNECTED && tentativa < max_tentativas) {
      delay(500);
      Serial.print('.');
      tentativa++;
      Serial.print(WiFi.status());
    }
    if (WiFi.status() != WL_CONNECTED) {
      return false;
    }
  }*/
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

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

bool reconnect_mqtt() {
  int tentativa = 0;
  long long timestamp = getTimestampAtual();
  if ((getTimestampAtual() - ultima_tentativa) < (1000 * 60)) {
    return false;
  }
  while (!client.connected() & tentativa < max_tentativas) {
    Serial.print("Tentando conectar ao MQTT...");
    // Conecta usando um ID de cliente, usuário e senha
    if (client.connect("ESP32_RFID_Client_01", mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");
      // Reinscreve no tópico após a reconexão
      client.subscribe(mqtt_topic_res_usrs);
      client.subscribe(mqtt_topic_res_eqps);
      return true;
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());

      Serial.println(" tentando novamente em 5 segundos");
      tentativa++;
      delay(5000);
    }
  }
  Serial.println("Tentativas excedidas, tentando novamente em alguns minutos.");
  ultima_tentativa = getTimestampAtual();
  return false;
}

void syncListaUsuarios() {
  client.publish(mqtt_topic_req_usrs, "");
  Serial.print("MQTT: Enviado requisicao no topico ");
  Serial.print(mqtt_topic_req_usrs);
  Serial.println(" para atualizacao da lista de usuarios");
}

void sendOperacaoUsuario(Usuario* usuario, String operacao) {
  if (usuario == nullptr) {
    return;
  }
  StaticJsonDocument<128> doc;
  JsonObject usuarioObj = doc.createNestedObject("usuario");
  char uidHex[TAG_LENGTH * 2 + 1];  // Buffer para a string (ex: 4 bytes -> 8 chars + nulo)
  byteArrayToHexString(usuario->uid, TAG_LENGTH, uidHex, sizeof(uidHex));

  usuarioObj["uid"] = uidHex;
  usuarioObj["nome"] = usuario->nome;
  usuarioObj["operacao"] = operacao;

  char jsonBuffer[128];
  serializeJson(doc, jsonBuffer);

  client.publish(mqtt_topic_log_usuarioLogado, jsonBuffer);  // Substitua pelo seu tópico real se necessário

  Serial.print("Enviado status do usuario: ");
  Serial.println(jsonBuffer);
}

void syncListaEquipamentos() {
  if (dadosEstadoAtual == DadosEstado::DESATUALIZADO) {
    client.publish(mqtt_topic_req_eqps, "");
    Serial.println("MQTT: Enviado requisicao para atualizacao da lista de equipamentos.");
  } else if (dadosEstadoAtual == DadosEstado::SUJO) {

    // --- Início do Código de Buffer JSON ---
    // Para N equipamentos, o tamanho é: N * (tamanho de um objeto) + overhead do array.
    // Ex: {"numero":123,"situacao":"MANUTENCAO"} -> ~45 bytes
    // Para 10 equipamentos: 10 * 45 + 10 (overhead) = 460

    const int JSON_DOC_SIZE = 2048;
    StaticJsonDocument<JSON_DOC_SIZE> doc;

    // 2. Crie o JSON como um Array (será a raiz do documento).
    JsonArray jsonArray = doc.to<JsonArray>();

    // 3. Itere sobre seu array de structs e adicione ao JSON
    // (Assumindo que seu array se chama 'listaDeEquipamentos' e tem 'NUM_EQUIPAMENTOS' itens)
    for (int i = 0; i < MAX_EQUIPAMENTOS; i++) {
      // Cria um objeto JSON aninhado dentro do array
      JsonObject equipamentoJson = jsonArray.createNestedObject();

      // Preenche o objeto JSON com os dados da struct
      equipamentoJson["numero"] = getNumeroEquipamento(i);
      equipamentoJson["situacao"] = getSituacaoEquipamento(i);
    }

    // 4. Serialize o documento JSON para um buffer de caracteres
    // ATENÇÃO: O PubSubClient por padrão tem um buffer de 256 bytes (MQTT_MAX_PACKET_SIZE).
    // Se o seu 'jsonBuffer' for maior que isso, a mensagem será cortada.
    // Veja a nota "MQTT_MAX_PACKET_SIZE" abaixo.
    char jsonBuffer[JSON_DOC_SIZE];
    serializeJson(doc, jsonBuffer);

    // (Opcional) Imprimir o JSON no Serial para debug
    Serial.print("MQTT: JSON a ser enviado: ");
    Serial.println(jsonBuffer);

    // 5. Publicar o buffer
    client.publish(mqtt_topic_sync_eqps, jsonBuffer);

    // --- Fim do Código de Buffer JSON ---

    Serial.println("MQTT: Sincronizando equipamentos emprestados/devolvidos");
    setEstadoAtual(SINCRONIZANDO);
  }
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
      lastSync = getTimestampAtual();

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
    lastSync = getTimestampAtual();
    Serial.print("Lista de usuários atualizada com sucesso. Novo timestamp local: ");
    Serial.println(timestampDaListaLocal);
  } else if (strcmp(topic, mqtt_topic_res_eqps) == 0) {
    Serial.println("Lista de equipamentos recebida. Processando...");

    // Aumente o tamanho se a lista de equipamentos for muito grande
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
      Serial.print("Falha ao parsear JSON da lista de equipamentos: ");
      Serial.println(error.c_str());
      return;
    }

    // Converte o payload inteiro para um JsonArray
    JsonArray equipamentosArray = doc.as<JsonArray>();

    // Chama a função para processar o array
    atualizarInventario(equipamentosArray);
  }
}

void marcaDirty() {
  dadosEstadoAtual = DadosEstado::SUJO;
}

DadosEstado getEstadoAtual() {
  return dadosEstadoAtual;
}

void setEstadoAtual(DadosEstado estado) {
  dadosEstadoAtual = estado;
}
