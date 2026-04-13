


/**
 * @file main.cpp
 * @brief Головний файл прошивки ESP32 Radio
 */

// Підключення необхідних бібліотек
#include <Arduino.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <algorithm>
#include "app_wifi.h"
#include "audio_player.h"
#include "tda7318.h"
#include "display.h"
#include "buttons.h"
#include "encoder.h"
#include "web_handlers.h"
#include "ir_remote.h"

// ============================================================================
// Глобальні змінні
// ============================================================================

// Об'єкт веб-сервера на 80 порту (стандартний HTTP)
WebServer server(80);

// Для відстеження стану кнопок
static TDA7318_Input lastInput = INPUT_WIFI_RADIO;  // Для відстеження зміни входу
static char lastKnownStation[64] = "";  // Для відображення коли радіо зупинено

// Режим офлайн (WiFi вимкнено)
static bool offlineMode = false;  // Чи увімкнено офлайн режим
static bool wifiConnected = false;  // Чи підключено WiFi
static bool lastRadioPlayingBeforeInputChange = false;  // Чи грало радіо до зміни входу

// Режим сну (пристрой вимкнено кнопкою PWR)
static bool deviceSleep = false;  // Чи пристрій в режимі сну
static bool wifiWasOnBeforeSleep = false;  // Чи був WiFi увімкнений до сну
static bool apModeBeforeSleep = false;  // Чи був AP режим до сну
static bool soundWasMutedBeforeSleep = false;  // Чи був звук вимкнений до сну
static uint8_t volumeBeforeSleep = 0;  // Гучність до сну
static TDA7318_Input inputBeforeSleep = INPUT_WIFI_RADIO;  // Вхід до сну
static unsigned long irWakeTime = 0;  // Час пробудження від ІЧ (для cooldown)
static unsigned long irLastKeyTime = 0;  // Час останнього сигналу пульта
static const unsigned long IR_COOLDOWN_MS = 500;  // Затримка після першого сигналу 0x0C

// Функція переходу в режим сну (вимкнення дисплея, WiFi, звуку)
// @param keepWifi true = не вимикати WiFi (для першого старту)
void deviceEnterSleep(bool keepWifi) {
  if (deviceSleep) return;  // Вже в режимі сну

  Serial.println("[Sleep] Entering sleep mode...");
  deviceSleep = true;

  // Зберігаємо стан гучності і входу
  volumeBeforeSleep = tda7318GetVolume();
  inputBeforeSleep = tda7318GetInput();

  // Вимикаємо звук на TDA7318 (встановлюємо mute або гучність 0)
  tda7318SetMute(true);
  soundWasMutedBeforeSleep = true;
  Serial.println("[Sleep] Sound muted");

  // Зупиняємо відтворення радіо якщо грало
  if (audioPlayerGetState() == PLAYER_PLAYING) {
    audioPlayerStop();
    audioPlayerSaveCurrentStation();
    Serial.println("[Sleep] Radio stopped");
  }

  // Вимикаємо WiFi тільки якщо не просили залишити
  if (!keepWifi && appWifiIsConnected()) {
    wifiWasOnBeforeSleep = true;
    apModeBeforeSleep = false;
    WiFi.mode(WIFI_OFF);
    MDNS.end();
    Serial.println("[Sleep] WiFi disabled");
  } else if (appWifiIsConnected()) {
    // WiFi підключений
    wifiWasOnBeforeSleep = true;
    apModeBeforeSleep = false;
    Serial.println("[Sleep] WiFi kept active (connected)");
  } else {
    // Не підключений - перевіряємо чи це AP режим
    wifiWasOnBeforeSleep = false;
    apModeBeforeSleep = !offlineMode;  // AP режим = не offlineMode і не connected
    if (apModeBeforeSleep) {
      Serial.println("[Sleep] AP mode saved");
    } else {
      Serial.println("[Sleep] WiFi kept active (first boot)");
    }
  }

  // Вимикаємо дисплей
  displaySetBacklight(false);
  Serial.println("[Sleep] Display backlight off");
}

// Функція виходу з режиму сну (вмикання всього назад)
void deviceWakeUp() {
  if (!deviceSleep) return;  // Не в режимі сну

  Serial.println("[Sleep] Waking up...");
  deviceSleep = false;

  // Вмикаємо дисплей
  displaySetBacklight(true);
  Serial.println("[Sleep] Display backlight on");

  // Вмикаємо WiFi якщо був вимкнений
  if (wifiWasOnBeforeSleep) {
    Serial.println("[Sleep] Restoring WiFi connection...");
    displayShowConnecting();
    wifiConnected = appWifiInit();

    if (!wifiConnected) {
      // Не вдалося підключитися - вмикаємо AP режим
      Serial.println("[Sleep] WiFi connection failed, entering AP mode");
      displayShowAPInfo(AP_SSID, AP_PASSWORD, appWifiGetIPAddress().c_str());
    } else {
      // Підключено - відновлюємо радіо
      Serial.println("[Sleep] WiFi connected, restoring radio");

      // Завантажуємо останню станцію
      int lastStationIndex = audioPlayerLoadCurrentStation();
      if (lastStationIndex >= 0) {
        RadioStation station;
        if (audioPlayerGetStation(lastStationIndex, &station)) {
          audioPlayerSetCurrentStationIndex(lastStationIndex);
          audioPlayerConnect(station.url, station.name, station.volume);
          strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);

          String wifiSSID = WiFi.SSID();
          String wifiIP = appWifiGetIPAddress();
          displayShowRadioInfo(station.name, "-", "-", station.volume, inputBeforeSleep, wifiSSID.c_str(), wifiIP.c_str(), tda7318GetVolume(), tda7318GetBalance());
        }
      } else {
        String wifiSSID = WiFi.SSID();
        String wifiIP = appWifiGetIPAddress();
        displayShowRadioInfo("-", "-", "-", 0, inputBeforeSleep, wifiSSID.c_str(), wifiIP.c_str(), tda7318GetVolume(), tda7318GetBalance());
      }
    }
  } else if (apModeBeforeSleep) {
    // Відновлюємо AP режим
    Serial.println("[Sleep] Restoring AP mode...");
    wifiConnected = false;
    displayShowAPInfo(AP_SSID, AP_PASSWORD, appWifiGetIPAddress().c_str());
  } else {
    // WiFi не був увімкнений - показуємо офлайн екран
    wifiConnected = false;
    displayShowOffline();
  }

  // Вмикаємо звук на TDA7318
  if (soundWasMutedBeforeSleep) {
    tda7318SetMute(false);
    // Відновлюємо гучність
    tda7318SetVolume(volumeBeforeSleep);
    soundWasMutedBeforeSleep = false;
    Serial.printf("[Sleep] Sound unmuted, volume restored: %d\n", volumeBeforeSleep);
  }
}

// Функція переходу в офлайн режим
void enterOfflineMode() {
  Serial.println("[INIT] Entering offline mode...");
  offlineMode = true;

  // Вимикаємо WiFi
  WiFi.mode(WIFI_OFF);
  Serial.println("[WiFi] WiFi disabled");

  // Зупиняємо mDNS
  MDNS.end();

  // Зупиняємо відтворення радіо
  audioPlayerStop();
  audioPlayerSaveCurrentStation();
  Serial.println("[Offline] Radio stopped and station index saved");

  // Встановлюємо вхід не WiFi (наприклад Computer)
  tda7318SetInput(INPUT_COMPUTER);

  // Показуємо екран для офлайн режиму
  displayShowOffline();
}

// Стан меню вибору станцій
static bool stationListOpen = false;  // Чи відкрито список станцій
static int stationListIndex = 0;  // Поточний індекс у списку (відображений)
static int targetStationIndex = 0;  // Цільовий індекс (куди хочемо перейти)
static int stationListCount = 0;  // Кількість станцій у списку
static int stationListPage = 0;  // Поточна сторінка списку
static int stationIndexBeforeOpen = 0;  // Індекс станції яка грала до відкриття списку
static const int STATIONS_PER_PAGE = 5;  // Кількість станцій на сторінці
static RadioStation stationsBuffer[MAX_STATIONS];  // Буфер для списку станцій

