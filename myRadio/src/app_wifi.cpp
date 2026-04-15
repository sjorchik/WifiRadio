/**
 * @file app_wifi.cpp
 * @brief Реалізація модуля для керування WiFi підключенням ESP32
 */

#include "app_wifi.h"
#include <ArduinoJson.h>
#include "config.h"

// Глобальне посилання на веб-сервер (оголошено в main.cpp)
extern WebServer server;

// Масив ключів для зручного доступу
const char* ssidKeys[MAX_WIFI_NETWORKS] = {WIFI_SSID_KEY_1, WIFI_SSID_KEY_2, WIFI_SSID_KEY_3};
const char* passKeys[MAX_WIFI_NETWORKS] = {WIFI_PASS_KEY_1, WIFI_PASS_KEY_2, WIFI_PASS_KEY_3};

// Об'єкт для збереження даних в постійну пам'ять (NVS)
Preferences preferences;

/**
 * @brief Ініціалізація WiFi модуля
 */
bool appWifiInit() {
  Serial.println(F("[WiFi] Ініціалізація WiFi модуля..."));
  
  // Намагаємось підключитися до збережених мереж
  if (appWifiConnect()) {
    return true;  // Підключення успішне
  }

  // Якщо не вдалося - вмикаємо режим точки доступу
  Serial.println(F("[WiFi] Не вдалося підключитися. Вмикаю AP mode..."));
  appWifiSetupAP();
  return false;
}

/**
 * @brief Підключення до WiFi мережі
 *
 * Намагається підключитися до всіх збережених мереж по черзі.
 * Оптимізовано для стабільного потоку аудіо.
 */
bool appWifiConnect(const char* ssid, const char* password) {
  WifiNetwork networks[MAX_WIFI_NETWORKS];
  size_t count = 0;

  // Якщо SSID та пароль вказані - використовуємо їх
  if (ssid != nullptr && password != nullptr) {
    networks[0].ssid = String(ssid);
    networks[0].password = String(password);
    count = 1;
  } else {
    // Читаємо збережені мережі
    if (!appWifiLoadCredentials(networks, count)) {
      Serial.println(F("[WiFi] Немає збережених налаштувань"));
      return false;
    }
  }

  // Оптимізація WiFi для стабільного потоку
  WiFi.setSleep(false);  // Вимикаємо WiFi sleep mode для стабільності
  Serial.println(F("[WiFi] WiFi sleep mode disabled for streaming stability"));

  // Намагаємось підключитися до кожної мережі
  for (size_t i = 0; i < count; i++) {
    if (networks[i].ssid.length() == 0) continue;  // Пропускаємо пусті

    Serial.print(F("[WiFi] Спроба підключення #"));
    Serial.print(i + 1);
    Serial.print(F(" до мережі: "));
    Serial.println(networks[i].ssid);

    // Ініціюємо підключення
    WiFi.begin(networks[i].ssid.c_str(), networks[i].password.c_str());

    // Лічильник спроб підключення
    int attempts = 0;

    // Цикл очікування підключення
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS) {
      delay(WIFI_RETRY_DELAY_MS);
      Serial.print(F("."));
      attempts++;
    }

    // Перевіряємо результат
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println(F("[WiFi] WiFi connected!"));
      Serial.print(F("[WiFi] IP address: "));
      Serial.println(WiFi.localIP());
      
      // Перевіряємо рівень сигналу
      int32_t rssi = WiFi.RSSI();
      Serial.print(F("[WiFi] RSSI (signal strength): "));
      Serial.print(rssi);
      Serial.println(F(" dBm"));
      
      if (rssi < WIFI_MIN_RSSI_THRESHOLD) {
        Serial.println(F("[WiFi] WARNING: Weak signal! Audio dropouts may occur."));
        Serial.println(F("[WiFi] Consider moving closer to the router."));
      } else if (rssi < -70) {
        Serial.println(F("[WiFi] Signal quality: Moderate"));
      } else {
        Serial.println(F("[WiFi] Signal quality: Good"));
      }
      
      return true;
    }

    Serial.println(F(" [не вдалося]"));
    WiFi.disconnect();  // Відключаємось для наступної спроби
  }

  Serial.println(F("[WiFi] Всі спроби підключення не вдалися"));
  return false;
}

