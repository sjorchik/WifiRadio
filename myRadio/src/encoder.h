/**
 * @file encoder.h
 * @brief Модуль для керування енкодером
 * Використовує бібліотеку Ai Esp32 Rotary Encoder
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "config.h"

// Піни енкодера (з зовнішньою підтяжкою до 3.3V)

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
