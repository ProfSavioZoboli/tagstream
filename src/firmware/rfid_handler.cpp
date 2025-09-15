// Arquivo: rfid_handler.cpp

#include <MFRC522.h>
#include "rfid_handler.h"
#include "config.h"
#include "feedback_handler.h"
#include "melodias.h"
#include <optional>

// Variáveis globais do .ino que este arquivo precisa conhecer
extern MFRC522 rfid;
extern Usuario usuariosAutorizados[MAX_TAGS];
extern int numeroDeTagsAutorizadas;
extern enum SistemaEstado { OCIOSO,
                            OCUPADO } sistemaEstadoAtual;
extern enum UsuarioEstado { NONE,
                            AUTORIZADO,
                            NAUTORIZADO } usuarioEstadoAtual;
std::optional<Usuario> usuarioLogado;
extern unsigned long tempoEstadoMudou;

void readRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Usuario* usuario = getUsuarioAutorizado(rfid.uid.uidByte, rfid.uid.size);
    if(usuario == nullptr){
      rejectUsuario();
      return;
    }
    if(sistemaEstadoAtual == OCIOSO && !usuarioLogado.has_value()){
      //Irá logar para iniciar uma leitura
      iniciaEmprestimo(usuario);
    }else if (usuarioLogado == usuario){
      //Irá deslogar
      finalizaEmprestimo(usuario);
    } else{
      showMensagem("Já há um usuário logado, impossível continuar");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

void rejectUsuario(){
    sistemaEstadoAtual = SistemaEstado::OCIOSO;
    usuarioEstadoAtual = UsuarioEstado::NAUTORIZADO;
    showMensagem("NAO AUTORIZADO");
    configLedsEstado();
    tocarSomDeFalha();
    delay(2000);
    usuarioEstadoAtual = UsuarioEstado::NONE;
}

void iniciaEmprestimo(Usuario* usuario) {
    sistemaEstadoAtual = SistemaEstado::OCUPADO;
    usuarioEstadoAtual = UsuarioEstado::AUTORIZADO;
    String mensagem = "Bem vindo, ";
    mensagem += usuario->nome;
    showMensagem(mensagem);
    usuarioLogado = *usuario;
    configLedsEstado();
    tocarMarioMoeda();
}

void finalizaEmprestimo(Usuario* usuario){
  sistemaEstadoAtual = SistemaEstado::OCIOSO;
    usuarioEstadoAtual = UsuarioEstado::NONE;
    String mensagem = "Obrigado, ";
    mensagem += usuario->nome;
    showMensagem(mensagem);
    usuarioLogado = nullopt;
    configLedsEstado();
    tocarMarioMoeda();
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