// Стан відображення налаштувань звуку
bool soundSettingsOpen = false;  // Чи відкрито меню налаштувань звуку (глобальна змінна)
static const int BASS_BAR_X = 81;  // Позиція X індикатора басу
static const int TREBLE_BAR_X = 177;  // Позиція X індикатора тембру
static uint8_t activeToneControl = 0;  // 0 = гучність, 1 = бас, 2 = тембр, 3 = баланс
static uint8_t lastActiveControl = 0;  // Попередній активний елемент для перемальовки рамки
static int8_t lastBassValue = 0;  // Попереднє значення басу
static int8_t lastTrebleValue = 0;  // Попереднє значення тембру
static unsigned long lastSoundSettingsTime = 0;  // Час останньої зміни в налаштуваннях звуку
static const unsigned long SOUND_SETTINGS_TIMEOUT_MS = 10000;  // 10 секунд
static bool soundSettingsTimeoutLogged = false;  // Для відладки

// Для захисту від подвійних спрацювань кнопки OK
static unsigned long okLongPressTime = 0;  // Час останнього LONG_PRESS
static const unsigned long OK_COOLDOWN_MS = 300;  // Затримка після LONG_PRESS

// Таймер для очищення написів NEXT/PREV/PLAY/STOP
static unsigned long messageDisplayTime = 0;  // Час виводу повідомлення
static const unsigned long MESSAGE_TIMEOUT_MS = 5000;  // Час через який прибрати напис (5 сек)
static bool messageDisplayed = false;  // Чи виведено повідомлення

// Таймер для автоматичного закриття списку станцій
static unsigned long lastButtonTimeInList = 0;  // Час останнього натискання в списку
static const unsigned long LIST_AUTO_CLOSE_MS = 10000;  // 10 секунд

// Функція для обробки зміни входу TDA7318
void handleInputChange(TDA7318_Input newInput) {
  TDA7318_Input oldInput = tda7318GetInput();

  // СПОЧАТКУ відправляємо команду по I2C для перемикання входу
  tda7318SetInput(newInput);
  delay(50);  // Невеличка очікування завершення I2C передачі

  // Потім виконуємо решту операцій

  // СПОЧАТКУ оновлюємо дисплей
  if (!stationListOpen && !soundSettingsOpen) {
    if (offlineMode) {
      // В офлайн режимі оновлюємо тільки панель входів і центральну зону
      displayUpdateOfflineInputBar(newInput);
      displayUpdateOfflineCenter(newInput);
    } else {
      uint8_t tdaVolume = tda7318GetVolume();
      int8_t balance = tda7318GetBalance();
      String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
      String wifiIP = appWifiGetIPAddress();
      const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
      displayUpdateStationAndTrack(displayStation, "", "", true, audioPlayerGetVolume(), tdaVolume, newInput, balance);
    }
  }

  // Потім виконуємо операції з радіо
  // Якщо перейшли на WiFi вхід
  if (newInput == INPUT_WIFI_RADIO) {
    // Якщо радіо грало до зміни входу - відновлюємо відтворення
    if (lastRadioPlayingBeforeInputChange) {
      int lastStationIndex = audioPlayerGetCurrentStationIndex();
      if (lastStationIndex >= 0) {
        RadioStation station;
        if (audioPlayerGetStation(lastStationIndex, &station)) {
          Serial.printf("[Input] Restoring last played station: %s\n", station.name);
          audioPlayerConnect(station.url, station.name, station.volume);
          audioPlayerSetCurrentStationIndex(lastStationIndex);
          strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);
        }
      }
    }
    lastRadioPlayingBeforeInputChange = false;
  }
  // Якщо пішли з WiFi входу на інший
  else if (oldInput == INPUT_WIFI_RADIO) {
    // Зберігаємо стан відтворення
    if (audioPlayerGetState() == PLAYER_PLAYING) {
      lastRadioPlayingBeforeInputChange = true;
      // Зупиняємо відтворення
      audioPlayerStop();
      audioPlayerSaveCurrentStation();
      Serial.println("[Input] Radio stopped - switched from WiFi input");
    } else {
      lastRadioPlayingBeforeInputChange = false;
    }
  }

  Serial.printf("[Input] Changed to input: %d\n", newInput);
}

// Оголошення функції для обробки ІЧ пульта
void handleIRRemote(uint8_t keyCode);

// Оголошення функції для закриття списку з main.cpp
void closeStationList();

// Функція закриття списку станцій з оновленням дисплея
void closeStationList() {
  if (stationListOpen) {
    stationListOpen = false;
    
    // Запускаємо відтворення станції яка грала до відкриття списку
    if (stationIndexBeforeOpen >= 0 && stationIndexBeforeOpen < stationListCount) {
      audioPlayerConnect(stationsBuffer[stationIndexBeforeOpen].url, stationsBuffer[stationIndexBeforeOpen].name, stationsBuffer[stationIndexBeforeOpen].volume);
      audioPlayerSetCurrentStationIndex(stationIndexBeforeOpen);
      audioPlayerSaveCurrentStation();  // Зберігаємо індекс
      Serial.printf("[Web] Close list - Resume playing: %s\n", stationsBuffer[stationIndexBeforeOpen].name);
    }
    
    // Оновлюємо весь дисплей
    uint8_t espVolume = audioPlayerGetVolume();
    uint8_t tdaVolume = tda7318GetVolume();
    const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
    String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
    String wifiIP = appWifiGetIPAddress();
    displayShowRadioInfo(displayStation, audioPlayerGetArtist(), audioPlayerGetTitle(), espVolume, tda7318GetInput(), wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, tda7318GetBalance());
  }
}

// ============================================================================
// Функція ініціалізації
// ============================================================================

/**
 * @brief Функція setup() - виконується один раз при старті
 */
