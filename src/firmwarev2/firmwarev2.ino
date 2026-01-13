/*
 * PROJETO: Leitor RFID V2 (Refatorado - Fase 4)
 */

#include "config.h"
#include "models.h"

#include "wifi_handler.h"
#include "mqtt_handler.h"
#include "user_repository.h"
#include "equipment_repository.h"
#include "display_handler.h"
#include "buzzer_handler.h"

#include "rfid_handler.h"
#include "keyboard_handler.h"
#include "locker_handler.h"
#include "emprestimos.h"
#include "log_repository.h"



SistemaEstado sistemaEstadoAtual = SIS_OCIOSO;
UsuarioEstado usuarioEstadoAtual = NONE;
OperacaoSistema operacaoAtual = OP_NENHUMA;

char inputBuffer[10];
int inputIndex = 0;

char idleBuffer[6];  // Buffer pequeno para códigos de manutenção
int idleIndex = 0;

void setup() {
  Serial.begin(115200);

  // 1. Hardware Básico
  setupFechadura();
  if (!setupDisplay()) { Serial.println("Falha Display"); }
  setupBuzzer();
  setupTeclado();
  setupRFID();
  logRepoInit();

  // 2. Inicializa Dados (Flash)
  userRepoInit();
  bool dadosEquipamentosExistem = equipRepoInit();
  int qtdUsuarios = userRepoGetCount();

  // 3. Lógica de Boot de Rede (Condicional)
  // Se não temos usuários OU não temos dados de equipamentos confiáveis, força conexão.
  bool precisaDeSyncObrigatorio = (qtdUsuarios == 0) || (!dadosEquipamentosExistem);

  if (precisaDeSyncObrigatorio) {
    Serial.println("BOOT: Flash vazia. Iniciando modo de recuperacao obrigatorio.");
    displayShowMensagem("Configuracao\nInicial");
    delay(2000);

    // Loop infinito até conseguir conectar WiFi
    while (!setupWiFi()) {
      displayShowTempMensagem("Sem Rede!\nTentando...", 3000);
    }

    // Conectou WiFi, agora tenta MQTT obrigatoriamente
    setupMQTT();
    displayShowMensagem("Sincronizando...");

    // Tenta conectar MQTT por um tempo antes de desistir (para não travar eternamente se o broker cair)
    unsigned long startMqttAttempt = millis();
    while (!mqttIsConnected() && (millis() - startMqttAttempt < 60000)) {  // 60s timeout para MQTT
      if (reconnect_mqtt()) {
        // Força um loop imediato para tentar baixar dados
        mqttLoop();
        checkMqttSync();
      }
      delay(500);
    }

  } else {
    // Boot Normal (tem dados na flash)
    Serial.println("BOOT: Dados encontrados. Tentativa de conexao nao-bloqueante.");
    displayShowMensagem("Iniciando...");
    // Tenta conectar uma vez. Se falhar, segue offline.
    if (setupWiFi()) {
      setupMQTT();
    } else {
      displayShowTempMensagem("Modo Offline", 2000);
    }
  }

  // 4. Finalização
  travaFechadura();
  //buzzerTocarMusicaZelda();
  buzzerSoundBoot();
}

void loop() {
  // --- 1. Manutenção de Rede ---
  // Se estiver conectado, mantem MQTT. Se caiu, tenta reconectar sem bloquear o loop principal.
  if (checkWiFiConnection()) {
    if (!mqttIsConnected()) {
      reconnect_mqtt();

    } else {
      mqttLoop();
      checkMqttSync();
      if (logRepoCountPending() > 0) {
        displayShowMensagem("Enviando logs\npendentes...");
        flushLogQueue();
      }
    }
  }

  // --- 2. Máquina de Estados Principal ---
  switch (sistemaEstadoAtual) {
    case SIS_OCIOSO:
      {
        displayShowMensagem("Aproxime a tag", false);
        readRFID();

        // --- NOVO: Detecção de código de manutenção ---
        char tec = getTeclaPressionada();
        if (tec != '\0') {
          buzzerTocarSinal(1, 3000);  // Beep agudo curto

          if (tec == '*') {
            idleIndex = 0;  // Começa código
            idleBuffer[idleIndex++] = '*';
            idleBuffer[idleIndex] = '\0';
            Serial.println("Iniciando codigo manutencao...");
          } else if (idleIndex > 0) {
            // Se já começou com *, continua gravando
            if (idleIndex < 5) {
              idleBuffer[idleIndex++] = tec;
              idleBuffer[idleIndex] = '\0';
            }

            // Verifica se completou o código *999#
            if (strcmp(idleBuffer, "*999#") == 0) {
              Serial.println("CODIGO DE RESET DETECTADO!");
              displayShowTempMensagem("RESET",2000);
              performFactoryReset();
              idleIndex = 0;  // Reset buffer
            }

            if (strcmp(idleBuffer, "*1138") == 0) {
              Serial.println("CODIGO DO STAR WARS!");
              displayShowTempMensagem("IM YOUR FATHER",2000);
              buzzerTocarStarWars();
              idleIndex = 0;  // Reset buffer
            }
            if (strcmp(idleBuffer, "*5843") == 0) {
              Serial.println("CODIGO DO SUPER MARIO!");
              displayShowTempMensagem("ITS ME LUIGI",2000);
              buzzerTocarMarioMoeda();
              idleIndex = 0;  // Reset buffer
            }
          }
        }
        // ---------------------------------------------

        // Reset de buffers de operação normal
        inputIndex = 0;
        memset(inputBuffer, 0, sizeof(inputBuffer));
        operacaoAtual = OP_NENHUMA;
        break;
      }
    case SIS_OCUPADO:
      handleOcupado();
      break;
  }

  delay(50);
}

