/**
 * @file display.h
 * @brief Модуль для керування дисплеєм ST7789 170x320
 * Бібліотека: TFT_eSPI з підтримкою української мови
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "tda7318.h"
#include "audio_player.h"  // Для типу RadioStation

// Розміри дисплея (для ландшафтної орієнтації)
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  170

// Кольори (сумісні з TFT_eSPI)
#define COLOR_BLACK     TFT_BLACK
#define COLOR_WHITE     TFT_WHITE
#define COLOR_RED       TFT_RED
#define COLOR_GREEN     TFT_GREEN
#define COLOR_BLUE      TFT_BLUE
#define COLOR_YELLOW    TFT_YELLOW
#define COLOR_CYAN      TFT_CYAN
#define COLOR_MAGENTA   TFT_MAGENTA
#define COLOR_GRAY      0x8410
#define COLOR_DARKGRAY  0x4208
#define COLOR_LIGHTGRAY  TFT_LIGHTGREY
#define COLOR_ORANGE    TFT_ORANGE
#define COLOR_PURPLE    0x783F
#define COLOR_NONE      0x1000
#define COLOR_LIGHT_GREEN 0x9772

/**
 * @brief Встановити підсвітку дисплея
 * @param on true = увімкнути, false = вимкнути
 */
void displaySetBacklight(bool on);

/**
 * @brief Ініціалізація дисплея
 */
void displayInit();

/**
 * @brief Очистити екран
 * @param color Колір фону
 */
void displayClear(uint16_t color = COLOR_BLACK);

/**
 * @brief Показати екран підключення до WiFi
 */
void displayShowConnecting();

/**
 * @brief Показати інформацію про точку доступу (AP mode)
 * @param ssid Назва точки доступу
 * @param password Пароль точки доступу
 * @param ip IP адреса точки доступу
 */
void displayShowAPInfo(const char* ssid, const char* password, const char* ip);

/**
 * @brief Показати екран офлайн режиму (WiFi вимкнено)
 */
void displayShowOffline();

/**
 * @brief Оновити центральну зону в офлайн режимі (картинка входу)
 * @param activeInput Активний вхід
 */
void displayUpdateOfflineCenter(TDA7318_Input activeInput);

/**
 * @brief Вивести нижній бар з 3 входами для офлайн режиму
 * @param activeInput Активний вхід
 */
void displayDrawOfflineInputBar(TDA7318_Input activeInput);

/**
 * @brief Оновити нижній бар з 3 входами для офлайн режиму
 * @param activeInput Активний вхід
 */
void displayUpdateOfflineInputBar(TDA7318_Input activeInput);

/**
 * @brief Вивести текст
 * @param text Текст для виводу (підтримує UTF-8/кирилицю)
 * @param x Позиція X
 * @param y Позиція Y
 * @param font Розмір шрифту (1-8)
 * @param color Колір тексту
 * @param bg Колір фону (COLOR_BLACK за замовчуванням)
 */
void displayPrint(const char* text, int x, int y, uint8_t font = 2, uint16_t color = COLOR_WHITE, uint16_t bg = COLOR_BLACK);

/**
 * @brief Вивести текст з центруванням по горизонталі
 * @param text Текст для виводу
 * @param y Позиція Y
 * @param font Розмір шрифту
 * @param color Колір тексту
 * @param bg Колір фону
 */
void displayPrintCenter(const char* text, int y, uint8_t font = 2, uint16_t color = COLOR_WHITE, uint16_t bg = COLOR_BLACK);

/**
 * @brief Вивести число
 * @param value Число
 * @param x Позиція X
 * @param y Позиція Y
 * @param font Розмір шрифту
 * @param color Колір тексту
 * @param bg Колір фону
 */
void displayPrintNum(int value, int x, int y, uint8_t font = 2, uint16_t color = COLOR_WHITE, uint16_t bg = COLOR_BLACK);

/**
 * @brief Намалювати прямокутник
 * @param x Позиція X
 * @param y Позиція Y
 * @param w Ширина
 * @param h Висота
 * @param color Колір рамки
 * @param fill Колір заповнення (COLOR_BLACK за замовчуванням, якщо COLOR_NONE - без заповнення)
 */
#define COLOR_NONE 0x1000
void displayDrawRect(int x, int y, int w, int h, uint16_t color, uint16_t fill = COLOR_NONE);

/**
 * @brief Намалювати лінію
 * @param x0 Початок X
 * @param y0 Початок Y
 * @param x1 Кінець X
 * @param y1 Кінець Y
 * @param color Колір
 * @param thickness Товщина лінії
 */