void setup() {
  // Ініціалізація послідовного порту
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("================================="));
  Serial.println(F("ESP32 Radio - Starting"));
  Serial.println(F("================================="));

  // Монтування SPIFFS
  Serial.println(F("[INIT] Mounting SPIFFS..."));
  if (!SPIFFS.begin(true)) {
    Serial.println(F("[INIT] SPIFFS mount error!"));
    Serial.println(F("[INIT] Try uploading filesystem image: pio run -t uploadfs"));
  } else {
    Serial.println(F("[INIT] SPIFFS mounted"));
    
    // Перевірка наявності файлів
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.print(F("[INIT] Files in SPIFFS: "));
    while(file){
      Serial.print(file.name());
      Serial.print(F(" "));
      file = root.openNextFile();
    }
    Serial.println();
    root.close();
  }

  // Ініціалізація дисплея
  Serial.println(F("[INIT] Initializing Display..."));
  displayInit();
  Serial.println(F("[INIT] Display ready"));

  // Ініціалізація WiFi
  Serial.println(F("[INIT] Initializing WiFi..."));
  displayShowConnecting();
  wifiConnected = appWifiInit();

  // Якщо не вдалося підключитися - виводимо інформацію про точку доступу
  if (!wifiConnected) {
    displayShowAPInfo(AP_SSID, AP_PASSWORD, appWifiGetIPAddress().c_str());
  }

  // Ініціалізація аудіо плеєра
  Serial.println(F("[INIT] Initializing Audio Player..."));
  if (audioPlayerInit()) {
    Serial.println(F("[INIT] Audio Player ready"));
  } else {
    Serial.println(F("[INIT] Audio Player failed!"));
  }

  // Ініціалізація TDA7318
  Serial.println(F("[INIT] Initializing TDA7318..."));
  if (tda7318Init()) {
    Serial.println(F("[INIT] TDA7318 ready"));
  } else {
    Serial.println(F("[INIT] TDA7318 failed!"));
  }

  // Ініціалізація кнопок
  Serial.println(F("[INIT] Initializing Buttons..."));
  buttonsInit();
  Serial.println(F("[INIT] Buttons ready"));

  // Ініціалізація енкодера
  Serial.println(F("[INIT] Initializing Encoder..."));
  encoderInit();
  Serial.println(F("[INIT] Encoder ready"));

  // Ініціалізація ІЧ пульта
  Serial.println(F("[INIT] Initializing IR Remote..."));
  irRemoteInit();
  Serial.println(F("[INIT] IR Remote ready"));

  // Отримуємо WiFi інформацію
  String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
  String wifiIP = appWifiGetIPAddress();

  // Якщо WiFi підключено - відображаємо радіо інформацію
  // Якщо AP режим - залишаємо екран з інформацією про точку доступу
  if (wifiConnected) {
    // Отримуємо поточний вхід
    TDA7318_Input currentInput = tda7318GetInput();

    // Завантажуємо останню станцію з пам'яті
    int lastStationIndex = audioPlayerLoadCurrentStation();
    if (lastStationIndex >= 0) {
      RadioStation station;
      if (audioPlayerGetStation(lastStationIndex, &station)) {
        // Встановлюємо індекс і запускаємо відтворення
        audioPlayerSetCurrentStationIndex(lastStationIndex);
        Serial.printf("[INIT] Restoring last station: %s (index=%d)\n", station.name, lastStationIndex);
        audioPlayerConnect(station.url, station.name, station.volume);
        audioPlayerSaveCurrentStation();  // Зберігаємо індекс

        // Зберігаємо назву станції для відображення
        strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);

        // Виводимо на дисплей (displayShowRadioInfo сама обробляє Computer вхід)
        displayShowRadioInfo(station.name, "-", "-", station.volume, currentInput, wifiSSID.c_str(), wifiIP.c_str(), tda7318GetVolume(), tda7318GetBalance());
      } else {
        Serial.println(F("[INIT] Last station not found"));
        displayShowRadioInfo("-", "-", "-", 0, currentInput, wifiSSID.c_str(), wifiIP.c_str(), tda7318GetVolume(), tda7318GetBalance());
      }
    } else {
      Serial.println(F("[INIT] No last station saved"));
      displayShowRadioInfo("-", "-", "-", 0, currentInput, wifiSSID.c_str(), wifiIP.c_str(), tda7318GetVolume(), tda7318GetBalance());
    }
  } else {
    Serial.println(F("[INIT] AP mode - radio screen skipped"));
  }

  // Налаштування маршрутів веб-сервера
  server.on("/", HTTP_GET, handleRoot);
  server.on("/setup", HTTP_GET, handleAPSetup);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/stations", HTTP_GET, handleStations);
  server.on("/status", HTTP_GET, appWifiHandleStatus);
  server.on("/ipaddr", HTTP_GET, handleIpAddr);
  server.on("/settings/wifi", HTTP_POST, appWifiHandleSettings);
  server.on("/settings/wifi/load", HTTP_GET, appWifiHandleLoadSettings);

  // Маршрути аудіо плеєра
  server.on("/audio/status", HTTP_GET, handleAudioStatus);
  server.on("/audio/play", HTTP_GET, handleAudioPlay);
  server.on("/audio/stop", HTTP_GET, handleAudioStop);
  server.on("/audio/volume", HTTP_GET, handleAudioVolume);
  server.on("/audio/stations", HTTP_GET, handleAudioStations);
  server.on("/audio/stations_add", HTTP_POST, handleAudioStationsAdd);
  server.on("/audio/stations_del", HTTP_POST, handleAudioStationsDel);
  server.on("/audio/stations_move", HTTP_POST, handleAudioStationsMove);
  server.on("/audio/stations_volume", HTTP_POST, handleAudioStationsVolume);
  server.on("/audio/stations_update", HTTP_POST, handleAudioStationsUpdate);
  server.on("/audio/playstation", HTTP_GET, handleAudioPlayStation);

  // Маршрути TDA7318
  server.on("/sound", HTTP_GET, handleSound);
  server.on("/sound/settings", HTTP_GET, handleSoundSettings);
  server.on("/sound/volume", HTTP_GET, handleSoundVolume);
  server.on("/sound/bass", HTTP_GET, handleSoundBass);
  server.on("/sound/treble", HTTP_GET, handleSoundTreble);
  server.on("/sound/balance", HTTP_GET, handleSoundBalance);
  server.on("/sound/input", HTTP_GET, handleSoundInput);
  server.on("/sound/mute", HTTP_GET, handleSoundMute);
  server.on("/sound/reset", HTTP_POST, handleSoundReset);

  server.serveStatic("/", SPIFFS, "/");
  server.onNotFound(handleRoot);

  // Запуск сервера
  server.begin();
  Serial.println(F("[INIT] HTTP server started"));

  // Переходимо в режим очікування після всіх ініціалізацій
  deviceEnterSleep(true);  // true = залишити WiFi активним

  Serial.println(F("================================="));
}

// ============================================================================
// Головний цикл
// ============================================================================

/**
 * @brief Функція loop() - виконується нескінченно
 */
