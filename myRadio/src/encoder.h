/**
 * @file encoder.h
 * @brief Модуль для керування енкодером
 * Використовує бібліотеку Ai Esp32 Rotary Encoder
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

// Піни енкодера (з зовнішньою підтяжкою до 3.3V)
#define ENC_A_PIN       13      // GPIO13
#define ENC_B_PIN       14      // GPIO14
#define ENC_BTN_PIN     19      // Кнопка енкодера
#define ENC_PULSES_PER_STEP 4   // Кількість імпульсів на крок (для стабільності)

// Стан енкодера
typedef struct {
    int position;           // Поточна позиція
    int lastPosition;       // Остання прочитана позиція
    bool changed;           // Прапорець зміни
    bool buttonPressed;     // Кнопка натиснута
    bool buttonWasPressed;  // Було натискання кнопки
    unsigned long lastChangeTime;  // Час останньої зміни
} EncoderState;

/**
 * @brief Ініціалізація енкодера
 */
void encoderInit();

/**
 * @brief Опитування енкодера (викликати в loop)
 */
void encoderLoop();

/**
 * @brief Отримати поточну позицію енкодера
 * @return Позиція енкодера
 */
int encoderGetPosition();

/**
 * @brief Отримати зміну позиції енкодера
 * @return Зміна позиції (позитивне = обертання вправо, негативне = вліво)
 */
int encoderGetChange();

/**
 * @brief Скинути лічильник змін
 */
void encoderResetChange();

/**
 * @brief Встановити позицію енкодера
 * @param position Нова позиція
 */
void encoderSetPosition(int position);

/**
 * @brief Перевірити чи була натиснута кнопка енкодера
 * @return true якщо було натискання
 */
bool encoderButtonWasPressed();

/**
 * @brief Перевірити чи натиснута кнопка енкодера зараз
 * @return true якщо кнопка натиснута
 */
bool encoderButtonIsPressed();

#endif // ENCODER_H
