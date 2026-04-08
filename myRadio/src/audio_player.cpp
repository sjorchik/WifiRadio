/**
 * @file audio_player.cpp
 * @brief Модуль для відтворення інтернет-радіо через I2S
 * Використовує ESP32-audioI2S library v2.3.0
 */

#include "audio_player.h"
#include "display.h"
#include "tda7318.h"

// Глобальна змінна з main.cpp для перевірки стану налаштувань звуку
extern bool soundSettingsOpen;

// Глобальний об'єкт Audio
Audio audio;

// Preferences для збереження станцій
Preferences audioPreferences;

static PlayerState playerState = PLAYER_STOPPED;
static char currentStation[64] = "";  // Назва станції, не URL!
static char currentUrl[128] = "";     // URL станції
static char currentArtist[64] = "";   // Виконавець
static char currentTitle[64] = "";    // Назва композиції
static uint8_t currentVolume = 80;    // Поточна гучність
static int currentStationIndex = -1;  // Індекс поточної станції

// Для відстеження змін треку
static char lastDisplayedArtist[64] = "";
static char lastDisplayedTitle[64] = "";

// Callbacks
void audio_info(const char *info) {
    Serial.print("[Audio] info: ");
    Serial.println(info);

    // Парсинг StreamTitle='Artist - Title'
    const char* streamTitle = strstr(info, "StreamTitle='");
    if (streamTitle) {
        streamTitle += 13;  // Пропускаємо "StreamTitle='"
        const char* endQuote = strchr(streamTitle, '\'');
        if (endQuote) {
            static char title[128];  // Static для зменшення використання стека
            int len = min((int)(endQuote - streamTitle), 127);
            
            // Якщо StreamTitle пустий (len=0) - очищуємо artist/title
            if (len == 0) {
                currentArtist[0] = '\0';
                currentTitle[0] = '\0';
                lastDisplayedArtist[0] = '\0';
                lastDisplayedTitle[0] = '\0';
                Serial.println("[Audio] StreamTitle empty - cleared");
            } else if (len > 0) {
                strncpy(title, streamTitle, len);
                title[len] = '\0';

                // Шукаємо розділювач " - "
                const char* separator = strstr(title, " - ");
                if (separator) {
                    // Розділяємо на виконавця і назву
                    int artistLen = min((int)(separator - title), 63);
                    if (artistLen > 0) {
                        strncpy(currentArtist, title, artistLen);
                        currentArtist[artistLen] = '\0';
                    }

                    strncpy(currentTitle, separator + 3, 63);
                    currentTitle[63] = '\0';

                    Serial.printf("[Audio] Artist: %s, Title: %s\n", currentArtist, currentTitle);

                    // Оновлюємо дисплей якщо трек змінився
                    if (strcmp(currentArtist, lastDisplayedArtist) != 0 ||
                        strcmp(currentTitle, lastDisplayedTitle) != 0) {
                        // Зберігаємо поточний трек
                        strncpy(lastDisplayedArtist, currentArtist, sizeof(lastDisplayedArtist) - 1);
                        strncpy(lastDisplayedTitle, currentTitle, sizeof(lastDisplayedTitle) - 1);

                        // Оновлюємо дисплей (тільки якщо WiFi вхід активний і не відкриті налаштування звуку)
                        TDA7318_Input activeInput = tda7318GetInput();
                        if (activeInput == INPUT_WIFI_RADIO && !soundSettingsOpen) {
                            uint8_t espVolume = audioPlayerGetVolume();
                            uint8_t tdaVolume = tda7318GetVolume();
                            displayUpdateStationAndTrack(currentStation, currentArtist, currentTitle, true, espVolume, tdaVolume, activeInput, tda7318GetBalance());
                        }
                    }
                } else {
                    strncpy(currentTitle, title, 63);
                    currentTitle[63] = '\0';
                    currentArtist[0] = '\0';
                }
            }
        }
    }
}

bool audioPlayerInit() {
    Serial.println(F("[Audio] Initializing..."));
    
    // Ініціалізація Audio бібліотеки
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(currentVolume);
    
    Serial.println(F("[Audio] Initialized"));
    return true;
}

