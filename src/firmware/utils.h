#ifndef UTILS_H
#define UTILS_H

long long getTimestampAtual();
bool hexStringToByteArray(const char* hexStr, byte* byteArray, int arraySize);
void byteArrayToHexString(byte* byteArray, int arraySize, char* outputBuffer, int bufferSize);


#endif