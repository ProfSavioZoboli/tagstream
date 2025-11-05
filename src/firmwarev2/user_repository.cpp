#include "user_repository.h"
#include "config.h"
#include "utils.h"
#include <Preferences.h>

// --- DADOS PRIVADOS DO MÓDULO ---
static Usuario usuariosAutorizados[MAX_TAGS];
static int numeroDeTagsAutorizadas = 0;
static unsigned long long timestampListaLocal = 0;

static Preferences preferences;

// Nome do namespace na flash (max 15 chars)
const char* PREF_NAMESPACE = "rfid_data";

void userRepoInit() {
    // Abre em modo leitura/escrita (false)
    preferences.begin(PREF_NAMESPACE, false);

    // Carrega o timestamp
    // Nota: getULong64 pode não estar disponível em versões antigas do core ESP32.
    // Se der erro, usaremos duas ULongs ou converteremos para string.
    timestampListaLocal = preferences.getULong64("ts", 0);

    // Carrega a contagem
    int count = preferences.getInt("count", 0);
    if (count > MAX_TAGS) count = MAX_TAGS; // Proteção contra corrupção

    // Carrega o blob de dados
    size_t lido = preferences.getBytes("users", usuariosAutorizados, sizeof(usuariosAutorizados));
    
    // Validação básica
    if (lido > 0) {
        numeroDeTagsAutorizadas = count;
        Serial.print("Repo: Carregados ");
        Serial.print(numeroDeTagsAutorizadas);
        Serial.println(" usuarios da Flash.");
    } else {
        numeroDeTagsAutorizadas = 0;
        Serial.println("Repo: Nenhum usuario na Flash ou falha na leitura.");
    }

    preferences.end();
}

void userRepoSave() {
    Serial.println("Repo: Salvando dados na Flash...");
    if (preferences.begin(PREF_NAMESPACE, false)) {
        preferences.putULong64("ts", timestampListaLocal);
        preferences.putInt("count", numeroDeTagsAutorizadas);
        // Salva apenas os bytes úteis (numeroDeTags * tamanho da struct)
        preferences.putBytes("users", usuariosAutorizados, numeroDeTagsAutorizadas * sizeof(Usuario));
        preferences.end();
        Serial.println("Repo: Dados salvos com sucesso.");
    } else {
        Serial.println("Repo: ERRO ao abrir Flash para escrita.");
    }
}

void userRepoClear() {
    numeroDeTagsAutorizadas = 0;
    // Opcional: zerar memória por segurança
    memset(usuariosAutorizados, 0, sizeof(usuariosAutorizados));
}

bool userRepoAdd(const char* uidHex, const char* nome) {
    if (numeroDeTagsAutorizadas >= MAX_TAGS) {
        return false;
    }

    Usuario* u = &usuariosAutorizados[numeroDeTagsAutorizadas];
    
    // Usa a função utilitária já existente para converter Hex -> Byte Array
    if (!hexStringToByteArray(uidHex, u->uid, TAG_LENGTH)) {
        return false; // UID inválido
    }

    // Copia o nome com segurança
    strncpy(u->nome, nome, sizeof(u->nome) - 1);
    u->nome[sizeof(u->nome) - 1] = '\0';

    numeroDeTagsAutorizadas++;
    return true;
}

Usuario* userRepoFind(byte* uid, byte uidSize) {
    for (int i = 0; i < numeroDeTagsAutorizadas; i++) {
        // Assume que TAG_LENGTH é o tamanho correto para comparação
        if (memcmp(uid, usuariosAutorizados[i].uid, TAG_LENGTH) == 0) {
            return &usuariosAutorizados[i];
        }
    }
    return nullptr;
}

int userRepoGetCount() {
    return numeroDeTagsAutorizadas;
}

unsigned long long userRepoGetTimestamp() {
    return timestampListaLocal;
}

void userRepoSetTimestamp(unsigned long long newTimestamp) {
    timestampListaLocal = newTimestamp;
}

void userRepoFactoryReset() {
    if (preferences.begin(PREF_NAMESPACE, false)) {
        preferences.clear(); // Remove todas as chaves deste namespace
        preferences.end();
        Serial.println("UserRepo: FACTORY RESET COMPLETO.");
    }
}