bool audioPlayerConnect(const char* url, const char* name, uint8_t volume) {
    if (playerState == PLAYER_PLAYING) {
        audioPlayerStop();
    }

    Serial.printf("[Audio] Connecting to: %s\n", url);
    playerState = PLAYER_CONNECTING;

    // Зберігаємо назву станції (або URL якщо назва не вказана)
    if (name && strlen(name) > 0) {
        strncpy(currentStation, name, sizeof(currentStation) - 1);
    } else {
        strncpy(currentStation, url, sizeof(currentStation) - 1);
    }
    strncpy(currentUrl, url, sizeof(currentUrl) - 1);

    // Скидаємо відстеження треку для нової станції
    lastDisplayedArtist[0] = '\0';
    lastDisplayedTitle[0] = '\0';

    // Встановлюємо гучність для цієї станції
    currentVolume = volume;
    audio.setVolume(currentVolume);

    // Підключення до потоку
    if (audio.connecttohost(url)) {
        Serial.println("[Audio] Connected!");
        playerState = PLAYER_PLAYING;
        return true;
    } else {
        Serial.println("[Audio] Connection failed!");
        playerState = PLAYER_ERROR;
        return false;
    }
}

int audioPlayerLoop() {
    if (playerState == PLAYER_PLAYING) {
        audio.loop();
        
        if (!audio.isRunning()) {
            Serial.println("[Audio] Stream stopped");
            playerState = PLAYER_STOPPED;
        }
    }
    return 0;
}

void audioPlayerStop() {
    Serial.println(F("[Audio] Stopping"));
    audio.stopSong();
    playerState = PLAYER_STOPPED;
    currentStation[0] = '\0';
}

PlayerState audioPlayerGetState() {
    return playerState;
}

const char* audioPlayerGetStationName() {
    return currentStation;
}

void audioPlayerSetVolume(uint8_t vol) {
    if (vol > 100) vol = 100;
    currentVolume = vol;
    audio.setVolume(currentVolume);
    Serial.printf("[Audio] Volume set to: %d\n", currentVolume);
}

uint8_t audioPlayerGetVolume() {
    return currentVolume;
}

void audioPlayerEnd() {
    audioPlayerStop();
    Serial.println(F("[Audio] Ended"));
}

// Отримати поточного виконавця
const char* audioPlayerGetArtist() {
    return currentArtist;
}

// Отримати поточну назву композиції
const char* audioPlayerGetTitle() {
    return currentTitle;
}

// ============================================================================
// Функції для роботи з радіостанціями
// ============================================================================

/**
 * @brief Додати радіостанцію
 */
bool audioPlayerAddStation(const char* name, const char* url, uint8_t volume) {
    if (!name || !url) return false;
    
    // Отримуємо поточну кількість станцій
    int count = audioPlayerGetStationCount();
    if (count >= MAX_STATIONS) {
        Serial.printf("[Audio] Max stations reached (%d)\n", MAX_STATIONS);
        return false;
    }
    
    // Відкриваємо Preferences
    if (!audioPreferences.begin("stations", false)) {
        Serial.println("[Audio] Error opening preferences");
        return false;
    }
    
    // Зберігаємо станцію
    String keyName = "name_" + String(count);
    String keyUrl = "url_" + String(count);
    String keyVol = "vol_" + String(count);
    
    audioPreferences.putString(keyName.c_str(), name);
    audioPreferences.putString(keyUrl.c_str(), url);
    audioPreferences.putUChar(keyVol.c_str(), volume);
    audioPreferences.putInt("count", count + 1);
    
    audioPreferences.end();
    
    Serial.printf("[Audio] Station added: %s (vol=%d)\n", name, volume);
    return true;
}

/**
 * @brief Видалити радіостанцію
 */
