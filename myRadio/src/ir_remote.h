/**
 * @file ir_remote.h
 * @brief Модуль для керування ІЧ пультом RC-5
 */

#ifndef IR_REMOTE_H
#define IR_REMOTE_H

#include <Arduino.h>

// Пін для ІЧ приймача
#define IR_RECEIVER_PIN 15

// Коди кнопок ІЧ пульта (RC-5)
// Система: 0 (стандартний RC-5)
#define IR_BTN_POWER       0x0C  // Кнопка живлення
#define IR_BTN_VOL_UP      0x10  // Гучність +
#define IR_BTN_VOL_DOWN    0x11  // Гучність -
#define IR_BTN_MUTE        0x0D  // Mute
#define IR_BTN_0           0x00
#define IR_BTN_1           0x01
#define IR_BTN_2           0x02
#define IR_BTN_3           0x03
#define IR_BTN_4           0x04
#define IR_BTN_5           0x05
#define IR_BTN_6           0x06
#define IR_BTN_7           0x07
#define IR_BTN_8           0x08
#define IR_BTN_9           0x09
#define IR_BTN_CH_UP       0x20  // Канал + (попередня станція)
#define IR_BTN_CH_DOWN     0x21  // Канал - (наступна станція)
#define IR_BTN_PLAY_PAUSE  0x22  // Play/Pause
#define IR_BTN_STOP        0x23  // Stop
#define IR_BTN_INPUT       0x24  // Перемикання входу (циклічне)
#define IR_BTN_WIFI        0x2B  // Вхід WiFi
#define IR_BTN_COMPUTER    0x2C  // Вхід Computer
#define IR_BTN_TV_BOX      0x2D  // Вхід TV Box
#define IR_BTN_AUX         0x2E  // Вхід AUX

/**
 * @brief Ініціалізація ІЧ пульта
 */
void irRemoteInit();

/**
 * @brief Обробка ІЧ сигналу (викликати в loop())
 */
void irRemoteLoop();

/**
 * @brief Отримати останню натиснуту кнопку
 * @return Код кнопки або 0xFF якщо нічого не натиснуто
 */
uint8_t irRemoteGetLastKey();

#endif // IR_REMOTE_H