/**
 * @brief Налаштування точки доступу (AP mode)
 */
void appWifiSetupAP() {
  // Запускаємо режим точки доступу з вказаними обліковими даними
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  // Отримуємо IP-адресу створеної точки доступу
  IPAddress IP = WiFi.softAPIP();
  Serial.print(F("[WiFi] AP IP address: "));
  Serial.println(IP);
  
  // Запускаємо mDNS службу для доступу до пристрою за іменем "esp32.local"
  if (MDNS.begin("esp32")) {
    Serial.println(F("[WiFi] mDNS responder started"));
  } else {
    Serial.println(F("[WiFi] mDNS failed to start"));
  }
}

/**
 * @brief Збереження WiFi налаштувань в NVS пам'ять
 */
bool appWifiSaveCredentials(const WifiNetwork* networks, size_t count) {
  Serial.println(F("[WiFi] Saving WiFi credentials..."));
  
  // Відкриваємо простір "wifi" в режимі запису (false)
  if (!preferences.begin("wifi", false)) {
    Serial.println(F("[WiFi] Error opening preferences"));
    return false;
  }
  
  // Спочатку видаляємо всі старі записи
  for (size_t i = 0; i < MAX_WIFI_NETWORKS; i++) {
    preferences.remove(ssidKeys[i]);
    preferences.remove(passKeys[i]);
  }
  
  // Зберігаємо нові дані
  for (size_t i = 0; i < MAX_WIFI_NETWORKS && i < count; i++) {
    if (networks[i].ssid.length() > 0) {
      preferences.putString(ssidKeys[i], networks[i].ssid);
      preferences.putString(passKeys[i], networks[i].password);
      Serial.print(F("[WiFi] Saved network #"));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.println(networks[i].ssid);
    }
  }
  
  // Закриваємо простір Preferences
  preferences.end();
  
  Serial.println(F("[WiFi] Credentials saved successfully"));
  return true;
}

/**
 * @brief Читання збережених WiFi налаштувань з NVS пам'яті
 */
bool appWifiLoadCredentials(WifiNetwork* networks, size_t& count) {
  // Відкриваємо простір "wifi" в режимі читання (true)
  if (!preferences.begin("wifi", true)) {
    Serial.println(F("[WiFi] Error opening preferences"));
    return false;
  }

  count = 0;

  // Читаємо до 3-х мереж
  for (size_t i = 0; i < MAX_WIFI_NETWORKS; i++) {
    networks[i].ssid = preferences.getString(ssidKeys[i], "");
    networks[i].password = preferences.getString(passKeys[i], "");

    if (networks[i].ssid.length() > 0) {
      count++;
      Serial.print(F("[WiFi] Loaded network #"));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.println(networks[i].ssid);
    }
  }

  // Закриваємо простір Preferences
  preferences.end();

  if (count > 0) {
    Serial.println(F("[WiFi] Credentials loaded successfully"));
    return true;
  }

  Serial.println(F("[WiFi] No saved credentials found"));
  return false;
}

/**
 * @brief Видалення збережених WiFi налаштувань
 */
void appWifiClearCredentials() {
  Serial.println(F("[WiFi] Clearing credentials..."));
  
  // Відкриваємо простір "wifi" в режимі запису
  if (preferences.begin("wifi", false)) {
    preferences.clear();
    preferences.end();
    Serial.println(F("[WiFi] Credentials cleared"));
  } else {
    Serial.println(F("[WiFi] Error clearing credentials"));
  }
}

/**
 * @brief Перевірка статусу WiFi підключення
 */
bool appWifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Отримати рівень сигналу WiFi (RSSI)
 * @return RSSI в dBm або 0 якщо не підключено
 */
int32_t appWifiGetRSSI() {
  if (appWifiIsConnected()) {
    return WiFi.RSSI();
  }
  return 0;
}

/**
 * @brief Перевірити якість WiFi з'єднання
 * @return true якщо якість достатня для потоку
 */
bool appWifiCheckConnectionQuality() {
  if (!appWifiIsConnected()) {
    return false;
  }
  
  int32_t rssi = WiFi.RSSI();
  
  // Перевіряємо рівень сигналу
  if (rssi < WIFI_MIN_RSSI_THRESHOLD) {
    Serial.print(F("[WiFi] WARNING: Poor signal quality: "));
    Serial.print(rssi);
    Serial.println(F(" dBm"));
    return false;
  }
  
  return true;
}

