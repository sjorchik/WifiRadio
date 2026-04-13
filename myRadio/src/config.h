/**
 * @file config.h
 * @brief Централізована конфігурація проекту ESP32 Radio
 * 
 * Тут зберігаються всі налаштування: піни, затримки, коди кнопок,
 * таймаути, дефолтні значення та інші параметри.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// GPIO ПІНИ
// ============================================================================

// --- Кнопки ---
#define BTN_PWR_PIN       33   // Кнопка живлення
#define BTN_LEFT_PIN      35   // Кнопка вліво
#define BTN_RIGHT_PIN     39   // Кнопка вправо
#define BTN_UP_PIN        32   // Кнопка вгору
#define BTN_DOWN_PIN      36   // Кнопка вниз
#define BTN_OK_PIN        34   // Кнопка OK

// --- Енкодер ---
#define ENC_A_PIN         13   // Енкодер сигнал A
#define ENC_B_PIN         14   // Енкодер сигнал B
#define ENC_BTN_PIN       19   // Енкодер кнопка
#define ENC_PULSES_PER_STEP 4  // Кількість імпульсів на крок енкодера

// --- ІЧ приймач ---
#define IR_RECEIVER_PIN   15   // ІЧ приймач

// --- I2S DAC ---
#define I2S_DOUT          27   // I2S data out
#define I2S_BCLK          26   // I2S bit clock
#define I2S_LRC           25   // I2S left/right clock

// --- Дисплей (SPI) ---
#define TFT_DC            16   // Data/Command
#define TFT_RST           17   // Reset
#define TFT_MOSI          23   // SPI data
#define TFT_SCLK          18   // SPI clock
#define TFT_CS            5    // Chip select
#define TFT_BL            4    // Backlight

// --- I2C (TDA7318) ---
#define TDA7318_I2C_ADDR  0x44 // I2C адреса аудіопроцесора

// ============================================================================
// ЗАТРИМКИ ТА ТАЙМАУТИ
// ============================================================================

// --- ІЧ пульт ---
#define IR_COOLDOWN_MS          500   // Затримка після сигналу POWER (захист від подвійних спрацювань)
#define IR_REPEAT_MS            200   // Затримка між повторними натисканнями ІЧ
#define IR_KEY_BUFFER_MS        100   // Час буферизації ІЧ команд

// --- Кнопки ---
#define LONG_PRESS_MS           1000  // Час довгого натискання (1 сек)
#define BTN_DEBOUNCE_MS         50    // Антидребез кнопок

// --- Енкодер ---
#define ENC_BTN_DEBOUNCE_MS     50    // Антидребез кнопки енкодера
#define ENC_LONG_PRESS_MS       1000  // Довге натискання кнопки енкодера

// --- Таймаути інтерфейсу ---
#define SOUND_SETTINGS_TIMEOUT_MS   10000  // Автозакриття налаштувань звуку (10 сек)
#define LIST_AUTO_CLOSE_MS          10000  // Автозакриття списку станцій (10 сек)
#define MESSAGE_TIMEOUT_MS          5000   // Час показу повідомлень NEXT/PREV/PLAY/STOP (5 сек)
#define OK_COOLDOWN_MS              300    // Затримка після LONG_PRESS кнопки OK

// --- WiFi ---
#define WIFI_MAX_ATTEMPTS       20    // Максимальна кількість спроб підключення WiFi
#define WIFI_RETRY_DELAY_MS     500   // Затримка між спробами підключення (500 мс)

// --- Затримки I2C ---
#define I2C_INIT_DELAY_MS       10    // Затримка після ініціалізації I2C
#define I2C_INPUT_CHANGE_DELAY_MS 50  // Затримка після зміни входу TDA7318

// --- Затримки аудіо ---
#define AUDIO_STOP_PLAY_DELAY_MS 100  // Затримка між stop та play

// ============================================================================
// КОДИ КНОПОК ІЧ ПУЛЬТА (RC-5 Protocol)
// ============================================================================

#define IR_BTN_POWER        0x0C  // Кнопка живлення
#define IR_BTN_VOL_UP       0x10  // Гучність +
#define IR_BTN_VOL_DOWN     0x11  // Гучність -
#define IR_BTN_MUTE         0x0D  // Mute
#define IR_BTN_0            0x00  // Цифра 0
#define IR_BTN_1            0x01  // Цифра 1
#define IR_BTN_2            0x02  // Цифра 2
#define IR_BTN_3            0x03  // Цифра 3
#define IR_BTN_4            0x04  // Цифра 4
#define IR_BTN_5            0x05  // Цифра 5
#define IR_BTN_6            0x06  // Цифра 6
#define IR_BTN_7            0x07  // Цифра 7
#define IR_BTN_8            0x08  // Цифра 8
#define IR_BTN_9            0x09  // Цифра 9
#define IR_BTN_CH_UP        0x20  // Канал + (попередня станція)
#define IR_BTN_CH_DOWN      0x21  // Канал - (наступна станція)
#define IR_BTN_PLAY_PAUSE   0x22  // Play/Pause
#define IR_BTN_STOP         0x23  // Stop
#define IR_BTN_INPUT        0x24  // Циклічна зміна входу
#define IR_BTN_WIFI         0x2B  // Вхід WiFi
#define IR_BTN_COMPUTER     0x2C  // Вхід Computer
#define IR_BTN_TV_BOX       0x2D  // Вхід TV Box
#define IR_BTN_AUX          0x2E  // Вхід AUX

// ============================================================================
// ДЕФОЛТНІ ЗНАЧЕННЯ
// ============================================================================

// --- TDA7318 аудіопроцесор ---
#define DEFAULT_VOLUME      50    // Дефолтна гучність TDA7318 (0-100)
#define DEFAULT_BASS        0     // Дефолтний бас (-7...+7)
#define DEFAULT_TREBLE      0     // Дефолтний тембр (-7...+7)
#define DEFAULT_BALANCE     0     // Дефолтний баланс (-31...+31)

// --- Gain для входів TDA7318 ---
#define DEFAULT_INPUT_GAIN_WIFI    0   // Gain для WiFi входу
#define DEFAULT_INPUT_GAIN_COMPUTER 0  // Gain для Computer входу
#define DEFAULT_INPUT_GAIN_TVBOX  2   // Gain для TV Box входу
#define DEFAULT_INPUT_GAIN_AUX    0   // Gain для AUX входу

// --- ESP32 аудіоплеєр ---
#define DEFAULT_ESP_VOLUME    20    // Дефолтна гучність ESP32 плеєра (0-100)

// ============================================================================
// WiFi / AP КОНФІГУРАЦІЯ
// ============================================================================

// --- Access Point ---
#define AP_SSID             "ESP32_Radio"  // Назва точки доступу
#define AP_PASSWORD         "123456789"    // Пароль точки доступу
#define AP_CHANNEL          1              // Канал точки доступу
#define AP_MAX_CONNECTIONS  4              // Максимальна кількість підключень

// --- Збережені WiFi мережі ---
#define MAX_WIFI_NETWORKS   3              // Максимальна кількість збережених мереж

// ============================================================================
// ОБМЕЖЕННЯ ТА ЛІМІТИ
// ============================================================================

// --- Станції ---
#define MAX_STATIONS        20             // Максимальна кількість радіостанцій
#define STATIONS_PER_PAGE   5              // Кількість станцій на сторінці списку

// --- Діапазони налаштувань звуку ---
#define VOLUME_MIN          0              // Мінімальна гучність
#define VOLUME_MAX          100            // Максимальна гучність
#define BASS_MIN            -7             // Мінімальний бас
#define BASS_MAX            7              // Максимальний бас
#define TREBLE_MIN          -7             // Мінімальний тембр
#define TREBLE_MAX          7              // Максимальний тембр
#define BALANCE_MIN         -31            // Мінімальний баланс
#define BALANCE_MAX         31             // Максимальний баланс
#define INPUT_GAIN_MIN      0              // Мінімальний gain входу
#define INPUT_GAIN_MAX      3              // Максимальний gain входу

// ============================================================================
// ДИСПЛЕЙ
// ============================================================================

// --- Розміри дисплею ---
#define DISPLAY_WIDTH       320            // Ширина дисплею (пікселі)
#define DISPLAY_HEIGHT      170            // Висота дисплею (пікселі)

// --- Верхня панель ---
#define TOP_BAR_HEIGHT      25             // Висота верхньої панелі

// --- Індикатори гучності/балансу ---
#define VOLUME_BAR_BASE_WIDTH     6        // Базова ширина індикатора гучності
#define BALANCE_BAR_BASE_WIDTH    8        // Базова ширина індикатора балансу
#define VOLUME_BAR_SEGMENT_HEIGHT 6        // Висота сегмента індикатора гучності
#define BALANCE_BAR_SEGMENT_HEIGHT 6       // Висота сегмента індикатора балансу
#define BAR_SEGMENT_GAP           1        // Проміжок між сегментами
#define BAR_TOTAL_SEGMENTS        15       // Кількість сегментів
#define BAR_WIDTH_INCREMENT       3        // Збільшення ширини для гучності
#define BALANCE_WIDTH_INCREMENT   5        // Збільшення ширини для балансу

// --- Позиції індикаторів ---
#define VOLUME_BAR_X        6              // Позиція X індикатора гучності
#define BASS_BAR_X          81             // Позиція X індикатора басу
#define TREBLE_BAR_X        177            // Позиція X індикатора тембру
#define BALANCE_BAR_X_OFFSET 5             // Відступ індикатора балансу справа

// --- Налаштування звуку (екран) ---
#define SOUND_VOLUME_BAR_X      6          // X індикатора гучності
#define SOUND_VOLUME_BAR_WIDTH  43         // Максимальна ширина індикатора гучності
#define SOUND_BASS_BAR_X        81         // X індикатора басу
#define SOUND_TREBLE_BAR_X      177        // X індикатора тембру
#define SOUND_BALANCE_BAR_WIDTH 43         // Максимальна ширина індикатора балансу
#define SOUND_BAR_GAP           32         // Проміжок між індикаторами
#define SOUND_TONE_BAR_WIDTH    64         // Ширина індикатора тембру/басу

// --- Зарезервоване місце для бічних індикаторів ---
#define LEFT_BAR_RESERVED_WIDTH   55       // Місце зліва (гучність)
#define RIGHT_BAR_RESERVED_WIDTH  50       // Місце справа (баланс)
#define HORIZONTAL_PADDING        10       // Горизонтальний відступ

// --- Кольори дисплею ---
#define COLOR_BLACK       TFT_BLACK
#define COLOR_WHITE       TFT_WHITE
#define COLOR_RED         TFT_RED
#define COLOR_GREEN       TFT_GREEN
#define COLOR_BLUE        TFT_BLUE
#define COLOR_YELLOW      TFT_YELLOW
#define COLOR_CYAN        TFT_CYAN
#define COLOR_MAGENTA     TFT_MAGENTA
#define COLOR_GRAY        0x8410
#define COLOR_DARKGRAY    0x4208
#define COLOR_LIGHTGRAY   TFT_LIGHTGREY
#define COLOR_ORANGE      TFT_ORANGE
#define COLOR_PURPLE      0x783F
#define COLOR_NONE        0x1000
#define COLOR_LIGHT_GREEN 0x9772

// ============================================================================
// КЛЮЧІ NVS (Preferences)
// ============================================================================

// --- TDA7318 ---
#define TDA7318_PREFS_KEY     "tda7318"    // Основний ключ
#define TDA7318_INPUT_KEY     "input"      // Збережений вхід
#define TDA7318_VOL_KEY       "vol_%d"     // Гучність для входу
#define TDA7318_BASS_KEY      "bass_%d"    // Бас для входу
#define TDA7318_TREBLE_KEY    "treble_%d"  // Тембр для входу
#define TDA7318_BAL_KEY       "bal_%d"     // Баланс для входу

// --- Аудіоплеєр ---
#define AUDIO_STATIONS_NS     "stations"   // Namespace для станцій
#define AUDIO_PLAYER_NS       "player"     // Namespace для плеєра
#define AUDIO_LAST_STATION_KEY "last_station"  // Ключ останньої станції
#define AUDIO_STATION_NAME_KEY "name_%d"   // Назва станції
#define AUDIO_STATION_URL_KEY  "url_%d"    // URL станції
#define AUDIO_STATION_VOL_KEY  "vol_%d"    // Гучність станції

// --- WiFi мережі ---
#define WIFI_SSID_KEY_1       "ssid1"      // SSID мережі 1
#define WIFI_PASS_KEY_1       "pass1"      // Пароль мережі 1
#define WIFI_SSID_KEY_2       "ssid2"      // SSID мережі 2
#define WIFI_PASS_KEY_2       "pass2"      // Пароль мережі 2
#define WIFI_SSID_KEY_3       "ssid3"      // SSID мережі 3
#define WIFI_PASS_KEY_3       "pass3"      // Пароль мережі 3

// ============================================================================
// РОЗМІРИ JSON ДОКУМЕНТІВ
// ============================================================================

#define JSON_STATUS_SIZE      1024         // Розмір JSON для статусу
#define JSON_WIFI_LOAD_SIZE   512          // Розмір JSON для завантаження WiFi
#define JSON_WIFI_SAVE_SIZE   1024         // Розмір JSON для збереження WiFi
#define JSON_SOUND_SIZE       128          // Розмір JSON для налаштувань звуку

// ============================================================================
// ШВИДКІСТЬ SPI ДИСПЛЕЮ
// ============================================================================

#define TFT_SPI_FREQUENCY     80000000     // 80 MHz

// ============================================================================
// РОЗМІР СТЕКУ ARDUINO LOOP
// ============================================================================

#define ARDUINO_LOOP_STACK_SIZE  8192      // 8 KB

#endif // CONFIG_H
