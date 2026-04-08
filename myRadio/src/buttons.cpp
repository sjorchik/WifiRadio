/**
 * @file buttons.cpp
 * @brief Модуль для керування кнопками на базі OneButton
 */

#include "buttons.h"

// Конфігурація кнопок
#define LONG_PRESS_MS         1000    // Час для довгого натискання (1 сек)

// Масив об'єктів кнопок OneButton
static OneButton* buttons[BTN_COUNT] = {nullptr};

// Масив імен кнопок
static const char* buttonNames[BTN_COUNT] = {
    "PWR", "LEFT", "RIGHT", "UP", "DOWN", "OK"
};

// Масив пінів кнопок
static const int buttonPins[BTN_COUNT] = {
    BTN_PWR_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN, BTN_UP_PIN, BTN_DOWN_PIN, BTN_OK_PIN
};

// Прапорці подій для кожної кнопки
static ButtonFlags buttonFlags[BTN_COUNT];

// Обробник короткого натискання
static void clickHandler(void* param) {
    ButtonId id = (ButtonId)(intptr_t)param;
    buttonFlags[id].clicked = true;
    Serial.printf("[OneButton] %s clicked\n", buttonNames[id]);
}

// Обробник довгого натискання (тільки для кнопки OK)
static void longPressHandler(void* param) {
    ButtonId id = (ButtonId)(intptr_t)param;
    buttonFlags[id].longPressed = true;
    Serial.printf("[OneButton] %s long pressed\n", buttonNames[id]);
}

void buttonsInit() {
    Serial.println("[Buttons] Initializing with OneButton...");

    // Ініціалізація масиву прапорців
    for (int i = 0; i < BTN_COUNT; i++) {
        buttonFlags[i].clicked = false;
        buttonFlags[i].longPressed = false;
    }

    // Створення та налаштування кнопок
    for (int i = 0; i < BTN_COUNT; i++) {
        // Кнопки з зовнішньою підтяжкою - LOW при натисканні
        pinMode(buttonPins[i], INPUT);

        // Створюємо кнопку: пін, активний рівень LOW, pullup вимкнено (зовнішня підтяжка)
        buttons[i] = new OneButton(buttonPins[i], true, false);

        // Налаштування таймінгів (OneButton використовує мілісекунди)
        buttons[i]->setPressMs(LONG_PRESS_MS);

        // Реєстрація обробників (тільки click і longPress)
        buttons[i]->attachClick(clickHandler, (void*)(intptr_t)i);
        buttons[i]->attachLongPressStart(longPressHandler, (void*)(intptr_t)i);

        Serial.print("[Buttons] ");
        Serial.print(buttonNames[i]);
        Serial.print(" -> GPIO");
        Serial.print(buttonPins[i]);
        Serial.print(" (ID: ");
        Serial.print(i);
        Serial.println(")");
    }

    Serial.println("[Buttons] OneButton initialized");
    Serial.print("[Buttons] Long press timeout: ");
    Serial.print(LONG_PRESS_MS);
    Serial.println("ms");
}

void buttonsLoop() {
    for (int i = 0; i < BTN_COUNT; i++) {
        if (buttons[i] != nullptr) {
            buttons[i]->tick();
        }
    }
}

bool buttonsWasClicked(ButtonId id) {
    if (id < 0 || id >= BTN_COUNT) return false;
    return buttonFlags[id].clicked;
}

bool buttonsWasLongPressed(ButtonId id) {
    if (id < 0 || id >= BTN_COUNT) return false;
    return buttonFlags[id].longPressed;
}

void buttonsClearClicked(ButtonId id) {
    if (id < 0 || id >= BTN_COUNT) return;
    buttonFlags[id].clicked = false;
}

void buttonsClearLongPressed(ButtonId id) {
    if (id < 0 || id >= BTN_COUNT) return;
    buttonFlags[id].longPressed = false;
}

const char* buttonsGetName(ButtonId id) {
    if (id < 0 || id >= BTN_COUNT) return "UNKNOWN";
    return buttonNames[id];
}
