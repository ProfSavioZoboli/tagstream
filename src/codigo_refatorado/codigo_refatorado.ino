/*
 * ====================================================================================
 * PROJETO: Leitor RFID com Display OLED, LEDs, Buzzer e Validação Dinâmica via MQTT
 * DESCRIÇÃO: Este código integra um sistema de controle de acesso físico (RFID,
 * LEDs, Buzzer) com uma lista de UIDs autorizados gerenciada remotamente via
 * Node-RED e MQTT.
 * AUTOR: (Seu Nome) - Refatorado por Gemini
 * DATA: 11/09/2025
 * ====================================================================================
 */

// --- BIBLIOTECAS DE HARDWARE ---
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- BIBLIOTECAS DE REDE ---
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- DEFINIÇÕES DE PINOS ---
// Hardware de Feedback
#define BUZZER 26
#define LED_OCIOSO 33
#define LED_OCUPADO 32
// Display OLED (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_ADDRESS 0x3C // IMPORTANTE: Verifique com o I2C Scanner!
// Leitor RFID (SPI)
const byte SS_PIN = 5;
const byte RST_PIN = 2;

// --- CONFIGURAÇÕES DE REDE E MQTT ---
const char* ssid = "Smart 4.0 (3)";
const char* password = "Smart4.0";
const char* mqtt_server = "10.77.241.62";
const char* mqtt_user = "senai";
const char* mqtt_pass = "senai";
const char* mqtt_topic_req_usrs = "/indaial/request/teste";
const char* mqtt_topic_res_usrs = "/indaial/response/teste";

// --- OBJETOS E CLIENTES ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

// --- ARMAZENAMENTO DINÂMICO DE TAGS ---
const int MAX_TAGS = 20;   // Máximo de tags autorizadas que o sistema suporta
const int TAG_LENGTH = 4;  // Tamanho do UID em bytes
// --- ESTRUTURA DE DADOS PARA USUÁRIOS ---
struct Usuario {
  byte uid[TAG_LENGTH];
  char nome[32]; // Espaço para até 31 caracteres + terminador nulo
  int nivelAcesso;
};
Usuario usuariosAutorizados[MAX_TAGS];
int numeroDeTagsAutorizadas = 0;

// --- MÁQUINA DE ESTADOS ---
enum SistemaEstado { OCIOSO, OCUPADO};
enum UsuarioEstado {NONE, AUTORIZADO, NAUTORIZADO};
enum DadosEstado {DESATUALIZADO, SUJO, SINCRONIZADO};
SistemaEstado sistemaEstadoAtual = SistemaEstado::OCIOSO;
UsuarioEstado usuarioEstadoAtual = UsuarioEstado::NONE;
DadosEstado dadosEstadoAtual = DadosEstado::DESATUALIZADO;
unsigned long tempoEstadoMudou = 0;
const int tempoDeExibicao = 3000; // 3 segundos para exibir o status

// ====================================================================================
// FUNÇÃO DE CALLBACK MQTT - Processa a lista de tags recebida do Node-RED
// ====================================================================================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

   if (strcmp(topic, mqtt_topic_res_usrs) == 0) {
    Serial.println("Processando objeto de usuários...");
    
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, payload, length);

    numeroDeTagsAutorizadas = 0;
    JsonObject root = doc.as<JsonObject>();

    for (JsonPair kv : root) {
      if (numeroDeTagsAutorizadas >= MAX_TAGS) break;
      
      const char* uidHex = kv.key().c_str();
      JsonObject userData = kv.value().as<JsonObject>();
      
      const char* nome = userData["usuario"];
      int nivel = userData["nivel_acesso"];

      // Converte o UID e copia os dados para nossa struct
      hexStringToByteArray(uidHex, usuariosAutorizados[numeroDeTagsAutorizadas].uid, TAG_LENGTH);
      strncpy(usuariosAutorizados[numeroDeTagsAutorizadas].nome, nome, sizeof(usuariosAutorizados[0].nome) - 1);
      usuariosAutorizados[numeroDeTagsAutorizadas].nome[sizeof(usuariosAutorizados[0].nome) - 1] = '\0'; // Garante terminação nula
      usuariosAutorizados[numeroDeTagsAutorizadas].nivelAcesso = nivel;

      numeroDeTagsAutorizadas++;
    }
    Serial.print("\nQuantidade de usuários lidas:");
    Serial.println(numeroDeTagsAutorizadas);
  }
}

// ====================================================================================
// SETUP - Configuração inicial do sistema
// ====================================================================================
void setup() {
  Serial.begin(115200);
  
  // Configura pinos de feedback
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_OCIOSO, OUTPUT);
  pinMode(LED_OCUPADO, OUTPUT);

  // Inicializa Hardware
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    Serial.println(F("Falha ao iniciar display OLED"));
    while (true);
  }
  SPI.begin();
  rfid.PCD_Init();
  
  showMensagem("Iniciando...");
  delay(1000);

  // Conecta à rede
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Atualiza leds
  configLedsEstado();

  //Sync

  

}

// ====================================================================================
// LOOP PRINCIPAL - Controlado pela máquina de estados
// ====================================================================================
void loop() {
  // Garante que o cliente MQTT esteja sempre conectado e processando mensagens
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  switch (sistemaEstadoAtual) {
    case SistemaEstado::OCIOSO:
      showMensagem("Aproxime a tag");
      readRFID(); // Tenta ler um cartão
      break;

    case SistemaEstado::OCUPADO:
      delay(3000);
      sistemaEstadoAtual = SistemaEstado::OCIOSO;
      usuarioEstadoAtual = UsuarioEstado::NONE;
      configLedsEstado();
      break;
  }
}

