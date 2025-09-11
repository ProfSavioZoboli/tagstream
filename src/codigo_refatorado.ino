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
const char* ssid = "FIESC_IOT_EDU";
const char* password = "8120gv08";
const char* mqtt_server = "189.8.205.50";
const char* mqtt_user = "profblu";
const char* mqtt_pass = "ProFBlu@hotdog";
const char* mqtt_topic_tags = "indaial/test";

// --- OBJETOS E CLIENTES ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

// --- ARMAZENAMENTO DINÂMICO DE TAGS ---
const int MAX_TAGS = 20;   // Máximo de tags autorizadas que o sistema suporta
const int TAG_LENGTH = 4;  // Tamanho do UID em bytes
byte tagsAutorizadas[MAX_TAGS][TAG_LENGTH];
int numeroDeTagsAutorizadas = 0;

// --- MÁQUINA DE ESTADOS ---
enum Estado { OCIOSO, AUTENTICADO, NAO_AUTENTICADO };
Estado estadoAtual = OCIOSO;
unsigned long tempoEstadoMudou = 0;
const int tempoDeExibicao = 3000; // 3 segundos para exibir o status

// ====================================================================================
// FUNÇÃO DE CALLBACK MQTT - Processa a lista de tags recebida do Node-RED
// ====================================================================================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("Falha ao parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  numeroDeTagsAutorizadas = 0; // Limpa a lista antiga
  JsonArray arrayDeTags = doc.as<JsonArray>();

  Serial.println("Atualizando lista de tags autorizadas:");
  for (JsonVariant v : arrayDeTags) {
    if (numeroDeTagsAutorizadas >= MAX_TAGS) {
      Serial.println("Aviso: Número máximo de tags atingido.");
      break;
    }
    const char* uidHex = v.as<const char*>();
    if (hexStringToByteArray(uidHex, tagsAutorizadas[numeroDeTagsAutorizadas], TAG_LENGTH)) {
      Serial.print("  - Adicionando tag: ");
      Serial.println(uidHex);
      numeroDeTagsAutorizadas++;
    }
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

  switch (estadoAtual) {
    case OCIOSO:
      showMensagem("Aproxime a tag");
      readRFID(); // Tenta ler um cartão
      break;

    case AUTENTICADO:
    case NAO_AUTENTICADO:
      // Verifica se o tempo de exibição do status já passou
      if (millis() - tempoEstadoMudou > tempoDeExibicao) {
        sinalSonoroLuminoso(false, true); // Desliga tudo
        estadoAtual = OCIOSO;
      }
      break;
  }
}

// ====================================================================================
// FUNÇÕES DE LÓGICA RFID E VALIDAÇÃO
// ====================================================================================

// <<< LÓGICA DE VALIDAÇÃO REFEITA >>>
// Verifica se a tag lida está na nossa lista local de tags autorizadas
bool isTagAutorizada() {
  if (numeroDeTagsAutorizadas == 0) return false;
  
  for (int i = 0; i < numeroDeTagsAutorizadas; i++) {
    if (memcmp(rfid.uid.uidByte, tagsAutorizadas[i], rfid.uid.size) == 0) {
      return true; // Encontrou!
    }
  }
  return false; // Não encontrou.
}

// Função principal de leitura, agora com a nova validação
void readRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    
    if (isTagAutorizada()) {
      estadoAtual = AUTENTICADO;
      showMensagem("AUTENTICADO");
      sinalSonoroLuminoso(true, false); // Sinal de sucesso
    } else {
      estadoAtual = NAO_AUTENTICADO;
      showMensagem("NAO AUTORIZADO");
      sinalSonoroLuminoso(false, false); // Sinal de falha
    }
    
    tempoEstadoMudou = millis(); // Marca o tempo da mudança de estado
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

// ====================================================================================
// FUNÇÕES DE FEEDBACK (Display, LED, Buzzer) - Mantidas do seu código original
// ====================================================================================

// Controla os LEDs e o Buzzer
void sinalSonoroLuminoso(bool autenticado, bool desliga) {
  if (desliga) {
    digitalWrite(LED_OCIOSO, LOW);
    digitalWrite(LED_OCUPADO, LOW);
    digitalWrite(BUZZER, LOW);
    return;
  }

  if (autenticado) {
    digitalWrite(LED_OCIOSO, HIGH);
    digitalWrite(LED_OCUPADO, LOW);
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
  } else {
    digitalWrite(LED_OCIOSO, LOW);
    digitalWrite(LED_OCUPADO, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(BUZZER, LOW);
  }
}

// Mostra mensagens no display OLED
void showMensagem(String mensagem) {
  static String ultimaMensagem = "";
  if (mensagem == ultimaMensagem) return; // Evita redesenhar a tela desnecessariamente

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
  ultimaMensagem = mensagem;
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
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
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
            client.subscribe(mqtt_topic_tags);
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