void handleOcupado() {
  // A mensagem inicial é definida pela operação
  switch (operacaoAtual) {
    case OP_EMPRESTIMO:
      displayShowMensagem("Emprestimo:");
      break;
    case OP_DEVOLUCAO:
      displayShowMensagem("Devolucao:");
      break;
    case OP_MANUTENCAO:
      displayShowMensagem("Manutencao:");
      break;
    default:
      displayShowMensagem("E: Emp\nD: Dev\nM: Manut");
      break;
  }

  // Loop principal enquanto o sistema está ocupado
  while (sistemaEstadoAtual == SIS_OCUPADO) {
    char tecla = getTeclaPressionada();  // Tenta ler uma tecla

    if (tecla != '\0') {  // Se uma tecla foi pressionada
      if (operacaoAtual == OP_NENHUMA) {
        // Define a operação se nenhuma foi escolhida ainda
        switch (tecla) {
          case 'E': operacaoAtual = OP_EMPRESTIMO; break;
          case 'D': operacaoAtual = OP_DEVOLUCAO; break;
          case 'M': operacaoAtual = OP_MANUTENCAO; break;
        }
        // Limpa o buffer para a próxima entrada
        memset(inputBuffer, 0, sizeof(inputBuffer));
        inputIndex = 0;
      } else {
        // Se já há uma operação, processa a entrada numérica ou os comandos
        switch (tecla) {
          case '*':  // Cancelar/Limpar
            memset(inputBuffer, 0, sizeof(inputBuffer));
            inputIndex = 0;
            break;
          case '#':  // Confirmar
            // Adicione aqui a lógica para processar o que está no inputBuffer
            // Ex: handleEmprestimo(inputBuffer);
            displayShowTempMensagem("Processando...", 1000);
            Serial.print("Valor no buffer: ");
            Serial.println(inputBuffer);
            if (operacaoAtual == OP_EMPRESTIMO) {
              if (handleEmprestimo(inputBuffer)) {
                buzzerSoundOperationSuccess();
                displayShowTempMensagem("Ok", 2000);
              } else {
                buzzerSoundOperationFail();
                displayShowTempMensagem("Erro:\nJa emprestado", 3000);
              }
            } else if (operacaoAtual == OP_DEVOLUCAO) {
              if (handleDevolucao(inputBuffer)) {
                buzzerSoundOperationSuccess();
                displayShowTempMensagem("Ok", 2000);
              } else {
                buzzerSoundOperationFail();
                displayShowTempMensagem("Erro:\nJa devolvido", 3000);
              }
            } else {
            }


            memset(inputBuffer, 0, sizeof(inputBuffer));
            inputIndex = 0;
            break;
          case 'E':
          case 'D':
          case 'M':
            // Ignora teclas de operação se uma já foi selecionada
            break;
          default:  // Adiciona o dígito ao buffer
            if (inputIndex < sizeof(inputBuffer) - 1) {
              inputBuffer[inputIndex++] = tecla;
              inputBuffer[inputIndex] = '\0';  // Garante o terminador nulo
            }
            break;
        }
      }
    }

    // Atualiza o display com o buffer atual (apenas se houver uma operação)
    if (operacaoAtual != OP_NENHUMA) {
      displayShowMensagem(inputBuffer);
    }

    readRFID();  // Permite que o usuário faça logoff a qualquer momento
    mqttLoop();  // Mantém a conexão MQTT ativa
    delay(100);  // Pequeno delay para evitar sobrecarga da CPU
  }
}

void performFactoryReset() {
  displayShowMensagem("FACTORY RESET\nEM 5s...");
  buzzerTocarSinal(5, 1000);  // 5 beeps de aviso
  delay(1000);

  displayShowMensagem("Limpando\nMemoria...");
  userRepoFactoryReset();
  equipRepoFactoryReset();
  logRepoFactoryReset();

  displayShowMensagem("Reiniciando...");
  delay(2000);
  ESP.restart();  // Reinicia o microcontrolador
}