// ====================================================================================
// FUNÇÕES DE LÓGICA RFID E VALIDAÇÃO
// ====================================================================================


// Verifica se a tag lida está na nossa lista local de tags autorizadas
Usuario* getUsuarioAutorizado(byte* uid, byte uidSize) {
  Usuario* usuario = getUsuario(uid,uidSize);
  return usuario;    
}


// Procura por um UID e retorna um ponteiro para a struct do usuário, ou nullptr se não encontrar
Usuario* getUsuario(byte* uid, byte uidSize) {
  for (int i = 0; i < numeroDeTagsAutorizadas; i++) {
    if (memcmp(uid, usuariosAutorizados[i].uid, uidSize) == 0) {
      return &usuariosAutorizados[i]; // Retorna o endereço da struct encontrada
    }
  }
  return nullptr; // Retorna nulo se não encontrou
}

// Função principal de leitura, agora com a nova validação
void readRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Usuario* usuario = getUsuarioAutorizado(rfid.uid.uidByte,rfid.uid.size);
    if (usuario != nullptr) {
      sistemaEstadoAtual = SistemaEstado::OCUPADO;
      usuarioEstadoAtual = UsuarioEstado::AUTORIZADO;
      String mensagem = "Bem vindo, ";
      mensagem += usuario -> nome;
      showMensagem(mensagem);
      configLedsEstado();
      sinalSonoro(2);
    } else {
      sistemaEstadoAtual = SistemaEstado::OCIOSO;
      usuarioEstadoAtual = UsuarioEstado::NAUTORIZADO;
      showMensagem("NAO AUTORIZADO");
      configLedsEstado();
      sinalSonoro(3);
      delay(2000);
      usuarioEstadoAtual = UsuarioEstado::NONE;
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

// ====================================================================================
// FUNÇÕES DE FEEDBACK (Display, LED, Buzzer) - Mantidas do seu código original
// ====================================================================================

// Controla os LEDs e o Buzzer
void configLedsEstado(){

  digitalWrite(LED_OCIOSO,LOW);
  digitalWrite(LED_OCUPADO,LOW);

  if(sistemaEstadoAtual == SistemaEstado::OCIOSO){
    digitalWrite(LED_OCIOSO,HIGH);
  }
  if(sistemaEstadoAtual == SistemaEstado::OCUPADO){
    digitalWrite(LED_OCUPADO,HIGH);
  }
}

void sinalSonoro(int rounds){
  for(int i=0;i<rounds;i++){
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
}


// Mostra mensagens no display OLED
void showMensagem(String mensagem) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 25); // Centraliza um pouco melhor
  
  // Centraliza o texto para melhor visualização
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(mensagem, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);

  display.println(mensagem);
  display.display();
}

// ====================================================================================
// FUNÇÕES AUXILIARES DE REDE E CONVERSÃO
// ====================================================================================

// Converte String Hex (ex: "708CE255") para um array de bytes
bool hexStringToByteArray(const char* hexStr, byte* byteArray, int arraySize) {
  int len = strlen(hexStr);
  if (len != arraySize * 2) return false;

  for (int i = 0; i < arraySize; i++) {
    char hexPair[3] = {hexStr[i * 2], hexStr[i * 2 + 1], '\0'};
    byteArray[i] = (byte)strtol(hexPair, NULL, 16);
  }
  return true;
}

// Configura e conecta ao WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  float counter = 1.0; // Mude para float
while (WiFi.status() != WL_CONNECTED) {
    delay(500 * counter);
    counter += 0.1;
}

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
}

// Reconecta ao broker MQTT se a conexão cair
void reconnect_mqtt() { 
    while (!client.connected()) {
        Serial.print("Tentando conectar ao MQTT...");
        // Conecta usando um ID de cliente, usuário e senha
        if (client.connect("ESP32_RFID_Client_01", mqtt_user, mqtt_pass)) {
            Serial.println("conectado!");
            // Reinscreve no tópico após a reconexão
            client.subscribe(mqtt_topic_res_usrs);


            syncListaUsuarios();
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

// Função auxiliar para imprimir um UID no formato "XX XX XX XX" no Monitor Serial
void imprimirUIDSerial(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

// ======================================================================================
// REQUISIÇÕES MQTT
// ======================================================================================

// Envia uma requisição para o Node-RED pedindo informações sobre uma tag específica
void solicitarInfoDaTag(byte* uid, byte uidSize) {
  if (!client.connected()) {
    Serial.println("Não conectado ao MQTT. Requisição cancelada.");
    return;
  }

  // 1. Converte o UID de bytes para uma string Hexadecimal
  String uidHex = "";
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] < 0x10) { uidHex += "0"; }
    uidHex += String(uid[i], HEX);
  }
  uidHex.toUpperCase();

  // 2. Monta a mensagem de requisição em JSON
  StaticJsonDocument<200> doc;
  doc["tagId"] = uidHex;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  // 3. Publica a mensagem no tópico de requisição
  client.publish(mqtt_topic_req_usrs, jsonBuffer);
  
  Serial.print("Requisição enviada para o tópico '");
  Serial.print(mqtt_topic_req_usrs);
  Serial.print("' com a tag: ");
  Serial.println(uidHex);
}

void syncListaUsuarios(){
  if (!client.connected()) {
    Serial.println("Não conectado ao MQTT. Requisição cancelada.");
    return;
  }
  client.publish(mqtt_topic_req_usrs,"");
  Serial.print("MQTT: Enviado requisicao no topico ");
  Serial.print(mqtt_topic_req_usrs);
  Serial.println(" para atualizacao da lista de usuarios");
}