/**
 * @brief Отримання IP-адреси пристрою
 */
String appWifiGetIPAddress() {
  if (appWifiIsConnected()) {
    return WiFi.localIP().toString();
  } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    return WiFi.softAPIP().toString();
  }
  return "";
}

/**
 * @brief Перезавантаження ESP32 модуля
 */
void appWifiRestart() {
  Serial.println(F("[WiFi] Restarting ESP32..."));
  delay(1000);  // Затримка для завершення всіх операцій
  ESP.restart();
}

/**
 * @brief Обробник HTTP запиту статусу (/status)
 */
void appWifiHandleStatus() {
  // Створюємо JSON документ розміром 1024 байти
  DynamicJsonDocument doc(1024);

  // Додаємо статус підключення до WiFi
  doc["connected"] = appWifiIsConnected();

  // Додаємо час роботи в секундах
  doc["uptime"] = millis() / 1000;

  // Додаємо вільну пам'ять heap
  doc["freeheap"] = ESP.getFreeHeap();

  // Серіалізуємо та відправляємо JSON
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Обробник HTTP запиту завантаження налаштувань WiFi (/settings/wifi/load)
 */
void appWifiHandleLoadSettings() {
  WifiNetwork networks[MAX_WIFI_NETWORKS];
  size_t count = 0;

  // Завантажуємо збережені мережі
  bool loaded = appWifiLoadCredentials(networks, count);

  Serial.print(F("[WiFi] Load settings - loaded="));
  Serial.print(loaded);
  Serial.print(F(", count="));
  Serial.println(count);

  // Створюємо JSON документ
  DynamicJsonDocument doc(512);

  // Додаємо мережі до JSON використовуючи const char ключі
  for (size_t i = 0; i < MAX_WIFI_NETWORKS; i++) {
    if (i < MAX_WIFI_NETWORKS && networks[i].ssid.length() > 0) {
      doc[ssidKeys[i]] = networks[i].ssid.c_str();
      doc[passKeys[i]] = networks[i].password.c_str();
      Serial.print(F("[WiFi] Network #"));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.println(networks[i].ssid);
    } else {
      doc[ssidKeys[i]] = "";
      doc[passKeys[i]] = "";
    }
  }

  // Відправляємо JSON відповідь
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.print(F("[WiFi] Sending JSON: "));
  Serial.println(jsonString);
  server.send(200, "application/json", jsonString);
}

/**
 * @brief Обробник HTTP запиту збереження налаштувань (/settings/wifi)
 */
void appWifiHandleSettings() {
  // Перевіряємо метод запиту
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  // Отримуємо тіло запиту
  String requestBody = server.arg("plain");

  // Парсимо JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, requestBody);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Масив для збереження мереж
  WifiNetwork networks[MAX_WIFI_NETWORKS];
  size_t count = 0;

  // Витягуємо дані для 3-х мереж використовуючи const char ключі
  for (size_t i = 0; i < MAX_WIFI_NETWORKS; i++) {
    networks[i].ssid = doc[ssidKeys[i]] | "";
    networks[i].password = doc[passKeys[i]] | "";

    // Рахуємо тільки мережі з заповненим SSID
    if (networks[i].ssid.length() > 0) {
      count++;
    }
  }

  // Перевірка - хоч одна мережа має бути
  if (count == 0) {
    server.send(400, "application/json", "{\"error\":\"At least one SSID is required\"}");
    return;
  }

  // Зберігаємо налаштування в NVS
  if (!appWifiSaveCredentials(networks, count)) {
    server.send(500, "application/json", "{\"error\":\"Failed to save\"}");
    return;
  }

  // Створюємо відповідь
  DynamicJsonDocument response(256);
  response["status"] = "settings_saved";
  response["count"] = count;
  response["message"] = "Device will restart to apply new settings";

  // Відправляємо відповідь
  String jsonString;
  serializeJson(response, jsonString);
  server.send(200, "application/json", jsonString);

  // Затримка для відправки відповіді
  delay(1000);

  // Перезавантажуємо модуль
  appWifiRestart();
}
