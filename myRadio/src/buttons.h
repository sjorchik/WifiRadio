/**
 * @file buttons.h
 * @brief Модуль для керування кнопками на базі OneButton
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include <OneButton.h>
#include "config.h"

// Піни кнопок (з зовнішньою підтяжкою до 3.3V)

// Ідентифікатори кнопок
typedef enum {
    BTN_PWR,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_UP,
    BTN_DOWN,
    BTN_OK,
    BTN_COUNT
} ButtonId;

// Прапорці подій для кожної кнопки
typedef struct {
    bool clicked;       // Коротке натискання
    bool longPressed;   // Довге натискання
} ButtonFlags;

/**
 * @brief Ініціалізація кнопок
 */
void buttonsInit();

/**
 * @brief Опитування кнопок (викликати в loop)
 */
void buttonsLoop();

/**
 * @brief Перевірити чи була коротке натискання кнопки
 * @param id Ідентифікатор кнопки
 * @return true якщо було коротке натискання
 */
bool buttonsWasClicked(ButtonId id);

/**
 * @brief Перевірити чи було довге натискання кнопки
 * @param id Ідентифікатор кнопки
 * @return true якщо було довге натискання
 */
bool buttonsWasLongPressed(ButtonId id);

/**
 * @brief Скинути прапорець короткого натискання
 * @param id Ідентифікатор кнопки
 */
void buttonsClearClicked(ButtonId id);

/**
 * @brief Скинути прапорець довгого натискання
 * @param id Ідентифікатор кнопки
 */
void buttonsClearLongPressed(ButtonId id);

/**
 * @brief Отримати назву кнопки
 * @param id Ідентифікатор кнопки
 * @return Назва кнопки
 */
const char* buttonsGetName(ButtonId id);

#endif // BUTTONS_H
