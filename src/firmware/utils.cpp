#include <Arduino.h>
#include <chrono>


long long getTimestampAtual(){
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return milis;
}