void displayDrawLine(int x0, int y0, int x1, int y1, uint16_t color, uint8_t thickness = 1);

/**
 * @brief Намалювати коло
 * @param x Центр X
 * @param y Центр Y
 * @param r Радіус
 * @param color Колір
 * @param fill Заповнення (true - заповнене)
 */
void displayDrawCircle(int x, int y, int r, uint16_t color, bool fill = false);

/**
 * @brief Вивести іконку гучності
 * @param x Позиція X
 * @param y Позиція Y
 * @param volume Рівень гучності (0-100)
 * @param color Колір
 */
void displayDrawVolumeIcon(int x, int y, uint8_t volume, uint16_t color);

/**
 * @brief Вивести інформацію про радіостанцію
 * @param stationName Назва станції
 * @param trackArtist Виконавець
 * @param trackTitle Назва треку
 * @param volume Гучність (0-100)
 * @param activeInput Активний аудіовхід
 * @param wifiSSID Назва WiFi мережі
 * @param wifiIP IP адреса
 * @param tdaVolume Гучність TDA7318 (0-100)
 * @param balance Баланс L/R (-7 до +7)
 */
void displayShowRadioInfo(const char* stationName, const char* trackArtist, const char* trackTitle, uint8_t volume, TDA7318_Input activeInput = INPUT_WIFI_RADIO, const char* wifiSSID = "", const char* wifiIP = "", uint8_t tdaVolume = 0, int8_t balance = 0);

/**
 * @brief Оновити тільки інформацію про станцію і трек (вся центральна зона)
 * @param stationName Назва станції
 * @param trackArtist Виконавець
 * @param trackTitle Назва треку
 * @param isPlaying Стан відтворення (true - грає, false - стоп)
 * @param volume Гучність ESP32 (0-100)
 * @param tdaVolume Гучність TDA7318 (0-100)
 * @param activeInput Активний вхід (для відображення інформації тільки на WiFi)
 * @param balance Баланс L/R (-7 до +7)
 */
void displayUpdateStationAndTrack(const char* stationName, const char* trackArtist, const char* trackTitle, bool isPlaying = true, uint8_t volume = 0, uint8_t tdaVolume = 0, TDA7318_Input activeInput = INPUT_WIFI_RADIO, int8_t balance = 0);

/**
 * @brief Очистити зону інформації про трек
 */
void displayClearTrackInfo();

/**
 * @brief Намалювати вертикальний індикатор гучності зліва
 * @param volume Гучність (0-100)
 */
void displayDrawVolumeBar(uint8_t volume);

/**
 * @brief Намалювати вертикальний індикатор гучності TDA7318 зліва
 * @param volume Гучність TDA7318 (0-100)
 * @param isActive Чи активний цей індикатор (true = жовтий, false = сірий)
 */
void displayDrawTDAVolumeBar(uint8_t volume, bool isActive = true);

/**
 * @brief Намалювати вертикальний індикатор балансу справа
 * @param balance Баланс L/R (-7 до +7)
 * @param isActive Чи активний цей індикатор (true = блакитний, false = сірий)
 */
void displayDrawBalanceBar(int8_t balance, bool isActive = true);

/**
 * @brief Оновити тільки індикатори гучності (без перемальовки всього екрану)
 * @param volume Гучність ESP32 (0-100)
 * @param tdaVolume Гучність TDA7318 (0-100)
 * @param balance Баланс L/R (-7 до +7)
 */
void displayUpdateVolumeBars(uint8_t volume, uint8_t tdaVolume, int8_t balance);

/**
 * @brief Намалювати нижню панель з назвами індикаторів звуку
 * @param activeControl Активний індикатор (0=гучність, 1=бас, 2=тембр, 3=баланс)
 */
void displayDrawSoundControlBar(uint8_t activeControl);

/**
 * @brief Оновити нижню панель з назвами індикаторів (тільки зміни)
 * @param activeControl Активний індикатор (0=гучність, 1=бас, 2=тембр, 3=баланс)
 */
void displayUpdateSoundControlBar(uint8_t activeControl);

/**
 * @brief Намалювати індикатор тембру/басу
 * @param value Значення (-7 до +7)
 * @param barX Позиція X для індикатора
 * @param color Колір індикатора (якщо активний)
 * @param isActive Чи активний цей індикатор (true = color, false = сірий)
 */
void displayDrawToneBarVertical(int8_t value, int barX, uint16_t color, bool isActive = true);

