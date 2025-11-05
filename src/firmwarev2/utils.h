#ifndef UTILS_H
#define UTILS_H

#include "emprestimos.h"

long long getTimestampAtual();
bool hexStringToByteArray(const char* hexStr, byte* byteArray, int arraySize);
void byteArrayToHexString(byte* byteArray, int arraySize, char* outputBuffer, int bufferSize);

const char* situacaoParaString(SituacaoEquipamento s);

void getISOTimeString(char* buffer, size_t bufferSize);

#endif