#include <Arduino.h>
#include "locker_handler.h"
#include "config.h"

bool situacaoFechadura;

void setupFechadura(){
  pinMode(RELE,OUTPUT);
  digitalWrite(RELE,LOW);
  situacaoFechadura = false;
}

void liberaFechadura(){
  Serial.println("Liberou fechadura");
  digitalWrite(RELE,HIGH);
  situacaoFechadura = true;
}

void travaFechadura(){
  Serial.println("Fechou fechadura");
  digitalWrite(RELE,LOW);
  situacaoFechadura = false;
}

bool isTravaAberta(){
  return situacaoFechadura;
}

