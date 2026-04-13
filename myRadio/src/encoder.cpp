/**
 * @file encoder.cpp
 * @brief Модуль для керування енкодером
 * Використовує бібліотеку Ai Esp32 Rotary Encoder
 */

#include "encoder.h"
#include <AiEsp32RotaryEncoder.h>

// Глобальний об'єкт енкодера
static AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(
    ENC_A_PIN, 
    ENC_B_PIN, 
    -1,     // Button pin (не використовується, обробляємо окремо)
    -1,     // Number of pulses (не використовується)
    ENC_PULSES_PER_STEP
);

// Стан енкодера
static EncoderState encoder = {
    .position = 0,
    .lastPosition = 0,
    .changed = false,
    .buttonPressed = false,
    .buttonWasPressed = false,
    .lastChangeTime = 0
};

// Для обробки кнопки енкодера
static bool encBtnLastState = true;
static bool encBtnCurrentState = true;
static unsigned long encBtnLastDebounceTime = 0;
static unsigned long encBtnPressTime = 0;
static bool encBtnLongPressHandled = false;

// ISR callback
static void IRAM_ATTR readEncoderISR() {
    rotaryEncoder.readEncoder_ISR();
}

void encoderInit() {
    Serial.println(F("[Encoder] Initializing..."));

    // Ініціалізація енкодера
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(-32768, 32768, false);
    rotaryEncoder.setAcceleration(250);
    rotaryEncoder.reset();

    // Кнопка енкодера (зовнішня підтяжка)
    pinMode(ENC_BTN_PIN, INPUT);
    encBtnLastState = digitalRead(ENC_BTN_PIN);
    encBtnCurrentState = encBtnLastState;

    Serial.printf("[Encoder] A -> GPIO%d\n", ENC_A_PIN);
    Serial.printf("[Encoder] B -> GPIO%d\n", ENC_B_PIN);
    Serial.printf("[Encoder] Button -> GPIO%d\n", ENC_BTN_PIN);
    Serial.printf("[Encoder] Pulses per step: %d\n", ENC_PULSES_PER_STEP);
    Serial.println(F("[Encoder] Initialized with Ai Esp32 Rotary Encoder library"));
}

void encoderLoop() {
    unsigned long currentTime = millis();

    // ========================================================================
    // Обробка енкодера (читання з бібліотеки)
    // ========================================================================
    int newPosition = rotaryEncoder.readEncoder();
    
    if (newPosition != encoder.position) {
        encoder.changed = true;
        encoder.lastChangeTime = currentTime;
        encoder.position = newPosition;
    }

    // ========================================================================
    // Обробка кнопки енкодера
    // ========================================================================
    bool encBtnReading = digitalRead(ENC_BTN_PIN);

    if (encBtnReading != encBtnLastState) {
        encBtnLastDebounceTime = currentTime;
    }

    encBtnLastState = encBtnReading;

    if ((currentTime - encBtnLastDebounceTime) > ENC_BTN_DEBOUNCE_MS) {
        bool newState = (encBtnReading == LOW);  // Активний низький рівень

        // Детектування натискання
        if (newState && !encBtnCurrentState) {
            encBtnPressTime = currentTime;
            encBtnLongPressHandled = false;
        }

        // Перевірка на довге натискання
        if (newState && encBtnCurrentState) {
            if ((currentTime - encBtnPressTime) > ENC_LONG_PRESS_MS) {
                if (!encBtnLongPressHandled) {
                    encBtnLongPressHandled = true;
                }
            }
        }

        // Детектування відпускання (коротке натискання)
        if (!newState && encBtnCurrentState) {
            if ((currentTime - encBtnPressTime) <= ENC_LONG_PRESS_MS) {
                if (!encBtnLongPressHandled) {
                    encoder.buttonWasPressed = true;
                }
            }
        }

        encBtnCurrentState = newState;
        encoder.buttonPressed = newState;
    }
}

int encoderGetPosition() {
    return encoder.position;
}

int encoderGetChange() {
    if (encoder.changed) {
        int change = encoder.position - encoder.lastPosition;
        encoder.lastPosition = encoder.position;
        encoder.changed = false;
        return change;
    }
    return 0;
}

void encoderResetChange() {
    encoder.changed = false;
    encoder.lastPosition = encoder.position;
}

void encoderSetPosition(int position) {
    encoder.position = position;
    encoder.lastPosition = position;
    encoder.changed = false;
    rotaryEncoder.reset();
    rotaryEncoder.readEncoder();  // Прочитати поточне значення
}

bool encoderButtonWasPressed() {
    bool result = encoder.buttonWasPressed;
    encoder.buttonWasPressed = false;
    return result;
}

bool encoderButtonIsPressed() {
    return encoder.buttonPressed;
}