bool audioPlayerRemoveStation(int index) {
    if (index < 0 || index >= audioPlayerGetStationCount()) {
        return false;
    }
    
    if (!audioPreferences.begin("stations", false)) {
        return false;
    }
    
    // Видаляємо станцію
    String keyName = "name_" + String(index);
    String keyUrl = "url_" + String(index);
    
    audioPreferences.remove(keyName.c_str());
    audioPreferences.remove(keyUrl.c_str());
    
    // Зсуваємо решту станцій
    int count = audioPreferences.getInt("count", 0);
    for (int i = index; i < count - 1; i++) {
        String nextName = "name_" + String(i + 1);
        String nextUrl = "url_" + String(i + 1);
        
        String name = audioPreferences.getString(nextName.c_str(), "");
        String url = audioPreferences.getString(nextUrl.c_str(), "");
        
        if (name.length() > 0) {
            String currName = "name_" + String(i);
            String currUrl = "url_" + String(i);
            audioPreferences.putString(currName.c_str(), name);
            audioPreferences.putString(currUrl.c_str(), url);
        }
        
        audioPreferences.remove(nextName.c_str());
        audioPreferences.remove(nextUrl.c_str());
    }
    
    // Оновлюємо кількість
    audioPreferences.putInt("count", count - 1);
    audioPreferences.end();
    
    return true;
}

/**
 * @brief Отримати радіостанцію за індексом
 */
bool audioPlayerGetStation(int index, RadioStation* station) {
    if (!station || index < 0 || index >= audioPlayerGetStationCount()) {
        return false;
    }
    
    if (!audioPreferences.begin("stations", true)) {
        return false;
    }
    
    String keyName = "name_" + String(index);
    String keyUrl = "url_" + String(index);
    String keyVol = "vol_" + String(index);
    
    String name = audioPreferences.getString(keyName.c_str(), "");
    String url = audioPreferences.getString(keyUrl.c_str(), "");
    uint8_t volume = audioPreferences.getUChar(keyVol.c_str(), 80);
    
    audioPreferences.end();
    
    if (name.length() == 0 || url.length() == 0) {
        return false;
    }
    
    strncpy(station->name, name.c_str(), sizeof(station->name) - 1);
    strncpy(station->url, url.c_str(), sizeof(station->url) - 1);
    station->volume = volume;
    
    return true;
}

/**
 * @brief Отримати кількість радіостанцій
 */
int audioPlayerGetStationCount() {
    if (!audioPreferences.begin("stations", true)) {
        return 0;
    }
    
    int count = audioPreferences.getInt("count", 0);
    audioPreferences.end();
    
    return count;
}

/**
 * @brief Очистити всі радіостанції
 */
void audioPlayerClearStations() {
    if (!audioPreferences.begin("stations", false)) {
        return;
    }
    
    int count = audioPreferences.getInt("count", 0);
    for (int i = 0; i < count; i++) {
        String keyName = "name_" + String(i);
        String keyUrl = "url_" + String(i);
        audioPreferences.remove(keyName.c_str());
        audioPreferences.remove(keyUrl.c_str());
    }
    
    audioPreferences.putInt("count", 0);
    audioPreferences.end();
    
    Serial.println("[Audio] All stations cleared");
}

/**
 * @brief Отримати назву станції за індексом
 */
const char* audioPlayerGetStationNameByIndex(int index) {
    static char name[32];
    
    if (!audioPreferences.begin("stations", true)) {
        return "";
    }
    
    String keyName = "name_" + String(index);
    String stationName = audioPreferences.getString(keyName.c_str(), "");
    audioPreferences.end();
    
    if (stationName.length() > 0) {
        strncpy(name, stationName.c_str(), sizeof(name) - 1);
        return name;
    }
    return "";
}

/**
 * @brief Перемістити станцію в списку
 */
