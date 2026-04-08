/**
 * @file ir_remote.cpp
 * @brief Реалізація модуля для керування ІЧ пультом RC-5
 */

#include "ir_remote.h"
#include <IRrecv.h>
#include <IRutils.h>

// Об'єкт для прийому ІЧ сигналів
static IRrecv irReceiver(IR_RECEIVER_PIN);
static decode_results irResults;

// Остання натиснута кнопка
static uint8_t lastKey = 0xFF;
static unsigned long lastKeyTime = 0;
static const unsigned long IR_REPEAT_MS = 200;  // Затримка між повторними натисканнями

void irRemoteInit() {
    Serial.println(F("[IR] Initializing IR receiver..."));
    Serial.printf("[IR] IR receiver pin: GPIO %d\n", IR_RECEIVER_PIN);
    
    // Ініціалізація приймача
    irReceiver.enableIRIn();
    
    Serial.println(F("[IR] IR receiver ready (RC-5 protocol)"));
}

void irRemoteLoop() {
    // Перевіряємо чи є ІЧ сигнал
    if (irReceiver.decode(&irResults)) {
        // Перевіряємо чи це RC-5 протокол
        if (irResults.decode_type == RC5 || irResults.decode_type == RC5X) {
            // Витягуємо код кнопки (молодший байт)
            uint8_t keyCode = irResults.value & 0xFF;
            
            // Ігноруємо повторні натискання якщо занадто швидко
            unsigned long now = millis();
            if (now - lastKeyTime < IR_REPEAT_MS) {
                irReceiver.resume();
                return;
            }
            
            lastKey = keyCode;
            lastKeyTime = now;
            
            Serial.printf("[IR] RC-5 key received: 0x%02X\n", keyCode);
        }
        
        // Продовжуємо прийом
        irReceiver.resume();
    }
}

uint8_t irRemoteGetLastKey() {
    // Повертаємо і скидаємо код кнопки
    uint8_t key = lastKey;
    lastKey = 0xFF;  // Скидаємо
    return key;
}