void loop() {
  // Обробка кнопки PWR - вхід/вихід з режиму сну (працює в усіх режимах)
  buttonsLoop();
  if (buttonsWasClicked(BTN_PWR)) {
    buttonsClearClicked(BTN_PWR);
    if (deviceSleep) {
      // Прокинутися
      deviceWakeUp();
    } else {
      // Заснути
      deviceEnterSleep(false);  // false = вимкнути WiFi
    }
    return;
  }

  // Якщо пристрій в режимі сну - перевіряємо тільки ІЧ пульт для пробудження
  if (deviceSleep) {
    irRemoteLoop();
    uint8_t irKey = irRemoteGetLastKey();
    if (irKey != 0xFF) {
      if (irKey == IR_BTN_POWER) {
        Serial.println("[IR] POWER key pressed - waking up from sleep");
        deviceWakeUp();
        irLastKeyTime = millis();  // <--cooldown ПІСЛЯ wake up
        Serial.println("[IR] Wakeup complete, cooldown started");
      }
    }
    delay(10);
    return;
  }

  // Обробка ІЧ пульта (працює в активному режимі)
  irRemoteLoop();
  uint8_t irKey = irRemoteGetLastKey();
  if (irKey != 0xFF) {
    // Для кнопки POWER застосовуємо cooldown щоб уникнути подвійних спрацювань
    if (irKey == IR_BTN_POWER) {
      if (millis() - irLastKeyTime < IR_COOLDOWN_MS) {
        Serial.printf("[IR] POWER cooldown active (%lu ms)\n", millis() - irLastKeyTime);
        return;
      }
      irLastKeyTime = millis();  // Запам'ятовуємо час сигналу
    }
    handleIRRemote(irKey);
  }

  server.handleClient();

  // Офлайн режим - окрема обробка
  if (offlineMode) {
    encoderLoop();
    buttonsLoop();

    // Утримування кнопки PWR - перейти в AP режим
    if (buttonsWasLongPressed(BTN_PWR)) {
      buttonsClearLongPressed(BTN_PWR);
      Serial.println("[Offline] PWR long press - entering AP mode...");

      // Скидаємо офлайн режим
      offlineMode = false;

      // Показуємо екран підключення
      displayShowConnecting();

      // Вмикаємо WiFi і намагаємось підключитися
      wifiConnected = appWifiInit();

      if (!wifiConnected) {
        // Не вдалося підключитися - показуємо AP інфо
        displayShowAPInfo(AP_SSID, AP_PASSWORD, appWifiGetIPAddress().c_str());
      } else {
        // Підключено - перезапускаємо сервер і показуємо радіо
        server.begin();
        String wifiSSID = WiFi.SSID();
        String wifiIP = appWifiGetIPAddress();
        TDA7318_Input currentInput = tda7318GetInput();
        uint8_t tdaVolume = tda7318GetVolume();
        displayShowRadioInfo("-", "-", "-", 0, currentInput, wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, tda7318GetBalance());
      }
      return;
    }

    // Регулювання гучності енкодером
    int encChange = encoderGetChange();
    if (encChange != 0) {
      // Якщо відкрито меню звуку - регулюємо активний параметр
      if (soundSettingsOpen) {
        lastSoundSettingsTime = millis();
        if (activeToneControl == 0) {
          uint8_t tdaVolume = tda7318GetVolume();
          if (encChange > 0 && tdaVolume < 100) {
            tdaVolume++;
            tda7318SetVolume(tdaVolume);
          } else if (encChange < 0 && tdaVolume > 0) {
            tdaVolume--;
            tda7318SetVolume(tdaVolume);
          }
          displayDrawTDAVolumeBar(tdaVolume);
        } else if (activeToneControl == 1) {
          int8_t bass = tda7318GetBass();
          int8_t newBass = bass;
          if (encChange > 0 && bass < 7) {
            newBass++;
            tda7318SetBass(newBass);
          } else if (encChange < 0 && bass > -7) {
            newBass--;
            tda7318SetBass(newBass);
          }
          if (newBass != bass) {
            displayUpdateToneBarBassSegment(bass, newBass, BASS_BAR_X);
            lastBassValue = newBass;
          }
        } else if (activeToneControl == 2) {
          int8_t treble = tda7318GetTreble();
          int8_t newTreble = treble;
          if (encChange > 0 && treble < 7) {
            newTreble++;
            tda7318SetTreble(newTreble);
          } else if (encChange < 0 && treble > -7) {
            newTreble--;
            tda7318SetTreble(newTreble);
          }
          if (newTreble != treble) {
            displayUpdateToneBarTrebleSegment(treble, newTreble, TREBLE_BAR_X);
            lastTrebleValue = newTreble;
          }
        } else if (activeToneControl == 3) {
          int8_t balance = tda7318GetBalance();
          if (encChange > 0 && balance < 7) {
            balance++;
            tda7318SetBalance(balance);
          } else if (encChange < 0 && balance > -7) {
            balance--;
            tda7318SetBalance(balance);
          }
          displayDrawBalanceBar(balance);
        }
      } else {
        // Регулюємо гучність
        uint8_t tdaVolume = tda7318GetVolume();
        if (encChange > 0 && tdaVolume < 100) {
          tdaVolume++;
          tda7318SetVolume(tdaVolume);
        } else if (encChange < 0 && tdaVolume > 0) {
          tdaVolume--;
          tda7318SetVolume(tdaVolume);
        }
        displayDrawTDAVolumeBar(tdaVolume);
      }
    }

    // Кнопка енкодера - відкрити/закрити меню звуку
    if (encoderButtonWasPressed()) {
      if (soundSettingsOpen) {
        // Перемкнути активний елемент
        lastSoundSettingsTime = millis();
        soundSettingsTimeoutLogged = false;
        uint8_t oldControl = activeToneControl;
        activeToneControl = (activeToneControl + 1) % 4;
        uint8_t tdaVolume = tda7318GetVolume();
        int8_t balance = tda7318GetBalance();
        int8_t bass = tda7318GetBass();
        int8_t treble = tda7318GetTreble();
        displayUpdateActiveIndicator(oldControl, activeToneControl, tdaVolume, bass, treble, balance);
      } else {
        // Відкрити меню звуку
        soundSettingsOpen = true;
        lastSoundSettingsTime = millis();
        soundSettingsTimeoutLogged = false;
        activeToneControl = 0;
        lastActiveControl = 0;
        lastBassValue = tda7318GetBass();
        lastTrebleValue = tda7318GetTreble();
        uint8_t tdaVolume = tda7318GetVolume();
        int8_t balance = tda7318GetBalance();
        int8_t bass = tda7318GetBass();
        int8_t treble = tda7318GetTreble();
        displayShowSoundSettings(tdaVolume, balance, bass, treble, activeToneControl);
      }
    }

    // Кнопка OK - закрити меню звуку
    if (buttonsWasClicked(BTN_OK)) {
      buttonsClearClicked(BTN_OK);
      if (soundSettingsOpen) {
        soundSettingsOpen = false;
        displayShowOffline();
      }
    }

    // Кнопки LEFT/RIGHT - перемикання входів (тільки якщо меню звуку закрите)
    // LEFT = попередній вхід, RIGHT = наступний вхід
    // Цикл: Computer → TV Box → AUX → Computer
    if (!soundSettingsOpen) {
      bool leftPressed = buttonsWasClicked(BTN_LEFT);
      bool rightPressed = buttonsWasClicked(BTN_RIGHT);

      if (leftPressed || rightPressed) {
        buttonsClearClicked(BTN_LEFT);
        buttonsClearClicked(BTN_RIGHT);

        TDA7318_Input currentInput = tda7318GetInput();
        TDA7318_Input newInput;

        // Якщо обидві натиснуті - ігноруємо (можливий дребезг)
        if (leftPressed && rightPressed) {
          Serial.println("[Offline] Both buttons pressed, ignoring");
        } else if (leftPressed) {
          // Попередній вхід: Computer ← TV Box ← AUX ← Computer
          if (currentInput == INPUT_COMPUTER) {
            newInput = INPUT_AUX;
          } else if (currentInput == INPUT_TV_BOX) {
            newInput = INPUT_COMPUTER;
          } else {
            newInput = INPUT_TV_BOX;
          }
          Serial.printf("[Offline] LEFT pressed, switching from %d to %d\n", currentInput, newInput);

          handleInputChange(newInput);
        } else {
          // Наступний вхід: Computer → TV Box → AUX → Computer
          if (currentInput == INPUT_COMPUTER) {
            newInput = INPUT_TV_BOX;
          } else if (currentInput == INPUT_TV_BOX) {
            newInput = INPUT_AUX;
          } else {
            newInput = INPUT_COMPUTER;
          }
          Serial.printf("[Offline] RIGHT pressed, switching from %d to %d\n", currentInput, newInput);

          handleInputChange(newInput);
        }
      }
    }

    // Автоматичне закриття меню звуку
    if (soundSettingsOpen) {
      unsigned long currentTime = millis();
      unsigned long elapsedTime = currentTime - lastSoundSettingsTime;
      if (elapsedTime > SOUND_SETTINGS_TIMEOUT_MS) {
        soundSettingsOpen = false;
        displayShowOffline();
        Serial.println("[Offline] Sound settings auto-closed");
      }
    }

    return;
  }

  // AP режим - тільки кнопка OK для переходу в офлайн
  if (!wifiConnected && !offlineMode) {
    buttonsLoop();

    // Кнопка OK - перейти в офлайн режим
    if (buttonsWasClicked(BTN_OK)) {
      buttonsClearClicked(BTN_OK);
      enterOfflineMode();
      return;
    }

    // В AP режимі сервер обробляє веб-інтерфейс налаштувань
    server.handleClient();
    return;
  }

  audioPlayerLoop();  // Обробка аудіо потоку

  // Автоматичне закриття списку станцій через 10 секунд бездіяльності
  if (stationListOpen && (millis() - lastButtonTimeInList > LIST_AUTO_CLOSE_MS)) {
    Serial.println("[Buttons] Auto-closing station list after 10s timeout");
    stationListOpen = false;
    
    // Запускаємо відтворення станції яка грала до відкриття списку
    if (stationIndexBeforeOpen >= 0 && stationIndexBeforeOpen < stationListCount) {
      audioPlayerConnect(stationsBuffer[stationIndexBeforeOpen].url, stationsBuffer[stationIndexBeforeOpen].name, stationsBuffer[stationIndexBeforeOpen].volume);
      audioPlayerSetCurrentStationIndex(stationIndexBeforeOpen);
      audioPlayerSaveCurrentStation();  // Зберігаємо індекс
      Serial.printf("[Buttons] Auto-resume playing: %s\n", stationsBuffer[stationIndexBeforeOpen].name);
    }
    
    // Оновлюємо весь дисплей
    uint8_t espVolume = audioPlayerGetVolume();
    uint8_t tdaVolume = tda7318GetVolume();
    const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
    String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
    String wifiIP = appWifiGetIPAddress();
    displayShowRadioInfo(displayStation, audioPlayerGetArtist(), audioPlayerGetTitle(), espVolume, tda7318GetInput(), wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, tda7318GetBalance());
  }
  
  // Перевірка таймеру для очищення написів NEXT/PREV/PLAY
  // Очищуємо напис тільки якщо за 5 секунд не з'явилося інфо про трек
  if (messageDisplayed && (millis() - messageDisplayTime > MESSAGE_TIMEOUT_MS)) {
    // Перевіряємо чи з'явилося інфо про трек
    const char* artist = audioPlayerGetArtist();
    const char* title = audioPlayerGetTitle();

    TDA7318_Input activeInput = tda7318GetInput();
    uint8_t espVolume = audioPlayerGetVolume();
    uint8_t tdaVolume = tda7318GetVolume();
    const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();

    // Якщо інфо про трек є (не пусте) - не очищуємо напис, воно залишиться на екрані
    // Якщо інфо немає - виводимо станцію без артиста/назви
    if (strlen(artist) > 0 || strlen(title) > 0) {
      // Інфо з'явилося - дисплей оновиться автоматично через web_handlers
      Serial.println("[Buttons] Track info available - keeping");
    } else {
      // Інфо немає - виводимо станцію без артиста/назви
      displayUpdateStationAndTrack(displayStation, "", "", true, espVolume, tdaVolume, activeInput, tda7318GetBalance());
      Serial.println("[Buttons] Message timeout - cleared (empty info)");
    }
    messageDisplayed = false;
  }

  // Опитування кнопок та енкодера
  buttonsLoop();
  encoderLoop();

  // Обробка енкодера (регулювання гучності TDA7318) - тільки якщо список закритий
  int encChange = encoderGetChange();
  if (encChange != 0) {
    if (stationListOpen) {
      // Якщо список відкритий - закриваємо його і запускаємо відтворення
      stationListOpen = false;

      // Запускаємо відтворення станції яка грала до відкриття списку
      if (stationIndexBeforeOpen >= 0 && stationIndexBeforeOpen < stationListCount) {
        audioPlayerConnect(stationsBuffer[stationIndexBeforeOpen].url, stationsBuffer[stationIndexBeforeOpen].name, stationsBuffer[stationIndexBeforeOpen].volume);
        audioPlayerSetCurrentStationIndex(stationIndexBeforeOpen);
        audioPlayerSaveCurrentStation();  // Зберігаємо індекс
        Serial.printf("[Encoder] Close list - Resume playing: %s\n", stationsBuffer[stationIndexBeforeOpen].name);
      }

      // Оновлюємо весь дисплей з поточною інформацією
      uint8_t espVolume = audioPlayerGetVolume();
      uint8_t tdaVolume = tda7318GetVolume();
      const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
      String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
      String wifiIP = appWifiGetIPAddress();
      displayShowRadioInfo(displayStation, audioPlayerGetArtist(), audioPlayerGetTitle(), espVolume, tda7318GetInput(), wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, tda7318GetBalance());
    } else if (soundSettingsOpen) {
      // Регулюємо активний параметр в режимі налаштувань звуку
      lastSoundSettingsTime = millis();  // Оновлюємо таймер
      soundSettingsTimeoutLogged = false;  // Скидаємо прапорець відладки
      Serial.printf("[Sound Settings] ADJUST at %lu\n", lastSoundSettingsTime);
      if (activeToneControl == 0) {
        // Гучність
        uint8_t tdaVolume = tda7318GetVolume();
        if (encChange > 0) {
          if (tdaVolume < 100) {
            tdaVolume++;
            tda7318SetVolume(tdaVolume);
            Serial.printf("[Encoder] Volume UP: %d\n", tdaVolume);
          }
        } else {
          if (tdaVolume > 0) {
            tdaVolume--;
            tda7318SetVolume(tdaVolume);
            Serial.printf("[Encoder] Volume DOWN: %d\n", tdaVolume);
          }
        }
        // Оновити тільки індикатор гучності
        displayDrawTDAVolumeBar(tdaVolume);
      } else if (activeToneControl == 1) {
        // Бас
        int8_t bass = tda7318GetBass();
        int8_t newBass = bass;
        if (encChange > 0) {
          if (bass < 7) {
            newBass++;
            tda7318SetBass(newBass);
            Serial.printf("[Encoder] Bass UP: %d -> %d\n", bass, newBass);
          }
        } else {
          if (bass > -7) {
            newBass--;
            tda7318SetBass(newBass);
            Serial.printf("[Encoder] Bass DOWN: %d -> %d\n", bass, newBass);
          }
        }
        // Оновити тільки змінені сегменти індикатора басу
        if (newBass != bass) {
          displayUpdateToneBarBassSegment(bass, newBass, BASS_BAR_X);
          lastBassValue = newBass;
        }
      } else if (activeToneControl == 2) {
        // Тембр
        int8_t treble = tda7318GetTreble();
        int8_t newTreble = treble;
        if (encChange > 0) {
          if (treble < 7) {
            newTreble++;
            tda7318SetTreble(newTreble);
            Serial.printf("[Encoder] Treble UP: %d -> %d\n", treble, newTreble);
          }
        } else {
          if (treble > -7) {
            newTreble--;
            tda7318SetTreble(newTreble);
            Serial.printf("[Encoder] Treble DOWN: %d -> %d\n", treble, newTreble);
          }
        }
        // Оновити тільки змінені сегменти індикатора тембру
        if (newTreble != treble) {
          displayUpdateToneBarTrebleSegment(treble, newTreble, TREBLE_BAR_X);
          lastTrebleValue = newTreble;
        }
      } else if (activeToneControl == 3) {
        // Баланс
        int8_t balance = tda7318GetBalance();
        if (encChange > 0) {
          if (balance < 7) {
            balance++;
            tda7318SetBalance(balance);
            Serial.printf("[Encoder] Balance UP: %d\n", balance);
          }
        } else {
          if (balance > -7) {
            balance--;
            tda7318SetBalance(balance);
            Serial.printf("[Encoder] Balance DOWN: %d\n", balance);
          }
        }
        // Оновити тільки індикатор балансу
        displayDrawBalanceBar(balance);
      }
    } else {
      // Регулюємо гучність тільки якщо список закритий
      uint8_t tdaVolume = tda7318GetVolume();
      if (encChange > 0) {
        // Обертання вправо - збільшити гучність
        if (tdaVolume < 100) {
          tdaVolume++;
          tda7318SetVolume(tdaVolume);
          Serial.printf("[Encoder] Volume UP: %d\n", tdaVolume);
        }
      } else {
        // Обертання вліво - зменшити гучність
        if (tdaVolume > 0) {
          tdaVolume--;
          tda7318SetVolume(tdaVolume);
          Serial.printf("[Encoder] Volume DOWN: %d\n", tdaVolume);
        }
      }
      // Оновити індикатор гучності на дисплеї
      displayUpdateVolumeBars(audioPlayerGetVolume(), tdaVolume, tda7318GetBalance());
    }
  }

  // Кнопки UP/DOWN/LEFT/RIGHT - закрити регулювання звуку якщо відкрито
  if (soundSettingsOpen) {
    if (buttonsWasClicked(BTN_UP) || buttonsWasClicked(BTN_DOWN) ||
        buttonsWasClicked(BTN_LEFT) || buttonsWasClicked(BTN_RIGHT)) {
      // Закрити налаштування звуку
      soundSettingsOpen = false;
      soundSettingsTimeoutLogged = false;
      // Оновити весь дисплей
      uint8_t espVolume = audioPlayerGetVolume();
      uint8_t tdaVolume = tda7318GetVolume();
      int8_t balance = tda7318GetBalance();
      const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
      String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
      String wifiIP = appWifiGetIPAddress();
      displayShowRadioInfo(displayStation, audioPlayerGetArtist(), audioPlayerGetTitle(), espVolume, tda7318GetInput(), wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, balance);
      return;
    }
  }

  // Кнопка енкодера - закрити список якщо відкритий, або перемкнути активний елемент, або відкрити налаштування звуку
  if (encoderButtonWasPressed()) {
    if (stationListOpen) {
      stationListOpen = false;

      // Запускаємо відтворення станції яка грала до відкриття списку
      if (stationIndexBeforeOpen >= 0 && stationIndexBeforeOpen < stationListCount) {
        audioPlayerConnect(stationsBuffer[stationIndexBeforeOpen].url, stationsBuffer[stationIndexBeforeOpen].name, stationsBuffer[stationIndexBeforeOpen].volume);
        audioPlayerSetCurrentStationIndex(stationIndexBeforeOpen);
        audioPlayerSaveCurrentStation();  // Зберігаємо індекс
        Serial.printf("[Encoder BTN] Close list - Resume playing: %s\n", stationsBuffer[stationIndexBeforeOpen].name);
      }

      // Оновлюємо весь дисплей
      uint8_t espVolume = audioPlayerGetVolume();
      uint8_t tdaVolume = tda7318GetVolume();
      const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
      String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
      String wifiIP = appWifiGetIPAddress();
      displayShowRadioInfo(displayStation, audioPlayerGetArtist(), audioPlayerGetTitle(), espVolume, tda7318GetInput(), wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, tda7318GetBalance());
    } else if (soundSettingsOpen) {
      // Перемкнути активний елемент керування
      lastSoundSettingsTime = millis();  // Оновлюємо таймер
      soundSettingsTimeoutLogged = false;  // Скидаємо прапорець відладки
      Serial.printf("[Sound Settings] SWITCH at %lu\n", lastSoundSettingsTime);
      uint8_t oldControl = activeToneControl;
      activeToneControl = (activeToneControl + 1) % 4;  // 0→1→2→3→0
      Serial.printf("[Encoder BTN] Active control: %d -> %d\n", oldControl, activeToneControl);
      // Оновити тільки індикатори та кольори тексту
      uint8_t tdaVolume = tda7318GetVolume();
      int8_t balance = tda7318GetBalance();
      int8_t bass = tda7318GetBass();
      int8_t treble = tda7318GetTreble();
      displayUpdateActiveIndicator(oldControl, activeToneControl, tdaVolume, bass, treble, balance);
    } else {
      // Відкрити налаштування звуку
      soundSettingsOpen = true;
      lastSoundSettingsTime = millis();  // Встановлюємо таймер
      soundSettingsTimeoutLogged = false;  // Скидаємо прапорець відладки
      Serial.printf("[Sound Settings] OPENED at %lu\n", lastSoundSettingsTime);
      activeToneControl = 0;  // Починаємо з гучності
      lastActiveControl = 0;
      // Ініціалізуємо попередні значення
      lastBassValue = tda7318GetBass();
      lastTrebleValue = tda7318GetTreble();
      uint8_t tdaVolume = tda7318GetVolume();
      int8_t balance = tda7318GetBalance();
      int8_t bass = tda7318GetBass();
      int8_t treble = tda7318GetTreble();
      displayShowSoundSettings(tdaVolume, balance, bass, treble, activeToneControl);
    }
  }

  // Кнопка OK - закрити налаштування звуку
  if (soundSettingsOpen && buttonsWasClicked(BTN_OK)) {
    buttonsClearClicked(BTN_OK);
    // Закрити налаштування звуку
    soundSettingsOpen = false;
    // Оновити весь дисплей
    uint8_t espVolume = audioPlayerGetVolume();
    uint8_t tdaVolume = tda7318GetVolume();
    int8_t balance = tda7318GetBalance();
    const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
    String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
    String wifiIP = appWifiGetIPAddress();
    displayShowRadioInfo(displayStation, audioPlayerGetArtist(), audioPlayerGetTitle(), espVolume, tda7318GetInput(), wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, balance);
  }

  // Кнопка UP - перемикання станцій (WiFi вхід, список закритий) або Play/Stop
  if (buttonsWasClicked(BTN_UP)) {
    buttonsClearClicked(BTN_UP);
    
    TDA7318_Input activeInput = tda7318GetInput();
    
    // Якщо список закритий і WiFi вхід - перемкнути на попередню станцію
    if (!stationListOpen && activeInput == INPUT_WIFI_RADIO) {
      int currentIndex = audioPlayerGetCurrentStationIndex();
      int stationCount = audioPlayerGetStationCount();
      
      if (stationCount > 0 && currentIndex >= 0) {
        // Попередня станція (кільцеве переміщення)
        int newIndex = (currentIndex > 0) ? currentIndex - 1 : stationCount - 1;
        
        RadioStation station;
        if (audioPlayerGetStation(newIndex, &station)) {
          // СПОЧАТКУ виводимо PREV на дисплей (в trackArtist щоб було по центру)
          uint8_t espVolume = audioPlayerGetVolume();
          uint8_t tdaVolume = tda7318GetVolume();
          displayUpdateStationAndTrack(station.name, "PREV", "", true, espVolume, tdaVolume, activeInput, tda7318GetBalance());
          messageDisplayTime = millis();
          messageDisplayed = true;
          
          // Потім запускаємо відтворення
          audioPlayerStop();
          delay(100);
          audioPlayerConnect(station.url, station.name, station.volume);
          audioPlayerSetCurrentStationIndex(newIndex);
          audioPlayerSaveCurrentStation();  // Зберігаємо індекс
          strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);

          Serial.printf("[Buttons] UP - Previous station: %s (index=%d)\n", station.name, newIndex);
        }
      }
      return;
    }
    
    // Якщо список відкритий - навігація по списку
    if (stationListOpen && stationListCount > 0) {
      lastButtonTimeInList = millis();  // Оновлюємо таймер

      if (targetStationIndex > 0) {
        targetStationIndex--;
      } else {
        targetStationIndex = stationListCount - 1;
      }
      Serial.printf("[Buttons] UP - Target index: %d\n", targetStationIndex);

      int newPage = targetStationIndex / STATIONS_PER_PAGE;
      if (newPage != stationListPage) {
        stationListPage = newPage;
        displayShowStationList(stationsBuffer, stationListCount, targetStationIndex, stationListPage, STATIONS_PER_PAGE);
      } else {
        displayUpdateStationListSelection(stationsBuffer, stationListCount, stationListIndex, targetStationIndex, stationListPage, STATIONS_PER_PAGE);
      }
      stationListIndex = targetStationIndex;
    }
  }

  // Кнопка DOWN - перемикання станцій (WiFi вхід, список закритий) або навігація в списку
  if (buttonsWasClicked(BTN_DOWN)) {
    buttonsClearClicked(BTN_DOWN);
    
    TDA7318_Input activeInput = tda7318GetInput();
    
    // Якщо список закритий і WiFi вхід - перемкнути на наступну станцію
    if (!stationListOpen && activeInput == INPUT_WIFI_RADIO) {
      int currentIndex = audioPlayerGetCurrentStationIndex();
      int stationCount = audioPlayerGetStationCount();
      
      if (stationCount > 0 && currentIndex >= 0) {
        // Наступна станція (кільцеве переміщення)
        int newIndex = (currentIndex < stationCount - 1) ? currentIndex + 1 : 0;
        
        RadioStation station;
        if (audioPlayerGetStation(newIndex, &station)) {
          // СПОЧАТКУ виводимо NEXT на дисплей (в trackArtist щоб було по центру)
          uint8_t espVolume = audioPlayerGetVolume();
          uint8_t tdaVolume = tda7318GetVolume();
          displayUpdateStationAndTrack(station.name, "NEXT", "", true, espVolume, tdaVolume, activeInput, tda7318GetBalance());
          messageDisplayTime = millis();
          messageDisplayed = true;
          
          // Потім запускаємо відтворення
          audioPlayerStop();
          delay(100);
          audioPlayerConnect(station.url, station.name, station.volume);
          audioPlayerSetCurrentStationIndex(newIndex);
          audioPlayerSaveCurrentStation();  // Зберігаємо індекс
          strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);

          Serial.printf("[Buttons] DOWN - Next station: %s (index=%d)\n", station.name, newIndex);
        }
      }
      return;
    }
    
    // Якщо список відкритий - навігація по списку
    if (stationListOpen && stationListCount > 0) {
      lastButtonTimeInList = millis();  // Оновлюємо таймер

      if (targetStationIndex < stationListCount - 1) {
        targetStationIndex++;
      } else {
        targetStationIndex = 0;
      }
      Serial.printf("[Buttons] DOWN - Target index: %d\n", targetStationIndex);

      int newPage = targetStationIndex / STATIONS_PER_PAGE;
      if (newPage != stationListPage) {
        stationListPage = newPage;
        displayShowStationList(stationsBuffer, stationListCount, targetStationIndex, stationListPage, STATIONS_PER_PAGE);
      } else {
        displayUpdateStationListSelection(stationsBuffer, stationListCount, stationListIndex, targetStationIndex, stationListPage, STATIONS_PER_PAGE);
      }
      stationListIndex = targetStationIndex;
    }
  }

  // Обробка кнопок LEFT/RIGHT для перемикання входів TDA7318
  TDA7318_Input currentInput = tda7318GetInput();
  bool inputChanged = false;
  TDA7318_Input newInput = INPUT_WIFI_RADIO;  // Ініціалізуємо змінну для нового входу

  // Кнопка LEFT - попередній вхід (закриває список якщо відкритий) або попередня сторінка (циклічно)
  if (buttonsWasClicked(BTN_LEFT)) {
    buttonsClearClicked(BTN_LEFT);
    if (stationListOpen) {
      lastButtonTimeInList = millis();  // Оновлюємо таймер
      // Перемкнути на попередню сторінку (циклічно)
      int totalPages = (stationListCount + STATIONS_PER_PAGE - 1) / STATIONS_PER_PAGE;
      if (totalPages > 1) {
        stationListPage = (stationListPage > 0) ? stationListPage - 1 : totalPages - 1;
        int pageStart = stationListPage * STATIONS_PER_PAGE;
        // Встановлюємо індекс на першу станцію нової сторінки
        targetStationIndex = pageStart;
        stationListIndex = targetStationIndex;
        displayShowStationList(stationsBuffer, stationListCount, stationListIndex, stationListPage, STATIONS_PER_PAGE);
        Serial.printf("[Buttons] LEFT - Page %d of %d\n", stationListPage + 1, totalPages);
      }
    } else {
      newInput = (TDA7318_Input)((currentInput - 1 + 4) % 4);
      handleInputChange(newInput);
      inputChanged = true;
      Serial.printf("[Buttons] LEFT pressed - Input: %d\n", newInput);
    }
  }

  // Кнопка RIGHT - наступний вхід (закриває список якщо відкритий) або наступна сторінка (циклічно)
  if (buttonsWasClicked(BTN_RIGHT)) {
    buttonsClearClicked(BTN_RIGHT);
    if (stationListOpen) {
      lastButtonTimeInList = millis();  // Оновлюємо таймер
      // Перемкнути на наступну сторінку (циклічно)
      int totalPages = (stationListCount + STATIONS_PER_PAGE - 1) / STATIONS_PER_PAGE;
      if (totalPages > 1) {
        stationListPage = (stationListPage < totalPages - 1) ? stationListPage + 1 : 0;
        int pageStart = stationListPage * STATIONS_PER_PAGE;
        // Встановлюємо індекс на першу станцію нової сторінки
        targetStationIndex = pageStart;
        if (targetStationIndex >= stationListCount) targetStationIndex = stationListCount - 1;
        stationListIndex = targetStationIndex;
        displayShowStationList(stationsBuffer, stationListCount, stationListIndex, stationListPage, STATIONS_PER_PAGE);
        Serial.printf("[Buttons] RIGHT - Page %d of %d\n", stationListPage + 1, totalPages);
      }
    } else {
      newInput = (TDA7318_Input)((currentInput + 1) % 4);
      handleInputChange(newInput);
      inputChanged = true;
      Serial.printf("[Buttons] RIGHT pressed - Input: %d\n", newInput);
    }
  }

  // Кнопка OK - відкрити/закрити список станцій (тільки для WiFi входу)
  // LONG_PRESS - відкрити список, CLICK - запустити станцію
  if (buttonsWasLongPressed(BTN_OK)) {
    buttonsClearLongPressed(BTN_OK);
    TDA7318_Input activeInput = tda7318GetInput();

    if (activeInput == INPUT_WIFI_RADIO && !stationListOpen) {
      // Зупинити відтворення перед відкриттям списку
      audioPlayerStop();
      
      // Зберігаємо індекс станції яка грала до відкриття списку
      stationIndexBeforeOpen = audioPlayerGetCurrentStationIndex();
      if (stationIndexBeforeOpen < 0) stationIndexBeforeOpen = 0;
      
      // Відкрити список станцій - ЗАВАНТАЖИТИ ВЕСЬ СПИСОК
      stationListOpen = true;
      okLongPressTime = millis();  // Запам'ятовуємо час LONG_PRESS
      lastButtonTimeInList = millis();  // Запам'ятовуємо час відкриття списку

      // ЗАВАНТАЖИТИ ВЕСЬ СПИСОК ОДРАЗУ
      stationListCount = audioPlayerGetStationCount();
      if (stationListCount > 0) {
        for (int i = 0; i < stationListCount; i++) {
          audioPlayerGetStation(i, &stationsBuffer[i]);
        }
      }

      // Встановлюємо поточний індекс з плеєра
      int currentIndex = audioPlayerGetCurrentStationIndex();
      Serial.printf("[Buttons] Current station index from player: %d\n", currentIndex);
      if (currentIndex < 0 || currentIndex >= stationListCount) {
        Serial.printf("[Buttons] Index %d out of range, resetting to 0\n", currentIndex);
        currentIndex = 0;
      }

      // Ініціалізуємо обидві змінні однаковим значенням
      stationListIndex = currentIndex;
      targetStationIndex = currentIndex;

      // Визначаємо сторінку на якій знаходиться поточна станція
      stationListPage = stationListIndex / STATIONS_PER_PAGE;

      // Показуємо список
      displayShowStationList(stationsBuffer, stationListCount, stationListIndex, stationListPage, STATIONS_PER_PAGE);
      Serial.printf("[Buttons] OK Long Press - Opened station list (%d stations, page %d, index %d)\n", stationListCount, stationListPage, stationListIndex);
    }
  }
  
  // Коротке натискання OK - запустити поточну станцію (якщо список закритий)
  // Пропускаємо якщо недавно був LONG_PRESS (cooldown)
  if (buttonsWasClicked(BTN_OK)) {
    buttonsClearClicked(BTN_OK);

    // Перевіряємо cooldown після LONG_PRESS
    if (millis() - okLongPressTime < OK_COOLDOWN_MS) {
      // Ще не пройшов cooldown - ігноруємо CLICK
      return;
    }

    // Якщо AP режим - перейти в офлайн
    if (!wifiConnected && !offlineMode) {
      enterOfflineMode();
      return;
    }

    // В офлайн режимі - ігноруємо радіо функції
    if (offlineMode) {
      return;
    }

    TDA7318_Input activeInput = tda7318GetInput();

    // Якщо список відкритий - закрити і запустити вибрану станцію
    if (stationListOpen) {
      lastButtonTimeInList = millis();  // Оновлюємо таймер
      stationListOpen = false;

      if (stationListCount > 0 && stationListIndex >= 0 && stationListIndex < stationListCount) {
        // Зупинити поточне відтворення
        audioPlayerStop();
        delay(100);

        // Запустити вибрану станцію
        audioPlayerConnect(stationsBuffer[stationListIndex].url, stationsBuffer[stationListIndex].name, stationsBuffer[stationListIndex].volume);
        audioPlayerSetCurrentStationIndex(stationListIndex);
        audioPlayerSaveCurrentStation();  // Зберігаємо індекс
        strncpy(lastKnownStation, stationsBuffer[stationListIndex].name, sizeof(lastKnownStation) - 1);

        Serial.printf("[Buttons] OK - Playing: %s (index=%d)\n", stationsBuffer[stationListIndex].name, stationListIndex);

        // Оновити дисплей
        uint8_t espVolume = audioPlayerGetVolume();
        uint8_t tdaVolume = tda7318GetVolume();
        String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
        String wifiIP = appWifiGetIPAddress();
        displayShowRadioInfo(stationsBuffer[stationListIndex].name, "-", "-", espVolume, activeInput, wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, tda7318GetBalance());
      }
    }
    // Якщо список закритий і WiFi вхід - Play/Stop як кнопка UP
    else if (activeInput == INPUT_WIFI_RADIO) {
      bool wasPlaying = (audioPlayerGetState() == PLAYER_PLAYING);

      if (wasPlaying) {
        // Зупинити відтворення
        audioPlayerStop();
        Serial.println("[Buttons] OK - Stopped");

        // Оновити дисплей - виводимо STOP (залишається на екрані)
        uint8_t espVolume = audioPlayerGetVolume();
        uint8_t tdaVolume = tda7318GetVolume();
        const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
        displayUpdateStationAndTrack(displayStation, "", "", false, espVolume, tdaVolume, activeInput);
      } else {
        // Запустити останню станцію - СПОЧАТКУ виводимо PLAY
        int lastStationIndex = audioPlayerGetCurrentStationIndex();
        if (lastStationIndex >= 0) {
          RadioStation station;
          if (audioPlayerGetStation(lastStationIndex, &station)) {
            // СПОЧАТКУ виводимо PLAY на дисплей
            uint8_t espVolume = audioPlayerGetVolume();
            uint8_t tdaVolume = tda7318GetVolume();
            displayUpdateStationAndTrack(station.name, "PLAY", "", true, espVolume, tdaVolume, activeInput, tda7318GetBalance());
            messageDisplayTime = millis();
            messageDisplayed = true;

            // Потім запускаємо відтворення
            audioPlayerConnect(station.url, station.name, station.volume);
            audioPlayerSaveCurrentStation();  // Зберігаємо індекс
            Serial.printf("[Buttons] OK - Playing: %s\n", station.name);
            strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);
          }
        }
      }
    }
  }

  // Оновлення дисплея при зміні входу (тільки якщо список закритий)
  // handleInputChange() вже оновив дисплей, тому тільки оновлюємо панель входів
  if (inputChanged && !stationListOpen) {
    lastInput = tda7318GetInput();
    displayUpdateAudioInputBar(lastInput);
  }

  // Автоматичне закриття налаштувань звуку через 10 секунд бездіяльності
  if (soundSettingsOpen) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - lastSoundSettingsTime;
    
    // Виводимо відладку кожні 2 секунди
    if (!soundSettingsTimeoutLogged && (elapsedTime > 2000)) {
      Serial.printf("[Sound Settings] OPEN for %lu ms (timeout at %lu ms)\n", elapsedTime, SOUND_SETTINGS_TIMEOUT_MS);
      soundSettingsTimeoutLogged = true;
    }
    
    if (elapsedTime > SOUND_SETTINGS_TIMEOUT_MS) {
      Serial.printf("[Sound Settings] AUTO-CLOSE after %lu ms\n", elapsedTime);
      soundSettingsOpen = false;
      soundSettingsTimeoutLogged = false;
      
      // Оновити весь дисплей
      uint8_t espVolume = audioPlayerGetVolume();
      uint8_t tdaVolume = tda7318GetVolume();
      int8_t balance = tda7318GetBalance();
      const char* displayStation = (strlen(lastKnownStation) > 0) ? lastKnownStation : audioPlayerGetStationName();
      String wifiSSID = appWifiIsConnected() ? WiFi.SSID() : "";
      String wifiIP = appWifiGetIPAddress();
      displayShowRadioInfo(displayStation, audioPlayerGetArtist(), audioPlayerGetTitle(), espVolume, tda7318GetInput(), wifiSSID.c_str(), wifiIP.c_str(), tdaVolume, balance);
    }
  }

  yield();  // Розвантаження стека
  delay(1);
}

