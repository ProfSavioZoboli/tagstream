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

// --- BIBLIOTECAS DE APOIO ---

#include <chrono>
#include "pitches.h" //Notas musicais, porque sim

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
const char* mqtt_topic_req_usrs = "/indaial/req/usuarios";
const char* mqtt_topic_res_usrs = "/indaial/res/usuarios";

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
unsigned long long timestampDaListaLocal = 0;
unsigned long long lastSync = 0;

// --- MÁQUINA DE ESTADOS ---
enum SistemaEstado { OCIOSO, OCUPADO};
enum UsuarioEstado {NONE, AUTORIZADO, NAUTORIZADO};
enum DadosEstado {DESATUALIZADO, SUJO, SINCRONIZADO,SINCRONIZANDO};
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
    Serial.println("Mensagem de usuários recebida. Processando...");

  // 1. Parsear o JSON recebido
  StaticJsonDocument<1024> doc; // Aumente se a lista for muito grande
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
    usuariosAutorizados[numeroDeTagsAutorizadas].nome[sizeof(usuariosAutorizados[0].nome) - 1] = '\0'; // Garante terminação nula
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
  //tocarMusicaZelda();
  tocarCantinaBand();


  // Conecta à rede
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Atualiza leds
  configLedsEstado();

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

  if(client.connected()){
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    //long horasPassadasDaUltimaSync = (milis - lastSync)/(1000*60*60);
    long horasPassadasDaUltimaSync = (milis - lastSync)/(1000*60); // Teste com 4 minutos para ver acontecendo
    if(horasPassadasDaUltimaSync > 4){
      dadosEstadoAtual = DadosEstado::DESATUALIZADO;
    }

    if(dadosEstadoAtual == DadosEstado::DESATUALIZADO){
      syncListaUsuarios();
    }
  }

  

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
  delay(100);
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
      //sinalSonoro(2,349);
      tocarMarioMoeda();
    } else {
      sistemaEstadoAtual = SistemaEstado::OCIOSO;
      usuarioEstadoAtual = UsuarioEstado::NAUTORIZADO;
      showMensagem("NAO AUTORIZADO");
      configLedsEstado();
      tocarSomDeFalha();
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

void sinalSonoro(int rounds,int tom){
  for(int i=0;i<rounds;i++){
    tone(BUZZER,tom,100);
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
  dadosEstadoAtual = DadosEstado::SINCRONIZANDO;
  client.publish(mqtt_topic_req_usrs,"");
  Serial.print("MQTT: Enviado requisicao no topico ");
  Serial.print(mqtt_topic_req_usrs);
  Serial.println(" para atualizacao da lista de usuarios");
}



// ====================================================================================
// FUNÇÕES DE "FIRULA" :)
// ====================================================================================

void tocarMusicaZelda() {
  // Notas da melodia "Secret Found" do Zelda
  int melody[] = {
    NOTE_G4, NOTE_FS4, NOTE_DS4, NOTE_A4, NOTE_GS4, NOTE_E4, NOTE_GS4, NOTE_C5
  };

  // Duração de cada nota: 4 = semínima, 8 = colcheia, etc.
  int noteDurations[] = {
    8, 8, 8, 4, 8, 4, 8, 2
  };

  Serial.println("Tocando a música de sucesso do Zelda...");

  for (int thisNote = 0; thisNote < 8; thisNote++) {
    // Calcula a duração da nota em milissegundos
    int noteDuration = 1000 / noteDurations[thisNote];
    
    // Toca a nota
    // tone(pino, frequencia, duracao)
    tone(BUZZER, melody[thisNote], noteDuration);

    // Adiciona uma pequena pausa entre as notas para que não se misturem
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    
    // Para a reprodução da nota (opcional, mas bom para garantir)
    noTone(BUZZER);
  }
  Serial.println("Música finalizada.");
}

void tocarCantinaBand() {
  // A primeira parte da melodia da Cantina Band
  int melody[] = {
    // Frase 1
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_G4, NOTE_FS4, NOTE_E4, NOTE_D4,
    // Frase 2
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_G4, NOTE_FS4, NOTE_E4, NOTE_D4,
    // Frase 3 (a parte que sobe)
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5, NOTE_CS5,
    // Frase 4 (repetição da 2)
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_G4, NOTE_FS4, NOTE_E4, NOTE_D4
  };

  // Duração de cada nota
  // 8 = colcheia, 4 = semínima. Negativo = nota pontuada.
  int noteDurations[] = {
    // Frase 1
    8, 4, 8, 4, 8, 8, 8, 8, 4,
    // Frase 2
    8, 4, 8, 4, 8, 8, 8, 8, 4,
    // Frase 3
    8, 4, 8, 4, 8, 8, 8, 8, 4,
    // Frase 4
    -4, 8, 8, 8, 8, 8, 8, 8, 4
  };

  Serial.println("Tocando a Cantina Band...");

  // Calcula o número total de notas na melodia
  int songLength = sizeof(melody) / sizeof(melody[0]);

  for (int thisNote = 0; thisNote < songLength; thisNote++) {
    // Calcula a duração base da nota. abs() remove o sinal negativo para o cálculo.
    int noteDuration = 1000 / abs(noteDurations[thisNote]);

    // Se a duração for negativa no array, é uma nota pontuada (dura 1.5x mais)
    if (noteDurations[thisNote] < 0) {
      noteDuration *= 1.5;
    }

    // Toca a nota
    tone(BUZZER, melody[thisNote], noteDuration);

    // Pausa entre as notas para um som mais limpo
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    
    // Para a reprodução
    noTone(BUZZER);
  }
  Serial.println("Música finalizada.");
}

void tocarMarioMoeda() {
  // Melodia "1-Up" do Super Mario Bros.
  int melody[] = {
    NOTE_E5, NOTE_G5, NOTE_E6, NOTE_C6, NOTE_D6, NOTE_G6
  };

  // Duração de cada nota. São todas bem rápidas.
  int noteDurations[] = {
    16, 16, 16, 16, 16, 16
  };

  int songLength = sizeof(melody) / sizeof(melody[0]);

  for (int thisNote = 0; thisNote < songLength; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZER, melody[thisNote], noteDuration);
    
    // Pausa um pouco menor para um som mais contínuo e rápido
    int pauseBetweenNotes = noteDuration * 1.1; 
    delay(pauseBetweenNotes);
    
    noTone(BUZZER);
  }
}

void tocarSomDeFalha() {
  // Melodia grave e descendente para indicar falha
  int melody[] = {
    NOTE_G3, NOTE_FS3, NOTE_F3, NOTE_E3
  };

  // Duração de cada nota. A última é mais longa para dar um final.
  int noteDurations[] = {
    12, 12, 12, 6
  };

  int songLength = sizeof(melody) / sizeof(melody[0]);

  for (int thisNote = 0; thisNote < songLength; thisNote++) {
    int noteDuration = 1200 / noteDurations[thisNote]; // Um pouco mais lento que o normal
    tone(BUZZER, melody[thisNote], noteDuration);
    
    int pauseBetweenNotes = noteDuration * 1.2;
    delay(pauseBetweenNotes);
    
    noTone(BUZZER);
  }
}
