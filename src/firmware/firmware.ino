// Arquivo: ProjetoRFID.ino

/*
 * ====================================================================================
 * PROJETO: Leitor RFID com Display OLED, LEDs, Buzzer e Validação Dinâmica via MQTT
 * DESCRIÇÃO: Arquivo principal que coordena os módulos de RFID, Feedback e Rede.
 * DATA: 12/09/2025
 * ====================================================================================
 */

// --- INCLUDES DOS MÓDULOS DO PROJETO ---
#include "config.h"
#include "secrets.h"
#include "pitches.h"
#include "feedback_handler.h"
#include "melodias.h"
#include "rfid_handler.h"
#include "mqtt_handler.h"
#include "utils.h"

// --- BIBLIOTECAS DE HARDWARE E REDE (Para instanciar os objetos) ---
#include <Adafruit_SSD1306.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>


// --- OBJETOS E CLIENTES GLOBAIS ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

// --- DADOS E ESTADOS GLOBAIS ---
Usuario usuariosAutorizados[MAX_TAGS];
int numeroDeTagsAutorizadas = 0;
unsigned long long timestampDaListaLocal = 0;
unsigned long long lastSync = 0;

enum SistemaEstado { OCIOSO, OCUPADO} sistemaEstadoAtual = OCIOSO;
enum UsuarioEstado {NONE, AUTORIZADO, NAUTORIZADO} usuarioEstadoAtual = NONE;
enum DadosEstado {DESATUALIZADO, SUJO, SINCRONIZADO, SINCRONIZANDO} dadosEstadoAtual = DESATUALIZADO;
unsigned long tempoEstadoMudou = 0;


void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_OCIOSO, OUTPUT);
  pinMode(LED_OCUPADO, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    Serial.println(F("Falha ao iniciar display OLED"));
    while (true);
  }
  SPI.begin();
  rfid.PCD_Init();
  
  showMensagem("Iniciando...");
  delay(1000);
  //tocarCantinaBand();
  //tocarMusicaZelda();
  tocarStarWars();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  configLedsEstado();
}


void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  if(client.connected()){
    // Sua lógica de sincronização de tempo
    long long milis = getTimestampAtual();
    long minutosPassadosDaUltimaSync = (milis - lastSync)/(1000*60);
    if(minutosPassadosDaUltimaSync > 4 && dadosEstadoAtual != SINCRONIZANDO){
      dadosEstadoAtual = DESATUALIZADO;
    }
    if(dadosEstadoAtual == DESATUALIZADO){
      syncListaUsuarios();
    }
  }

  switch (sistemaEstadoAtual) {
    case OCIOSO:
      showMensagem("Aproxime a tag");
      readRFID();
      break;
    case OCUPADO:
      //delay(3000);
      //sistemaEstadoAtual = OCIOSO;
      //usuarioEstadoAtual = NONE;
      readRFID();
      configLedsEstado();
      break;
  }
  delay(100);
}