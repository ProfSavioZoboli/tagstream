// Arquivo: rfid_handler.cpp

#include <MFRC522.h>
#include "rfid_handler.h"
#include "config.h"
#include "feedback_handler.h"
#include "melodias.h"
#include <optional>
#include "mqtt_handler.h" //Permite enviar solicitações MQTT
#include "utils.h"

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
    }else if (memcmp(usuarioLogado->uid,usuario->uid,4)==0){
      //Irá deslogar
      finalizaEmprestimo(usuario);
    } else{
      //Se for autorizado mas tiver outro usuário logado no momento, terá que esperar o que está logado deslogar.
      showTempMensagem("Erro: duplo login",1000);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

void rejectUsuario(){
    sistemaEstadoAtual = SistemaEstado::OCIOSO;
    usuarioEstadoAtual = UsuarioEstado::NAUTORIZADO;
    tocarSomDeFalha();
    showMensagem("NAO AUTORIZADO");
    configLedsEstado();
    delay(2000);
    usuarioEstadoAtual = UsuarioEstado::NONE;
}

void iniciaEmprestimo(Usuario* usuario) {
    sistemaEstadoAtual = SistemaEstado::OCUPADO;
    usuarioEstadoAtual = UsuarioEstado::AUTORIZADO;
    tocarMarioMoeda();
    String mensagem = "Bem vindo, ";
    mensagem += usuario->nome;
    showTempMensagem(mensagem,500);
    usuarioLogado = *usuario;
    sendOperacaoUsuario(usuario,"login");
    configLedsEstado();
    
}

void finalizaEmprestimo(Usuario* usuario){
  sistemaEstadoAtual = SistemaEstado::OCIOSO;
    usuarioEstadoAtual = UsuarioEstado::NONE;
    tocarMarioMoeda();
    String mensagem = "Obrigado, ";
    mensagem += usuario->nome;
    showTempMensagem(mensagem,500);
    usuarioLogado = std::nullopt;
    sendOperacaoUsuario(usuario,"logoff");
    configLedsEstado();
    
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