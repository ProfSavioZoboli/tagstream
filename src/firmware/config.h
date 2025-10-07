// Arquivo: config.h

#ifndef CONFIG_H
#define CONFIG_H

// --- DEFINIÇÕES DE PINOS ---
#define BUZZER 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_ADDRESS 0x3C
const byte SS_PIN = 5;
const byte RST_PIN = 2;


// --- CONFIGURAÇÕES GLOBAIS DO SISTEMA ---
const int MAX_TAGS = 20;
const int TAG_LENGTH = 4;
const int tempoDeExibicao = 3000;
const int MAX_EQUIPAMENTOS = 35;

#endif