/**
 * @brief Оновити тільки змінені сегменти індикатора басу
 * @param oldValue Старе значення (-7 до +7)
 * @param newValue Нове значення (-7 до +7)
 * @param barX Позиція X для індикатора
 */
void displayUpdateToneBarBassSegment(int8_t oldValue, int8_t newValue, int barX);

/**
 * @brief Оновити тільки змінені сегменти індикатора тембру
 * @param oldValue Старе значення (-7 до +7)
 * @param newValue Нове значення (-7 до +7)
 * @param barX Позиція X для індикатора
 */
void displayUpdateToneBarTrebleSegment(int8_t oldValue, int8_t newValue, int barX);

/**
 * @brief Оновити активні індикатори при перемиканні елемента керування
 * @param oldControl Попередній активний елемент (0-3)
 * @param newControl Новий активний елемент (0-3)
 * @param volume Гучність (0-100)
 * @param bass Бас (-7 до +7)
 * @param treble Тембр (-7 до +7)
 * @param balance Баланс (-7 до +7)
 */
void displayUpdateActiveIndicator(uint8_t oldControl, uint8_t newControl, uint8_t volume, int8_t bass, int8_t treble, int8_t balance);

/**
 * @brief Оновити рамку активного елемента керування
 * @param oldControl Попередній активний елемент (0-3)
 * @param newControl Новий активний елемент (0-3)
 */
void displayUpdateActiveControlFrame(uint8_t oldControl, uint8_t newControl);

/**
 * @brief Показати екран налаштувань звуку
 * @param volume Гучність (0-100)
 * @param balance Баланс (-7 до +7)
 * @param bass Бас (-7 до +7)
 * @param treble Тембр (-7 до +7)
 * @param activeControl Активний елемент керування (0=гучність, 1=бас, 2=тембр, 3=баланс)
 */
void displayShowSoundSettings(uint8_t volume, int8_t balance, int8_t bass, int8_t treble, uint8_t activeControl);

/**
 * @brief Вивести панель аудіо входів знизу екрану
 * @param activeInput Активний вхід (0-3)
 */
void displayDrawAudioInputBar(TDA7318_Input activeInput);

/**
 * @brief Оновити панель входів (перемалювати з новим активним входом)
 * @param activeInput Активний вхід (0-3)
 */
void displayUpdateAudioInputBar(TDA7318_Input activeInput);

/**
 * @brief Вивести інформаційну панель зверху (WiFi SSID та IP)
 * @param ssid Назва WiFi мережі
 * @param ip IP адреса
 */
void displayDrawTopBar(const char* ssid, const char* ip);

/**
 * @brief Вивести верхню панель з написом "Налаштування звуку"
 */
void displayDrawSoundSettingsHeader();

/**
 * @brief Вивести верхню панель для списку станцій з номером сторінки
 * @param page Номер сторінки (0-based)
 * @param stationsPerPage Кількість станцій на сторінці
 * @param count Загальна кількість станцій
 */
void displayDrawStationListHeader(int page, int stationsPerPage, int count);

/**
 * @brief Вивести список радіостанцій
 * @param stations Масив структур RadioStation
 * @param count Кількість станцій
 * @param selectedIndex Індекс вибраної станції
 * @param page Номер сторінки (0-based)
 * @param stationsPerPage Кількість станцій на сторінці
 */
void displayShowStationList(const RadioStation* stations, int count, int selectedIndex, int page, int stationsPerPage);

/**
 * @brief Оновити тільки вибір у списку станцій (попередній і новий індекс)
 * @param stations Масив структур RadioStation
 * @param count Кількість станцій
 * @param prevIndex Попередній індекс
 * @param newIndex Новий індекс
 * @param page Номер сторінки (0-based)
 * @param stationsPerPage Кількість станцій на сторінці
 */
void displayUpdateStationListSelection(const RadioStation* stations, int count, int prevIndex, int newIndex, int page, int stationsPerPage);

/**
 * @brief Оновити екран
 */
void displayUpdate();

/**
 * @brief Вивести BMP картинку з SPIFFS по центру
 * @param filename Шлях до файлу (наприклад "/out_computer.bmp")
 */
void displayDrawBmpCenter(const char* filename);

/**
 * @brief Вивести BMP картинку з SPIFFS по центру
 * @param filename Шлях до файлу (наприклад "/out_computer.bmp")
 */
void displayDrawBmpCenter(const char* filename);

/**
 * @brief Отримати об'єкт дисплея для прямого доступу
 * @return Посилання на TFT_eSPI
 */
TFT_eSPI& displayGetTFT();

#endif // DISPLAY_H
