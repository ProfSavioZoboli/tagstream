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
#include "keyboard_handler.h"
#include "locker_handler.h"

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

enum SistemaEstado
{
  OCIOSO,
  OCUPADO
} sistemaEstadoAtual = OCIOSO;
enum UsuarioEstado
{
  NONE,
  AUTORIZADO,
  NAUTORIZADO
} usuarioEstadoAtual = NONE;
enum DadosEstado
{
  DESATUALIZADO,
  SUJO,
  SINCRONIZADO,
  SINCRONIZANDO
} dadosEstadoAtual = DESATUALIZADO;

enum OperacaoSistema
{
  NONE,
  EMPRESTIMO,
  DEVOLUCAO,
  MANUTENCAO
} operacaoAtual = NONE;
unsigned long tempoEstadoMudou = 0;

// --- VERIFICAÇÃO DA REDE ---
unsigned long long ultima_verif_rede = 0;

void setup()
{
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  pinMode(RELE, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS))
  {
    Serial.println(F("Falha ao iniciar display OLED"));
  }
  SPI.begin();
  rfid.PCD_Init();
  setupTeclado();
  travaFechadura();

  showMensagem("Iniciando...");
  delay(1000);
  // tocarCantinaBand();
  tocarMusicaZelda();
  // tocarStarWars();

  if (setup_wifi())
  {
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    ultima_verif_rede = getTimestampAtual();
  }
  else
  {
    showTempMensagem("WiFi OFF", 5000);
  }
}

void loop()
{

  verif_network();

  switch (sistemaEstadoAtual)
  {
  case OCIOSO:
    showMensagem("Aproxime a tag");
    readRFID();
    break;
  case OCUPADO:

    handleOcupado();

    break;
  }
  delay(100);
}

void verif_network()
{
  if (WiFi.status() != WL_CONNECTED)
  { // Se não estiver conectado ao WiFi

    if (ultima_verif_rede / (1000 * 60) < 1)
    {
      return;
    } // tenta novamente em 1 minuto

    if (setup_wifi())
    { // Tenta conectar (tentativas)
      client.setServer(mqtt_server, 1883);
      client.setCallback(callback);
    }
  }
  else
  { // Se estiver conectado
    if (!client.connected())
    { // Verifica se o MQTT está conectado
      reconnect_mqtt();
    }

    client.loop(); // Verifica tópicos

    if (client.connected())
    {
      // Sua lógica de sincronização de tempo
      long long milis = getTimestampAtual();
      long minutosPassadosDaUltimaSync = (milis - lastSync) / (1000 * 60);
      if (minutosPassadosDaUltimaSync > 4 && dadosEstadoAtual != SINCRONIZANDO)
      {
        dadosEstadoAtual = DESATUALIZADO;
      }
      if (dadosEstadoAtual == DESATUALIZADO)
      {
        syncListaUsuarios();
      }
    }
  }
}

void handleOcupado()
{
  char *valor = '';
  showMensagem(valor);
  while (sistemaEstadoAtual == OCUPADO)
  {
    char tecla = getTeclaPressionada();
    if (tecla != '\0')
    {

      if (operacaoAtual == NONE)
      {
        switch (tecla)
        {
        case 'E':
          // Empréstimo
          operacaoAtual = EMPRESTIMO;
          break;
        case 'D':
          // Devolução
          operacaoAtual = DEVOLUCAO;
          break;
        case 'M':
          // Manutenção
          operacaoAtual = MANUTENCAO;
          break;
        }
        char *valor = '';
      }
      else if (tecla != 'E' && tecla != 'D' && tecla != 'M')
      {
        switch (tecla)
        {
        case '*':
          // Cancelar
          char *valor = '';
          break;
        case '#':
          // Confirmar
          char *valor = '';
          break;
        default:
          // Outras teclas
          valor = valor + tecla;
          break;
        }
      }
    }
    showMensagem(valor);
    readRFID();
  }
}