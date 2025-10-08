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

// --- LEITOR DO TECLADO ---

char inputBuffer[10]; // Buffer para armazenar a entrada do teclado (ex: "12A34")
int inputIndex = 0;   // Posição atual no buffer


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
  NENHUMA,
  EMPRESTIMO,
  DEVOLUCAO,
  MANUTENCAO
} operacaoAtual = NENHUMA;
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
    client.setBufferSize(1024);
    ultima_verif_rede = getTimestampAtual();
  }
  else
  {
    showTempMensagem("WiFi OFF", 5000);
  }
}

void loop(){

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

void verif_network(){
  if (WiFi.status() != WL_CONNECTED)  { // Se não estiver conectado ao WiFi

    if ((ultima_verif_rede / (1000 * 60)) < 1)    {
      return;
    } // tenta novamente em 1 minuto

    if (setup_wifi()){ // Tenta conectar (tentativas)
      client.setServer(mqtt_server, 1883);
      client.setCallback(callback);
    }
    ultima_verif_rede = getTimestampAtual();
  }
  else{ // Se estiver conectado
    if (!client.connected()){ // Verifica se o MQTT está conectado
      reconnect_mqtt();
    }

    client.loop(); // Verifica tópicos

    if (client.connected()){
      // Sua lógica de sincronização de tempo
      long long milis = getTimestampAtual();
      long minutosPassadosDaUltimaSync = (milis - lastSync) / (1000 * 60);
      if (minutosPassadosDaUltimaSync > 4 && dadosEstadoAtual != SINCRONIZANDO){
        dadosEstadoAtual = DESATUALIZADO;
      }
      if (dadosEstadoAtual == DESATUALIZADO){
        syncListaUsuarios();
        syncListaEquipamentos();
      }
    }
  }
}

void handleOcupado() {
  // A mensagem inicial é definida pela operação
  switch (operacaoAtual) {
    case EMPRESTIMO:
      showMensagem("Emprestimo:");
      break;
    case DEVOLUCAO:
      showMensagem("Devolucao:");
      break;
    case MANUTENCAO:
      showMensagem("Manutencao:");
      break;
    default:
      showMensagem("E: Emp\nD: Dev\nM: Manut");
      break;
  }

  // Loop principal enquanto o sistema está ocupado
  while (sistemaEstadoAtual == OCUPADO) {
    char tecla = getTeclaPressionada(); // Tenta ler uma tecla

    if (tecla != '\0') { // Se uma tecla foi pressionada
      if (operacaoAtual == NONE) {
        // Define a operação se nenhuma foi escolhida ainda
        switch (tecla) {
          case 'E': operacaoAtual = EMPRESTIMO; break;
          case 'D': operacaoAtual = DEVOLUCAO; break;
          case 'M': operacaoAtual = MANUTENCAO; break;
        }
        // Limpa o buffer para a próxima entrada
        memset(inputBuffer, 0, sizeof(inputBuffer));
        inputIndex = 0;
      } else {
        // Se já há uma operação, processa a entrada numérica ou os comandos
        switch (tecla) {
          case '*': // Cancelar/Limpar
            operacaoAtual = NENHUMA;
            memset(inputBuffer, 0, sizeof(inputBuffer));
            inputIndex = 0;
            break;
          case '#': // Confirmar
            // Adicione aqui a lógica para processar o que está no inputBuffer
            // Ex: handleEmprestimo(inputBuffer);
            showTempMensagem("Processando...", 1000);
            operacaoAtual = NENHUMA; // Reseta a operação
            memset(inputBuffer, 0, sizeof(inputBuffer));
            inputIndex = 0;
            break;
          case 'E': case 'D': case 'M':
            // Ignora teclas de operação se uma já foi selecionada
            break;
          default: // Adiciona o dígito ao buffer
            if (inputIndex < sizeof(inputBuffer) - 1) {
              inputBuffer[inputIndex++] = tecla;
              inputBuffer[inputIndex] = '\0'; // Garante o terminador nulo
            }
            break;
        }
      }
    }

    // Atualiza o display com o buffer atual (apenas se houver uma operação)
    if (operacaoAtual != NONE) {
        showMensagem(inputBuffer);
    }

    readRFID(); // Permite que o usuário faça logoff a qualquer momento
    client.loop(); // Mantém a conexão MQTT ativa
    delay(20); // Pequeno delay para evitar sobrecarga da CPU
  }
}