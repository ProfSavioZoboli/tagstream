#ifndef BUZZER_HANDLER_H
#define BUZZER_HANDLER_H

#include <Arduino.h>

// Configura o pino do buzzer
void setupBuzzer();

// Toca um tom simples por um número de vezes
void buzzerTocarSinal(int rounds, int tom);

// --- SONS FUNCIONAIS (NOVOS) ---
void buzzerSoundBoot();             // Inicialização do sistema
void buzzerSoundRFIDSuccess();      // Leitura de tag OK
void buzzerSoundRFIDFail();         // Tag desconhecida/erro leitura
void buzzerSoundOperationSuccess(); // Empréstimo/Devolução OK
void buzzerSoundOperationFail();    // Erro na operação (ex: já emprestado)
void buzzerSoundLogoff();           // Usuário saiu

// Melodias complexas (movidas de melodias.h)
void buzzerTocarMusicaZelda();
void buzzerTocarCantinaBand();
void buzzerTocarMarioMoeda();
void buzzerTocarSomDeFalha();
void buzzerTocarStarWars();

#endif