#include <Arduino.h>
#include "locker_handler.h"

void liberaFechadura(){
  Serial.println("Liberou fechadura");
  digitalWrite(RELE,HIGH);
}

void travaFechadura(){
  Serial.println("Fechou fechadura");
  digitalWrite(RELE,LOW);
}