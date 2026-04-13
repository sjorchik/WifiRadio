/**
 * @file app_wifi.h
 * @brief Модуль для керування WiFi підключенням ESP32
 *
 * Цей файл містить функції для:
 * - Підключення до WiFi мережі
 * - Налаштування точки доступу (AP mode)
 * - Збереження та читання WiFi налаштувань з NVS пам'яті
 * - Обробники HTTP запитів для WiFi операцій
 */

#ifndef APP_WIFI_H
#define APP_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include "config.h"

// Структура для зберігання WiFi мережі
typedef struct {
  String ssid;
  String password;
} WifiNetwork;

/**
 * @brief Ініціалізація WiFi модуля
 *
 * Спочатку намагається підключитися до збереженої WiFi мережі.
 * Якщо не вдається - вмикає режим точки доступу.
 *
 * @return true якщо підключення успішне, false якщо увімкнено AP mode
 */
bool appWifiInit();

/**
 * @brief Підключення до WiFi мережі
 *
 * Намагається підключитися до вказаної WiFi мережі.
 * Якщо SSID та пароль не вказані - використовує збережені налаштування.
 *
 * @param ssid Назва WiFi мережі (опціонально)
 * @param password Пароль WiFi (опціонально)
 * @return true якщо підключення успішне, false інакше
 */
bool appWifiConnect(const char* ssid = nullptr, const char* password = nullptr);

/**
 * @brief Налаштування точки доступу (AP mode)
 *
 * Створює власну точку доступу з вказаними обліковими даними.
 * Також запускає mDNS службу для доступу за іменем.
 */
void appWifiSetupAP();

/**
 * @brief Збереження WiFi налаштувань в NVS пам'ять
 *
 * Зберігає до 3-х мереж WiFi в постійну пам'ять.
 *
 * @param networks Масив мереж WiFi
 * @param count Кількість мереж для збереження
 * @return true якщо збереження успішне, false інакше
 */
bool appWifiSaveCredentials(const WifiNetwork* networks, size_t count);

/**
 * @brief Читання збережених WiFi налаштувань з NVS пам'яті
 *
 * Отримує раніше збережені мережі WiFi (до 3-х).
 *
 * @param networks Масив для збереження мереж
 * @param count Посилання на змінну кількості мереж
 * @return true якщо налаштування знайдено, false інакше
 */
bool appWifiLoadCredentials(WifiNetwork* networks, size_t& count);

/**
 * @brief Видалення збережених WiFi налаштувань
 *
 * Очищає NVS пам'ять від збережених облікових даних.
 */
void appWifiClearCredentials();

/**
 * @brief Перевірка статусу WiFi підключення
 *
 * @return true якщо пристрій підключений до WiFi, false інакше
 */
bool appWifiIsConnected();

/**
 * @brief Отримання IP-адреси пристрою
 *
 * @return IP-адреса у вигляді рядка, або порожній рядок якщо не підключено
 */
String appWifiGetIPAddress();

/**
 * @brief Перезавантаження ESP32 модуля
 *
 * Виконує м'яке перезавантаження пристрою.
 */
void appWifiRestart();

/**
 * @brief Обробник HTTP запиту статусу (/status)
 *
 * Відправляє JSON з інформацією про:
 * - Статус підключення до WiFi
 * - Час роботи пристрою
 * - Вільну пам'ять heap
 */
void appWifiHandleStatus();

/**
 * @brief Обробник HTTP запиту збереження налаштувань (/settings/wifi)
 *
 * Приймає POST запит з JSON даними (ssid, password) та зберігає в NVS.
 * Після збереження перезавантажує пристрій.
 */
void appWifiHandleSettings();

/**
 * @brief Обробник HTTP запиту завантаження налаштувань WiFi (/settings/wifi/load)
 *
 * Відправляє JSON збережених мереж WiFi.
 */
void appWifiHandleLoadSettings();

#endif // APP_WIFI_H