// ============================================================================
// Обробка ІЧ пульта RC-5
// ============================================================================

void handleIRRemote(uint8_t keyCode) {
  Serial.printf("[IR] Processing key: 0x%02X\n", keyCode);

  switch (keyCode) {
    case IR_BTN_POWER:
      // Кнопка живлення - вхід/вихід з режиму сну
      if (deviceSleep) {
        deviceWakeUp();
      } else {
        deviceEnterSleep(false);
      }
      break;

    case IR_BTN_VOL_UP:
      // Гучність +
      if (!deviceSleep) {
        uint8_t tdaVolume = tda7318GetVolume();
        if (tdaVolume < 100) {
          tdaVolume++;
          tda7318SetVolume(tdaVolume);
          displayDrawTDAVolumeBar(tdaVolume);
          Serial.printf("[IR] Volume UP: %d\n", tdaVolume);
        }
      }
      break;

    case IR_BTN_VOL_DOWN:
      // Гучність -
      if (!deviceSleep) {
        uint8_t tdaVolume = tda7318GetVolume();
        if (tdaVolume > 0) {
          tdaVolume--;
          tda7318SetVolume(tdaVolume);
          displayDrawTDAVolumeBar(tdaVolume);
          Serial.printf("[IR] Volume DOWN: %d\n", tdaVolume);
        }
      }
      break;

    case IR_BTN_MUTE:
      // Mute/Unmute
      if (!deviceSleep) {
        if (tda7318IsMuted()) {
          tda7318UnMute();
          Serial.println("[IR] Unmuted");
        } else {
          tda7318Mute();
          Serial.println("[IR] Muted");
        }
      }
      break;

    case IR_BTN_CH_UP:
      // Канал + (попередня станція)
      if (!deviceSleep && tda7318GetInput() == INPUT_WIFI_RADIO) {
        int currentIndex = audioPlayerGetCurrentStationIndex();
        int stationCount = audioPlayerGetStationCount();

        if (stationCount > 0 && currentIndex >= 0) {
          int newIndex = (currentIndex > 0) ? currentIndex - 1 : stationCount - 1;

          RadioStation station;
          if (audioPlayerGetStation(newIndex, &station)) {
            uint8_t espVolume = audioPlayerGetVolume();
            uint8_t tdaVolume = tda7318GetVolume();
            TDA7318_Input activeInput = tda7318GetInput();
            displayUpdateStationAndTrack(station.name, "PREV", "", true, espVolume, tdaVolume, activeInput, tda7318GetBalance());
            messageDisplayTime = millis();
            messageDisplayed = true;

            audioPlayerStop();
            delay(100);
            audioPlayerConnect(station.url, station.name, station.volume);
            audioPlayerSetCurrentStationIndex(newIndex);
            audioPlayerSaveCurrentStation();
            strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);

            Serial.printf("[IR] Previous station: %s (index=%d)\n", station.name, newIndex);
          }
        }
      }
      break;

    case IR_BTN_CH_DOWN:
      // Канал - (наступна станція)
      if (!deviceSleep && tda7318GetInput() == INPUT_WIFI_RADIO) {
        int currentIndex = audioPlayerGetCurrentStationIndex();
        int stationCount = audioPlayerGetStationCount();

        if (stationCount > 0 && currentIndex >= 0) {
          int newIndex = (currentIndex < stationCount - 1) ? currentIndex + 1 : 0;

          RadioStation station;
          if (audioPlayerGetStation(newIndex, &station)) {
            uint8_t espVolume = audioPlayerGetVolume();
            uint8_t tdaVolume = tda7318GetVolume();
            TDA7318_Input activeInput = tda7318GetInput();
            displayUpdateStationAndTrack(station.name, "NEXT", "", true, espVolume, tdaVolume, activeInput, tda7318GetBalance());
            messageDisplayTime = millis();
            messageDisplayed = true;

            audioPlayerStop();
            delay(100);
            audioPlayerConnect(station.url, station.name, station.volume);
            audioPlayerSetCurrentStationIndex(newIndex);
            audioPlayerSaveCurrentStation();
            strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);

            Serial.printf("[IR] Next station: %s (index=%d)\n", station.name, newIndex);
          }
        }
      }
      break;

    case IR_BTN_PLAY_PAUSE:
      // Play/Pause
      if (!deviceSleep && tda7318GetInput() == INPUT_WIFI_RADIO) {
        if (audioPlayerGetState() == PLAYER_PLAYING) {
          audioPlayerStop();
          Serial.println("[IR] Stopped");
        } else {
          int lastStationIndex = audioPlayerGetCurrentStationIndex();
          if (lastStationIndex >= 0) {
            RadioStation station;
            if (audioPlayerGetStation(lastStationIndex, &station)) {
              audioPlayerConnect(station.url, station.name, station.volume);
              Serial.printf("[IR] Playing: %s\n", station.name);
              strncpy(lastKnownStation, station.name, sizeof(lastKnownStation) - 1);
            }
          }
        }
      }
      break;

    case IR_BTN_STOP:
      // Stop
      if (!deviceSleep && tda7318GetInput() == INPUT_WIFI_RADIO) {
        audioPlayerStop();
        Serial.println("[IR] Stopped");
      }
      break;

    case IR_BTN_INPUT:
      // Перемикання входу (циклічне)
      if (!deviceSleep) {
        TDA7318_Input currentInput = tda7318GetInput();
        TDA7318_Input newInput = (TDA7318_Input)((currentInput + 1) % 4);
        handleInputChange(newInput);
        Serial.printf("[IR] Input switched to: %d\n", newInput);
      }
      break;

    case IR_BTN_WIFI:
      // Прямий вхід WiFi
      if (!deviceSleep) {
        handleInputChange(INPUT_WIFI_RADIO);
        Serial.println("[IR] Input switched to: WiFi");
      }
      break;

    case IR_BTN_COMPUTER:
      // Прямий вхід Computer
      if (!deviceSleep) {
        handleInputChange(INPUT_COMPUTER);
        Serial.println("[IR] Input switched to: Computer");
      }
      break;

    case IR_BTN_TV_BOX:
      // Прямий вхід TV Box
      if (!deviceSleep) {
        handleInputChange(INPUT_TV_BOX);
        Serial.println("[IR] Input switched to: TV Box");
      }
      break;

    case IR_BTN_AUX:
      // Прямий вхід AUX
      if (!deviceSleep) {
        handleInputChange(INPUT_AUX);
        Serial.println("[IR] Input switched to: AUX");
      }
      break;

    default:
      // Невідома кнопка - ігноруємо
      Serial.printf("[IR] Unknown key: 0x%02X\n", keyCode);
      break;
  }
}
