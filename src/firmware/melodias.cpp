// Arquivo: melodias.cpp

#include <Arduino.h>
#include "melodias.h"
#include "config.h"
#include "pitches.h"

void tocarMusicaZelda() {
  // Notas da melodia "Secret Found" do Zelda
  int melody[] = {
    NOTE_G4, NOTE_FS4, NOTE_DS4, NOTE_A4, NOTE_GS4, NOTE_E4, NOTE_GS4, NOTE_C5
  };

  // Duração de cada nota: 4 = semínima, 8 = colcheia, etc.
  int noteDurations[] = {
    8, 8, 8, 4, 8, 4, 8, 2
  };

  Serial.println("Tocando a música de sucesso do Zelda...");

  for (int thisNote = 0; thisNote < 8; thisNote++) {
    // Calcula a duração da nota em milissegundos
    int noteDuration = 1000 / noteDurations[thisNote];

    // Toca a nota
    // tone(pino, frequencia, duracao)
    tone(BUZZER, melody[thisNote], noteDuration);

    // Adiciona uma pequena pausa entre as notas para que não se misturem
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    // Para a reprodução da nota (opcional, mas bom para garantir)
    noTone(BUZZER);
  }
  Serial.println("Música finalizada.");
}

void tocarCantinaBand() {
  // A primeira parte da melodia da Cantina Band
  int melody[] = {
    // Frase 1
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_G4, NOTE_FS4, NOTE_E4, NOTE_D4,
    // Frase 2
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_G4, NOTE_FS4, NOTE_E4, NOTE_D4,
    // Frase 3 (a parte que sobe)
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5, NOTE_CS5,
    // Frase 4 (repetição da 2)
    NOTE_A4, NOTE_D5, NOTE_A4, NOTE_D5, NOTE_A4, NOTE_G4, NOTE_FS4, NOTE_E4, NOTE_D4
  };

  // Duração de cada nota
  // 8 = colcheia, 4 = semínima. Negativo = nota pontuada.
  int noteDurations[] = {
    // Frase 1
    8, 4, 8, 4, 8, 8, 8, 8, 4,
    // Frase 2
    8, 4, 8, 4, 8, 8, 8, 8, 4,
    // Frase 3
    8, 4, 8, 4, 8, 8, 8, 8, 4,
    // Frase 4
    -4, 8, 8, 8, 8, 8, 8, 8, 4
  };

  Serial.println("Tocando a Cantina Band...");

  // Calcula o número total de notas na melodia
  int songLength = sizeof(melody) / sizeof(melody[0]);

  for (int thisNote = 0; thisNote < songLength; thisNote++) {
    // Calcula a duração base da nota. abs() remove o sinal negativo para o cálculo.
    int noteDuration = 1000 / abs(noteDurations[thisNote]);

    // Se a duração for negativa no array, é uma nota pontuada (dura 1.5x mais)
    if (noteDurations[thisNote] < 0) {
      noteDuration *= 1.5;
    }

    // Toca a nota
    tone(BUZZER, melody[thisNote], noteDuration);

    // Pausa entre as notas para um som mais limpo
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    // Para a reprodução
    noTone(BUZZER);
  }
  Serial.println("Música finalizada.");
}

void tocarMarioMoeda() { // Mantive o nome da função para não precisar mudar em outros lugares
  // --- DADOS DA MÚSICA ---
  // Um tempo bem rápido, característico do som
  int tempo = 200;

  // Melodia "1-Up" do Super Mario Bros.
  int melody[] = {
    NOTE_E5, NOTE_G5, NOTE_E6, NOTE_C6, NOTE_D6, NOTE_G6
  };

  // Duração de cada nota. A última nota é um pouco mais longa que as outras.
  // 12 é uma tercina (triplet), 6 é o dobro da duração.
  int noteDurations[] = {
    12, 12, 12, 12, 12, 6
  };
  
  // --- LÓGICA DE REPRODUÇÃO (baseada em BPM) ---
  Serial.println("Tocando som de 1-Up do Mario...");

  int songLength = sizeof(melody) / sizeof(melody[0]);

  // Calcula a duração de uma semibreve (nota inteira) em milissegundos
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  for (int thisNote = 0; thisNote < songLength; thisNote++) {
    divider = noteDurations[thisNote];
    if (divider > 0) {
      noteDuration = wholenote / divider;
    } else if (divider < 0) {
      noteDuration = wholenote / abs(divider);
      noteDuration *= 1.5;
    }

    // Toca a nota por 90% de sua duração para um som mais "seco"
    tone(BUZZER, melody[thisNote], noteDuration * 0.9);

    // Aguarda a duração completa da nota
    delay(noteDuration);
    
    noTone(BUZZER);
  }
}

void tocarSomDeFalha() {
  // Melodia grave e descendente para indicar falha
  int melody[] = {
    NOTE_G3, NOTE_FS3, NOTE_F3, NOTE_E3
  };

  // Duração de cada nota. A última é mais longa para dar um final.
  int noteDurations[] = {
    12, 12, 12, 6
  };

  int songLength = sizeof(melody) / sizeof(melody[0]);

  for (int thisNote = 0; thisNote < songLength; thisNote++) {
    int noteDuration = 1200 / noteDurations[thisNote];  // Um pouco mais lento que o normal
    tone(BUZZER, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.2;
    delay(pauseBetweenNotes);

    noTone(BUZZER);
  }
}

void tocarStarWars() {
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