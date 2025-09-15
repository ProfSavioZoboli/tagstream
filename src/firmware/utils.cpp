#include <Arduino.h>
#include <chrono>


long long getTimestampAtual(){
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return milis;
}

bool hexStringToByteArray(const char* hexStr, byte* byteArray, int arraySize) {
  int len = strlen(hexStr);
  if (len != arraySize * 2) return false;

  for (int i = 0; i < arraySize; i++) {
    char hexPair[3] = { hexStr[i * 2], hexStr[i * 2 + 1], '\0' };
    byteArray[i] = (byte)strtol(hexPair, NULL, 16);
  }
  return true;
}

// Converte um array de bytes para uma string hexadecimal
void byteArrayToHexString(byte* byteArray, int arraySize, char* outputBuffer, int bufferSize) {
    if (bufferSize < (arraySize * 2 + 1)) {
        // Buffer de saída é muito pequeno
        return;
    }
    
    for (int i = 0; i < arraySize; i++) {
        sprintf(outputBuffer + (i * 2), "%02X", byteArray[i]);
    }
    outputBuffer[arraySize * 2] = '\0'; // Adiciona o terminador nulo
}