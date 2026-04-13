/**
 * @file ir_remote.h
 * @brief Модуль для керування ІЧ пультом RC-5
 */

#ifndef IR_REMOTE_H
#define IR_REMOTE_H

#include <Arduino.h>
#include "config.h"

// Пін для ІЧ приймача

// Коди кнопок ІЧ пульта (RC-5)

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
