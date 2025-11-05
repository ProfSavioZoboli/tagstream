#include "buzzer_handler.h"
#include "config.h"
#include "pitches.h"

void setupBuzzer() {
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW); // Garante que começa desligado
}

void buzzerTocarSinal(int rounds, int tom) {
    for (int i = 0; i < rounds; i++) {
        tone(BUZZER, tom, 100);
        delay(100); // Espera o tom terminar
        delay(100); // Pausa entre beeps
    }
}

// =========================================
// NOVOS SONS FUNCIONAIS (UX MODERNIZADA)
// =========================================

void buzzerSoundBoot() {
    // Arpejo ascendente rápido (C5 -> E5 -> G5 -> C6)
    tone(BUZZER, NOTE_C5, 100); delay(120);
    tone(BUZZER, NOTE_E5, 100); delay(120);
    tone(BUZZER, NOTE_G5, 100); delay(120);
    tone(BUZZER, NOTE_C6, 250); delay(250);
    noTone(BUZZER);
}

void buzzerSoundRFIDSuccess() {
    // "Bip-bip" agudo e rápido (como um scanner de mercado)
    tone(BUZZER, NOTE_A6, 80); delay(100);
    tone(BUZZER, NOTE_A6, 80); delay(80);
    noTone(BUZZER);
}

void buzzerSoundRFIDFail() {
    // Som grave e longo de negação
    tone(BUZZER, NOTE_G2, 400); delay(400);
    noTone(BUZZER);
}

void buzzerSoundOperationSuccess() {
    // "Cha-ching!" positivo (salto de quinta perfeita)
    tone(BUZZER, NOTE_C6, 150); delay(150);
    tone(BUZZER, NOTE_G6, 300); delay(300);
    noTone(BUZZER);
}

void buzzerSoundOperationFail() {
    // "Uh-oh" descendente (terça menor)
    tone(BUZZER, NOTE_E5, 200); delay(250);
    tone(BUZZER, NOTE_CS5, 400); delay(400);
    noTone(BUZZER);
}

void buzzerSoundLogoff() {
    // Oposto do boot: arpejo descendente suave
    tone(BUZZER, NOTE_G5, 150); delay(150);
    tone(BUZZER, NOTE_E5, 150); delay(150);
    tone(BUZZER, NOTE_C5, 300); delay(300);
    noTone(BUZZER);
}

// --- IMPLEMENTAÇÃO DAS MELODIAS ---
// (Copiado e adaptado de melodias.cpp para manter o padrão de nomenclatura interno se desejar, 
//  ou pode manter os nomes originais das funções internas. Vou manter a lógica original para facilitar.)

void buzzerTocarMusicaZelda() {
  int melody[] = { NOTE_G4, NOTE_FS4, NOTE_DS4, NOTE_A4, NOTE_GS4, NOTE_E4, NOTE_GS4, NOTE_C5 };
  int noteDurations[] = { 8, 8, 8, 4, 8, 4, 8, 2 };
  // Serial.println("Tocando Zelda..."); // Opcional: remover serial de handlers de baixo nível
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZER, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER);
  }
}

void buzzerTocarMarioMoeda() {
  int tempo = 200;
  int melody[] = { NOTE_E5, NOTE_G5, NOTE_E6, NOTE_C6, NOTE_D6, NOTE_G6 };
  int noteDurations[] = { 12, 12, 12, 12, 12, 6 };
  int songLength = sizeof(melody) / sizeof(melody[0]);
  int wholenote = (60000 * 4) / tempo;
  for (int thisNote = 0; thisNote < songLength; thisNote++) {
    int divider = noteDurations[thisNote];
    int noteDuration = (divider > 0) ? (wholenote / divider) : (wholenote / abs(divider) * 1.5);
    tone(BUZZER, melody[thisNote], noteDuration * 0.9);
    delay(noteDuration);
    noTone(BUZZER);
  }
}

void buzzerTocarSomDeFalha() {
  int melody[] = { NOTE_G3, NOTE_FS3, NOTE_F3, NOTE_E3 };
  int noteDurations[] = { 12, 12, 12, 6 };
  for (int i = 0; i < 4; i++) {
      int duration = 1200 / noteDurations[i];
      tone(BUZZER, melody[i], duration);
      delay(duration * 1.2);
      noTone(BUZZER);
  }
}

void buzzerTocarStarWars() {
   // --- DADOS DA MÚSICA ---
  // Velocidade da música em batidas por minuto (BPM)
  int tempo = 108;

  // Melodia em formato intercalado: {nota, duração, nota, duração, ...}
  // Duração: 4 = semínima, 8 = colcheia, etc.
  // Duração negativa = nota pontuada (dura 1.5x mais)
  int melody[] = {
    NOTE_AS4,8, NOTE_AS4,8, NOTE_AS4,8,
    NOTE_F5,2, NOTE_C6,2,
    NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,  
    NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,  
    NOTE_AS5,8, NOTE_A5,8, NOTE_AS5,8, NOTE_G5,2
  };

  // --- LÓGICA DE REPRODUÇÃO ---
  Serial.println("Tocando o tema de Star Wars...");

  // Calcula o número de pares (nota + duração) na melodia
  int notes = sizeof(melody) / sizeof(melody[0]) / 2;

  // Calcula a duração de uma semibreve (nota inteira) em milissegundos
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  // Itera sobre a melodia, pulando de 2 em 2 para pegar a nota e sua duração
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
    divider = melody[thisNote + 1];
    if (divider > 0) {
      noteDuration = wholenote / divider;
    } else if (divider < 0) {
      noteDuration = wholenote / abs(divider);
      noteDuration *= 1.5; // Aumenta a duração para notas pontuadas
    }

    // Toca a nota por 90% da sua duração para criar um efeito staccato
    tone(BUZZER, melody[thisNote], noteDuration * 0.9);

    // Aguarda a duração completa da nota antes de ir para a próxima
    delay(noteDuration);
    
    // Para a geração do som
    noTone(BUZZER);
  }
  Serial.println("Música finalizada."); 
  }