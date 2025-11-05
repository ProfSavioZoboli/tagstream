#include <MFRC522.h>
#include "rfid_handler.h"
#include "config.h"
#include "display_handler.h" // Atualizado para o novo handler
#include "buzzer_handler.h"  // Atualizado para o novo handler
#include <optional>
#include "mqtt_handler.h"
#include "utils.h"
#include "locker_handler.h"
#include "models.h"
#include "user_repository.h" // <--- A nova fonte da verdade
#include "log_repository.h"
#include "time.h"

static MFRC522 rfid(SS_PIN, RST_PIN);

extern SistemaEstado sistemaEstadoAtual;
extern UsuarioEstado usuarioEstadoAtual;
std::optional<Usuario> usuarioLogado;


void setupRFID() {
    SPI.begin();      // Inicia barramento SPI
    rfid.PCD_Init();  // Inicia o módulo MFRC522
    Serial.println("RFID: Hardware inicializado.");
}

void readRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    
    // IMPRESSÃO DO UID
    char uidStr[TAG_LENGTH * 2 + 1];
    byteArrayToHexString(rfid.uid.uidByte, rfid.uid.size, uidStr, sizeof(uidStr));
    Serial.print("--- TAG DETECTADA --- UID: [");
    Serial.print(uidStr);
    Serial.println("]");

    // Usa o repositório para buscar o usuário
    Usuario* usuario = userRepoFind(rfid.uid.uidByte, rfid.uid.size);
    
    if(usuario == nullptr){
      rejectUsuario();
    } else {
        // ... resto da lógica de login/logoff mantida igual ...
        if(sistemaEstadoAtual == SIS_OCIOSO && !usuarioLogado.has_value()){
          iniciaEmprestimo(usuario);
        }else if (usuarioLogado.has_value() && memcmp(usuarioLogado->uid, usuario->uid, TAG_LENGTH) == 0){
          finalizaEmprestimo(usuario);
        } else{
          displayShowTempMensagem("Erro: duplo login", 1000);
        }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

Usuario* getUsuarioAtual() {
    if (usuarioLogado.has_value()) {
        return &(*usuarioLogado);
    }
    return nullptr;
}

void rejectUsuario(){
    sistemaEstadoAtual = SIS_OCIOSO;
    usuarioEstadoAtual = NAUTORIZADO; // Usando enum do models.h diretamente se possível, ou qualificar
    buzzerSoundRFIDFail();
    displayShowMensagem("NAO AUTORIZADO");
    delay(2000);
    usuarioEstadoAtual = NONE;
}

void iniciaEmprestimo(Usuario* usuario) {
    sistemaEstadoAtual = SIS_OCUPADO;
    usuarioEstadoAtual = AUTORIZADO;
    buzzerSoundRFIDSuccess();
    
    String mensagem = "Bem vindo,\n";
    mensagem += usuario->nome;
    displayShowTempMensagem(mensagem, 2000); // Aumentei um pouco o tempo de boas vindas
    
    usuarioLogado = *usuario; // Cópia segura da struct
    sendOperacaoUsuario(usuario, "login");
    
    displayShowMensagem("Abra a \nporta");
    liberaFechadura();
}

void finalizaEmprestimo(Usuario* usuario){
    sistemaEstadoAtual = SIS_OCIOSO;
    usuarioEstadoAtual = NONE;
    buzzerSoundLogoff();
    
    String mensagem = "Obrigado,\n";
    mensagem += usuario->nome;
    displayShowTempMensagem(mensagem, 2000);
    
    usuarioLogado = std::nullopt;
    sendOperacaoUsuario(usuario, "logoff");
    travaFechadura();
    marcaDirty();

    flushLogQueue();
}