bool audioPlayerMoveStation(int index, int direction) {
    int count = audioPlayerGetStationCount();
    if (count < 2) return false;  // Немає що переміщувати
    
    int newIndex = index + direction;
    if (newIndex < 0 || newIndex >= count) return false;  // Вихід за межі
    
    if (!audioPreferences.begin("stations", false)) {
        return false;
    }
    
    // Отримуємо дані станції яку переміщуємо
    String keyName1 = "name_" + String(index);
    String keyUrl1 = "url_" + String(index);
    String keyVol1 = "vol_" + String(index);
    String name1 = audioPreferences.getString(keyName1.c_str(), "");
    String url1 = audioPreferences.getString(keyUrl1.c_str(), "");
    uint8_t vol1 = audioPreferences.getUChar(keyVol1.c_str(), 80);
    
    // Отримуємо дані станції з яким міняємо
    String keyName2 = "name_" + String(newIndex);
    String keyUrl2 = "url_" + String(newIndex);
    String keyVol2 = "vol_" + String(newIndex);
    String name2 = audioPreferences.getString(keyName2.c_str(), "");
    String url2 = audioPreferences.getString(keyUrl2.c_str(), "");
    uint8_t vol2 = audioPreferences.getUChar(keyVol2.c_str(), 80);
    
    if (name1.length() == 0 || name2.length() == 0) {
        audioPreferences.end();
        return false;
    }
    
    // Міняємо місцями
    audioPreferences.putString(keyName1.c_str(), name2);
    audioPreferences.putString(keyUrl1.c_str(), url2);
    audioPreferences.putUChar(keyVol1.c_str(), vol2);
    audioPreferences.putString(keyName2.c_str(), name1);
    audioPreferences.putString(keyUrl2.c_str(), url1);
    audioPreferences.putUChar(keyVol2.c_str(), vol1);
    
    audioPreferences.end();
    
    Serial.printf("[Audio] Moved station %d to %d\n", index, newIndex);
    return true;
}

/**
 * @brief Оновити гучність станції
 */
bool audioPlayerUpdateStationVolume(int index, uint8_t volume) {
    if (index < 0 || index >= audioPlayerGetStationCount()) {
        return false;
    }
    
    if (!audioPreferences.begin("stations", false)) {
        return false;
    }
    
    String keyVol = "vol_" + String(index);
    audioPreferences.putUChar(keyVol.c_str(), volume);
    audioPreferences.end();
    
    Serial.printf("[Audio] Updated station %d volume to %d\n", index, volume);
    return true;
}

/**
 * @brief Оновити дані станції (назва, URL, гучність)
 */
bool audioPlayerUpdateStation(int index, const char* name, const char* url, uint8_t volume) {
    if (index < 0 || index >= audioPlayerGetStationCount()) {
        return false;
    }
    
    if (!audioPreferences.begin("stations", false)) {
        return false;
    }
    
    String keyName = "name_" + String(index);
    String keyUrl = "url_" + String(index);
    String keyVol = "vol_" + String(index);
    
    audioPreferences.putString(keyName.c_str(), name);
    audioPreferences.putString(keyUrl.c_str(), url);
    audioPreferences.putUChar(keyVol.c_str(), volume);
    
    audioPreferences.end();
    
    Serial.printf("[Audio] Updated station %d: %s (vol=%d)\n", index, name, volume);
    return true;
}

/**
 * @brief Отримати поточну гучність
 */
uint8_t audioPlayerGetCurrentVolume() {
    return currentVolume;
}

/**
 * @brief Встановити поточну гучність
 */
void audioPlayerSetCurrentVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    currentVolume = volume;
    audio.setVolume(volume);
    Serial.printf("[Audio] Volume set to: %d\n", volume);
}

/**
 * @brief Отримати поточний індекс станції
 */
int audioPlayerGetCurrentStationIndex() {
    return currentStationIndex;
}

/**
 * @brief Встановити поточний індекс станції
 */
void audioPlayerSetCurrentStationIndex(int index) {
    currentStationIndex = index;
}

/**
 * @brief Зберегти індекс поточної станції в енергонезалежну пам'ять
 */
void audioPlayerSaveCurrentStation() {
    if (!audioPreferences.begin("player", false)) {
        Serial.println("[Audio] Error saving station index");
        return;
    }
    audioPreferences.putInt("last_station", currentStationIndex);
    audioPreferences.end();
    Serial.printf("[Audio] Saved station index: %d\n", currentStationIndex);
}

/**
 * @brief Завантажити індекс останньої станції з енергонезалежної пам'яті
 * @return Індекс станції або -1 якщо не збережено
 */
int audioPlayerLoadCurrentStation() {
    if (!audioPreferences.begin("player", true)) {
        return -1;
    }
    int index = audioPreferences.getInt("last_station", -1);
    audioPreferences.end();
    Serial.printf("[Audio] Loaded station index: %d\n", index);
    return index;
}
