// Arquivo: rfid_handler.cpp

#include <MFRC522.h>
#include "rfid_handler.h"
#include "config.h"
#include "feedback_handler.h"
#include "melodias.h"

// Variáveis globais do .ino que este arquivo precisa conhecer
extern MFRC522 rfid;
extern Usuario usuariosAutorizados[MAX_TAGS];
extern int numeroDeTagsAutorizadas;
extern enum SistemaEstado { OCIOSO,
                            OCUPADO } sistemaEstadoAtual;
extern enum UsuarioEstado { NONE,
                            AUTORIZADO,
                            NAUTORIZADO } usuarioEstadoAtual;
extern unsigned long tempoEstadoMudou;

void readRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Usuario* usuario = getUsuarioAutorizado(rfid.uid.uidByte, rfid.uid.size);
    if (usuario != nullptr) {
      sistemaEstadoAtual = SistemaEstado::OCUPADO;
      usuarioEstadoAtual = UsuarioEstado::AUTORIZADO;
      String mensagem = "Bem vindo, ";
      mensagem += usuario->nome;
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
Usuario* getUsuarioAutorizado(byte* uid, byte uidSize) {
  Usuario* usuario = getUsuario(uid, uidSize);
  return usuario;
}
Usuario* getUsuario(byte* uid, byte uidSize) {
  for (int i = 0; i < numeroDeTagsAutorizadas; i++) {
    if (memcmp(uid, usuariosAutorizados[i].uid, uidSize) == 0) {
      return &usuariosAutorizados[i];  // Retorna o endereço da struct encontrada
    }
  }
  return nullptr;  // Retorna nulo se não encontrou
}