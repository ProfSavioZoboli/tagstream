#include "equipment_repository.h"
#include <Preferences.h>

static Equipamento inventario[MAX_EQUIPAMENTOS];
static Preferences preferences;
const char* PREF_EQ_NAMESPACE = "equip_data";

bool equipRepoInit() {
    preferences.begin(PREF_EQ_NAMESPACE, false);
    size_t lido = preferences.getBytes("inv", inventario, sizeof(inventario));
    bool dadosExistentes = (lido == sizeof(inventario));
    
    if (!dadosExistentes) {
        Serial.println("EquipRepo: Flash vazia ou invalida. Iniciando com padroes.");
        for (int i = 0; i < MAX_EQUIPAMENTOS; i++) {
            inventario[i].numero = i;
            inventario[i].situacao = EQP_LIVRE;
        }
        // Não salvamos imediatamente para não desgastar flash se não houver mudanças reais,
        // mas poderia salvar aqui se quiser garantir que a próxima boot seja "não-vazio".
    } else {
        Serial.println("EquipRepo: Inventario carregado da Flash.");
    }
    preferences.end();
    return dadosExistentes;
}

void equipRepoSave() {
    if (preferences.begin(PREF_EQ_NAMESPACE, false)) {
        preferences.putBytes("inv", inventario, sizeof(inventario));
        preferences.end();
        Serial.println("EquipRepo: Estado salvo na Flash.");
    } else {
        Serial.println("EquipRepo: ERRO ao salvar na Flash.");
    }
}

SituacaoEquipamento equipRepoGetStatus(int numero) {
    if (numero < 0 || numero >= MAX_EQUIPAMENTOS) return EQP_MANUTENCAO;
    return inventario[numero].situacao;
}

bool equipRepoUpdateStatus(int numero, SituacaoEquipamento novaSituacao, bool salvarAgora) {
    if (numero < 0 || numero >= MAX_EQUIPAMENTOS) return false;
    
    inventario[numero].situacao = novaSituacao;
    // Garante integridade do número
    inventario[numero].numero = numero; 
    
    if (salvarAgora) {
        equipRepoSave();
    }
    return true;
}

void equipRepoUpdateAll(const Equipamento* novoInventario) {
    memcpy(inventario, novoInventario, sizeof(inventario));
    equipRepoSave();
}

void equipRepoFactoryReset() {
    if (preferences.begin(PREF_EQ_NAMESPACE, false)) {
        preferences.clear();
        preferences.end();
        Serial.println("EquipRepo: FACTORY RESET COMPLETO.");
    }
}