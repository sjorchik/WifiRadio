/**
 * @file web_handlers.cpp
 * @brief Обробники HTTP запитів веб-сервера
 */

#include "web_handlers.h"
#include "app_wifi.h"
#include "audio_player.h"
#include "tda7318.h"
#include "display.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Глобальне посилання на сервер
extern WebServer server;

// Глобальна змінна з main.cpp для перевірки стану налаштувань звуку
extern bool soundSettingsOpen;

// Змінні для відстеження змін на дисплеї
static char lastStationName[64] = "";
static char lastArtist[64] = "";
static char lastTitle[64] = "";
static uint8_t lastVolume = 255;  // 255 = ніколи не співпадає
static bool lastPlayingState = false;
static TDA7318_Input lastInput = INPUT_WIFI_RADIO;

/**
 * @brief Відправка HTML файлу клієнту
 */
void sendHTML() {
  if (SPIFFS.exists("/index.html")) {
    File file = SPIFFS.open("/index.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

/**
 * @brief Відправка сторінки налаштувань WiFi (для AP режиму)
 */
void sendAPSetup() {
  if (SPIFFS.exists("/ap.html")) {
    File file = SPIFFS.open("/ap.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

/**
 * @brief Відправка сторінки налаштувань WiFi (/settings)
 */
void sendSettings() {
  if (SPIFFS.exists("/settings.html")) {
    File file = SPIFFS.open("/settings.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

/**
 * @brief Відправка сторінки радіостанцій (/stations)
 */
void sendStations() {
  if (SPIFFS.exists("/stations.html")) {
    File file = SPIFFS.open("/stations.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

/**
 * @brief Обробник для кореневого URL (/)
 */
void handleRoot() {
  // Якщо не підключено до WiFi - показуємо сторінку налаштувань
  if (!appWifiIsConnected()) {
    sendAPSetup();
    return;
  }
  sendHTML();
}

/**
 * @brief Обробник для сторінки налаштувань (/setup)
 */
void handleAPSetup() {
  sendAPSetup();
}

/**
 * @brief Обробник для сторінки налаштувань WiFi (/settings)
 */
void handleSettings() {
  sendSettings();
}

/**
 * @brief Обробник для сторінки радіостанцій (/stations)
 */
void handleStations() {
  sendStations();
}

/**
 * @brief Обробник для отримання IP адреси (/ipaddr)
 */
void handleIpAddr() {
  String ip = "";
  if (appWifiIsConnected()) {
    ip = WiFi.localIP().toString();
  } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    ip = WiFi.softAPIP().toString();
  }
  server.send(200, "text/plain", ip);
}

/**
 * @brief Обробник статусу аудіо плеєра (/audio/status)
 */
void handleAudioStatus() {
  // Закрити список станцій якщо відкритий
  closeStationList();
  
  DynamicJsonDocument doc(512);
  doc["state"] = audioPlayerGetState();

  // Отримуємо назву станції
  const char* stationName = audioPlayerGetStationName();
  if (stationName && strlen(stationName) > 0) {
    doc["station"] = stationName;
  } else {
    doc["station"] = "-";
  }

  doc["volume"] = audioPlayerGetVolume();
  doc["stationIndex"] = audioPlayerGetCurrentStationIndex();

  // Додаємо інформацію про трек
  const char* artist = audioPlayerGetArtist();
  const char* title = audioPlayerGetTitle();
  if (artist && strlen(artist) > 0) {
    doc["artist"] = artist;
  }
  if (title && strlen(title) > 0) {
    doc["title"] = title;
  }

  // Оновлення дисплея якщо змінилися дані
  bool needUpdate = false;
  
  // Отримуємо поточний стан
  PlayerState currentState = audioPlayerGetState();
  bool isPlaying = (currentState == PLAYER_PLAYING);
  uint8_t currentVolume = audioPlayerGetVolume();
  TDA7318_Input currentInput = tda7318GetInput();
  
  // Порівнюємо з останніми збереженими значеннями
  bool stationChanged = strcmp(lastStationName, stationName) != 0;
  bool artistChanged = strcmp(lastArtist, artist) != 0;
  bool titleChanged = strcmp(lastTitle, title) != 0;
  bool playingChanged = isPlaying != lastPlayingState;
  bool volumeChanged = currentVolume != lastVolume;
  bool inputChanged = currentInput != lastInput;

  if (stationChanged || artistChanged || titleChanged || volumeChanged || playingChanged || inputChanged) {
    needUpdate = true;

    // Зберігаємо назву станції навіть якщо вона порожня (радіо зупинилось)
    if (stationName && strlen(stationName) > 0) {
      strncpy(lastStationName, stationName, sizeof(lastStationName) - 1);
    }
    strncpy(lastArtist, artist ? artist : "", sizeof(lastArtist) - 1);
    strncpy(lastTitle, title ? title : "", sizeof(lastTitle) - 1);
    lastVolume = currentVolume;
    lastPlayingState = isPlaying;
    lastInput = currentInput;
  }

  // Оновлюємо дисплей тільки якщо є зміни І радіо грає (або змінився стан) І не відкриті налаштування звуку
  if (needUpdate && (isPlaying || playingChanged || inputChanged) && !soundSettingsOpen) {
    // Оновлюємо всю центральну зону з станцією і треком
    // Використовуємо lastStationName щоб зберегти назву навіть коли радіо зупинилось
    uint8_t tdaVolume = tda7318GetVolume();
    // Завжди показуємо lastStationName навіть коли радіо зупинено
    displayUpdateStationAndTrack(lastStationName, lastPlayingState ? lastArtist : "", lastPlayingState ? lastTitle : "",
                                 lastPlayingState, lastVolume, tdaVolume, lastInput, tda7318GetBalance());
  }

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Обробник відтворення радіо (/audio/play)
 */
void handleAudioPlay() {
  String url = server.arg("url");
  
  if (url.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"URL required\"}");
    return;
  }
  
  // Для ручного URL передаємо тільки URL без назви
  if (audioPlayerConnect(url.c_str(), nullptr)) {
    server.send(200, "application/json", "{\"status\":\"playing\"}");
  } else {
    server.send(500, "application/json", "{\"status\":\"error\"}");
  }
}

/**
 * @brief Обробник зупинки (/audio/stop)
 */
void handleAudioStop() {
  audioPlayerStop();
  server.send(200, "application/json", "{\"status\":\"stopped\"}");
}

/**
 * @brief Обробник гучності (/audio/volume)
 */
void handleAudioVolume() {
  String vol = server.arg("value");
  uint8_t volume = vol.toInt();

  audioPlayerSetVolume(volume);
  
  // Оновити індикатор гучності на дисплеї
  uint8_t tdaVolume = tda7318GetVolume();
  displayUpdateVolumeBars(volume, tdaVolume, tda7318GetBalance());
  
  server.send(200, "application/json", "{\"status\":\"ok\",\"volume\":volume}");
}

/**
 * @brief Обробник списку станцій (/audio/stations)
 */
void handleAudioStations() {
  // Отримати список станцій
  DynamicJsonDocument doc(2048);
  JsonArray stations = doc.to<JsonArray>();
  
  int count = audioPlayerGetStationCount();
  for (int i = 0; i < count; i++) {
    RadioStation station;
    if (audioPlayerGetStation(i, &station)) {
      JsonObject obj = stations.createNestedObject();
      obj["index"] = i;
      obj["name"] = station.name;
      obj["url"] = station.url;
      obj["volume"] = station.volume;
    }
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Обробник додавання станції (/audio/stations_add)
 */
void handleAudioStationsAdd() {
  String requestBody = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, requestBody);
  
  String name = doc["name"] | "";
  String url = doc["url"] | "";
  uint8_t volume = doc["volume"] | 20;
  
  Serial.printf("[Web] Add station: %s - %s (vol=%d)\n", name.c_str(), url.c_str(), volume);
  
  if (name.length() == 0 || url.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Name and URL required\"}");
    return;
  }
  
  if (audioPlayerAddStation(name.c_str(), url.c_str(), volume)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(500, "application/json", "{\"error\":\"Failed to add\"}");
  }
}

/**
 * @brief Обробник видалення станції (/audio/stations_del)
 */
void handleAudioStationsDel() {
  String requestBody = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, requestBody);
  
  int index = doc["index"] | -1;
  
  Serial.printf("[Web] Delete station index: %d\n", index);
  
  if (index >= 0 && audioPlayerRemoveStation(index)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid index\"}");
  }
}

/**
 * @brief Обробник переміщення станції (/audio/stations_move)
 */
void handleAudioStationsMove() {
  String requestBody = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, requestBody);
  
  int index = doc["index"] | -1;
  int direction = doc["direction"] | 0;
  
  Serial.printf("[Web] Move station: index=%d, direction=%d\n", index, direction);
  
  if (index < 0 || audioPlayerMoveStation(index, direction)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid index\"}");
  }
}

/**
 * @brief Обробник оновлення гучності станції (/audio/stations_volume)
 */
void handleAudioStationsVolume() {
  String requestBody = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, requestBody);
  
  int index = doc["index"] | -1;
  uint8_t volume = doc["volume"] | 20;
  
  Serial.printf("[Web] Update station volume: index=%d, volume=%d\n", index, volume);
  
  if (index >= 0 && audioPlayerUpdateStationVolume(index, volume)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid index\"}");
  }
}

/**
 * @brief Обробник оновлення станції (/audio/stations_update)
 */
void handleAudioStationsUpdate() {
  String requestBody = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, requestBody);
  
  int index = doc["index"] | -1;
  String name = doc["name"] | "";
  String url = doc["url"] | "";
  uint8_t volume = doc["volume"] | 20;
  
  Serial.printf("[Web] Update station: index=%d, %s - %s (vol=%d)\n", index, name.c_str(), url.c_str(), volume);
  
  if (index < 0 || name.length() == 0 || url.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Invalid data\"}");
    return;
  }
  
  if (audioPlayerUpdateStation(index, name.c_str(), url.c_str(), volume)) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(500, "application/json", "{\"error\":\"Failed to update\"}");
  }
}

/**
 * @brief Обробник відтворення станції (/audio/playstation)
 */
void handleAudioPlayStation() {
  // Закрити список станцій якщо відкритий
  closeStationList();
  
  int index = server.arg("index").toInt();

  RadioStation station;
  if (audioPlayerGetStation(index, &station)) {
    // Передаємо назву станції та її гучність
    if (audioPlayerConnect(station.url, station.name, station.volume)) {
      // Зберігаємо індекс станції в енергонезалежну пам'ять
      audioPlayerSetCurrentStationIndex(index);
      audioPlayerSaveCurrentStation();

      server.send(200, "application/json", "{\"status\":\"playing\",\"name\":\"" + String(station.name) + "\",\"volume\":" + String(station.volume) + ",\"index\":" + String(index) + "}");
    } else {
      server.send(500, "application/json", "{\"status\":\"error\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid index\"}");
  }
}

// ============================================================================
// Обробники TDA7318
// ============================================================================

/**
 * @brief Відправка сторінки налаштувань звуку (/sound)
 */
void handleSound() {
  Serial.println(F("[Web] handleSound() called"));
  Serial.printf("[Web] SPIFFS exists /sound.html: %d\n", SPIFFS.exists("/sound.html"));
  
  if (SPIFFS.exists("/sound.html")) {
    File file = SPIFFS.open("/sound.html", "r");
    if (file) {
      Serial.printf("[Web] Streaming file, size: %d\n", file.size());
      server.streamFile(file, "text/html");
      file.close();
    } else {
      Serial.println(F("[Web] Failed to open file"));
      server.send(500, "text/plain", "Failed to open file");
    }
  } else {
    Serial.println(F("[Web] File not found in SPIFFS"));
    server.send(404, "text/plain", "File not found");
  }
}

/**
 * @brief Отримати налаштування звуку (/sound/settings)
 */
void handleSoundSettings() {
  DynamicJsonDocument doc(256);
  doc["volume"] = tda7318GetVolume();
  doc["bass"] = tda7318GetBass();
  doc["treble"] = tda7318GetTreble();
  doc["balance"] = tda7318GetBalance();
  doc["input"] = tda7318GetInput();
  doc["muted"] = tda7318IsMuted();

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Встановити гучність (/sound/volume)
 */
void handleSoundVolume() {
  String value = server.arg("value");
  uint8_t volume = value.toInt();

  if (volume > 100) volume = 100;
  tda7318SetVolume(volume);

  // Оновити індикатор гучності на дисплеї
  uint8_t espVolume = audioPlayerGetVolume();
  int8_t balance = tda7318GetBalance();
  displayUpdateVolumeBars(espVolume, volume, balance);

  DynamicJsonDocument doc(128);
  doc["status"] = "ok";
  doc["volume"] = volume;

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Встановити бас (/sound/bass)
 */
void handleSoundBass() {
  String value = server.arg("value");
  int8_t bass = value.toInt();

  if (bass < -7) bass = -7;
  if (bass > 7) bass = 7;
  tda7318SetBass(bass);

  // Оновити індикатор басу на дисплеї
  if (soundSettingsOpen) {
    displayDrawToneBarVertical(bass, 81, COLOR_LIGHT_GREEN, true);  // BASS_BAR_X = 81
  }

  DynamicJsonDocument doc(128);
  doc["status"] = "ok";
  doc["bass"] = bass;

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Встановити високі (/sound/treble)
 */
void handleSoundTreble() {
  String value = server.arg("value");
  int8_t treble = value.toInt();

  if (treble < -7) treble = -7;
  if (treble > 7) treble = 7;
  tda7318SetTreble(treble);

  // Оновити індикатор тембру на дисплеї
  if (soundSettingsOpen) {
    displayDrawToneBarVertical(treble, 177, COLOR_LIGHT_GREEN, true);  // TREBLE_BAR_X = 177
  }

  DynamicJsonDocument doc(128);
  doc["status"] = "ok";
  doc["treble"] = treble;

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Встановити баланс (/sound/balance)
 */
void handleSoundBalance() {
  String value = server.arg("value");
  int8_t balance = value.toInt();

  if (balance < -7) balance = -7;
  if (balance > 7) balance = 7;
  tda7318SetBalance(balance);

  // Оновити індикатор балансу на дисплеї
  displayDrawBalanceBar(balance);

  DynamicJsonDocument doc(128);
  doc["status"] = "ok";
  doc["balance"] = balance;

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Перемкнути вхід (/sound/input)
 */
void handleSoundInput() {
  String value = server.arg("value");
  uint8_t input = value.toInt();

  if (input > 3) input = 3;
  tda7318SetInput((TDA7318_Input)input);

  // Оновити тільки панель входів на дисплеї
  displayUpdateAudioInputBar((TDA7318_Input)input);

  // Оновити центральну зону дисплея
  {
    uint8_t tdaVolume = tda7318GetVolume();
    int8_t balance = tda7318GetBalance();
    const char* displayStation = audioPlayerGetStationName();
    displayUpdateStationAndTrack(displayStation, "", "", true, audioPlayerGetVolume(), tdaVolume, (TDA7318_Input)input, balance);
  }

  // Оновити центральну зону дисплея
  {
    uint8_t tdaVolume = tda7318GetVolume();
    int8_t balance = tda7318GetBalance();
    const char* displayStation = audioPlayerGetStationName();
    displayUpdateStationAndTrack(displayStation, "", "", true, audioPlayerGetVolume(), tdaVolume, (TDA7318_Input)input, balance);
  }

  const char* inputNames[] = {"WiFi Радіо", "Computer", "TV Box", "AUX"};

  DynamicJsonDocument doc(128);
  doc["status"] = "ok";
  doc["input"] = input;
  doc["inputName"] = inputNames[input];

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Увімкнути/вимкнути Mute (/sound/mute)
 */
void handleSoundMute() {
  String value = server.arg("value");
  bool muted = value.toInt() != 0;

  tda7318SetMute(muted);

  DynamicJsonDocument doc(128);
  doc["status"] = "ok";
  doc["muted"] = muted;

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Скинути налаштування до заводських (/sound/reset)
 */
void handleSoundReset() {
  tda7318SetVolume(50);
  tda7318SetBass(0);
  tda7318SetTreble(0);
  tda7318SetBalance(0);
  tda7318SetInput(INPUT_WIFI_RADIO);
  tda7318UnMute();

  DynamicJsonDocument doc(128);
  doc["status"] = "ok";

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}
