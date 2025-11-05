#include "log_repository.h"
#include <Preferences.h>

#define MAX_LOGS 50 // Aumentei para garantir que cabe uma sessão grande

static LogEntry logs[MAX_LOGS];
static int count = 0; // Simplifiquei para uma lista linear já que vamos limpar tudo de uma vez

static Preferences preferences;
const char* PREF_LOG_NS = "log_data";

void logRepoSaveState() {
    if (preferences.begin(PREF_LOG_NS, false)) {
        preferences.putBytes("logs", logs, sizeof(logs));
        preferences.putInt("count", count);
        preferences.end();
    }
}

void logRepoInit() {
    preferences.begin(PREF_LOG_NS, true);
    if (!preferences.isKey("logs")) {
        preferences.end();
        count = 0;
        logRepoSaveState();
    } else {
        preferences.end();
        preferences.begin(PREF_LOG_NS, false);
        preferences.getBytes("logs", logs, sizeof(logs));
        count = preferences.getInt("count", 0);
        preferences.end();
        if (count > 0) {
             Serial.print("LogRepo: Restaurados "); Serial.print(count); Serial.println(" logs pendentes.");
        }
    }
}

bool logRepoEnqueue(byte* userUid, int equipId, SituacaoEquipamento status, unsigned long long timestamp) {
    if (count >= MAX_LOGS) return false;

    memcpy(logs[count].userUid, userUid, TAG_LENGTH);
    logs[count].equipId = equipId;
    logs[count].novoStatus = status;
    logs[count].timestamp = timestamp;
    count++;
    
    logRepoSaveState();
    return true;
}

int logRepoCountPending() {
    return count;
}

bool logRepoPeek(int index, LogEntry* output) {
    if (index < 0 || index >= count) return false;
    *output = logs[index];
    return true;
}

void logRepoClear() {
    count = 0;
    logRepoSaveState();
    Serial.println("LogRepo: Fila limpa.");
}

void logRepoFactoryReset() {
    if (preferences.begin(PREF_LOG_NS, false)) {
        preferences.clear();
        preferences.end();
        Serial.println("LogRepo: FACTORY RESET COMPLETO.");
    }
}