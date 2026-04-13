/**
 * @file tda7318.h
 * @brief Модуль для керування аудіопроцесором TDA7318 через I2C
 * На основі бібліотеки з проекту SoundCube
 */

#ifndef TDA7318_H
#define TDA7318_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// Входи аудіо
typedef enum {
    INPUT_WIFI_RADIO = 0,   // WiFi радіо
    INPUT_COMPUTER = 1,     // Computer
    INPUT_TV_BOX = 2,       // TV Box
    INPUT_AUX = 3           // AUX
} TDA7318_Input;

// Стан аудіопроцесора
typedef struct {
    uint8_t volume;         // Гучність 0-100
    int8_t bass;            // Бас -7 до +7
    int8_t treble;          // Високі -7 до +7
    int8_t balance;         // Баланс L/R -31 до +31 (0 = центр)
    TDA7318_Input input;    // Поточний вхід
    bool muted;             // Стан Mute
} TDA7318_State;

/**
 * @brief Ініціалізація TDA7318
 * @return true якщо успішно
 */
bool tda7318Init();

/**
 * @brief Встановити гучність
 * @param volume Гучність від 0 до 100
 */
void tda7318SetVolume(uint8_t volume);

/**
 * @brief Отримати поточну гучність
 * @return Гучність від 0 до 100
 */
uint8_t tda7318GetVolume();

/**
 * @brief Встановити рівень басів
 * @param value Значення від -7 до +7
 */
void tda7318SetBass(int8_t value);

/**
 * @brief Отримати поточний рівень басів
 * @return Значення від -7 до +7
 */
int8_t tda7318GetBass();

/**
 * @brief Встановити рівень високих частот
 * @param value Значення від -7 до +7
 */
void tda7318SetTreble(int8_t value);

/**
 * @brief Отримати поточний рівень високих частот
 * @return Значення від -7 до +7
 */
int8_t tda7318GetTreble();

/**
 * @brief Встановити баланс лівий/правий
 * @param value Значення від -7 до +7 (-7 = макс. вліво, 0 = центр, +7 = макс. вправо)
 */
void tda7318SetBalance(int8_t value);

/**
 * @brief Отримати поточний баланс
 * @return Значення від -7 до +7
 */
int8_t tda7318GetBalance();

/**
 * @brief Перемкнути вхід
 * @param input Вхід (INPUT_WIFI_RADIO, INPUT_COMPUTER, INPUT_TV_BOX, INPUT_AUX)
 * @param gain Gain входу (0-3). Якщо не вказано, використовується дефолтний для цього входу:
 *             INPUT_WIFI_RADIO=2, INPUT_COMPUTER=2, INPUT_TV_BOX=2, INPUT_AUX=3
 */
void tda7318SetInput(TDA7318_Input input, int8_t gain = -1);

/**
 * @brief Отримати поточний вхід
 * @return Поточний вхід
 */
TDA7318_Input tda7318GetInput();

/**
 * @brief Увімкнути Mute
 */
void tda7318Mute();

/**
 * @brief Вимкнути Mute
 */
void tda7318UnMute();

/**
 * @brief Перемкнути стан Mute
 * @param muted true - увімкнути Mute, false - вимкнути
 */
void tda7318SetMute(bool muted);

/**
 * @brief Отримати стан Mute
 * @return true якщо Mute увімкнено
 */
bool tda7318IsMuted();

/**
 * @brief Отримати поточний стан аудіопроцесора
 * @return Структура з поточними налаштуваннями
 */
TDA7318_State tda7318GetState();

/**
 * @brief Встановити стан аудіопроцесора
 * @param state Структура з налаштуваннями
 */
void tda7318SetState(const TDA7318_State& state);

#endif // TDA7318_H
