/**
 * @file tda7318.cpp
 * @brief Модуль для керування аудіопроцесором TDA7318 через I2C
 * На основі бібліотеки з проекту SoundCube
 */

#include "tda7318.h"
#include <Preferences.h>

// Preferences для збереження налаштувань
static Preferences audioPrefs;

// Ключі для збереження
static const char* PREFS_KEY = "tda7318";
static const char* INPUT_KEY = "input";
static const char* VOL_KEY = "vol_%d";    // vol_0, vol_1, vol_2, vol_3
static const char* BASS_KEY = "bass_%d";  // bass_0, bass_0, bass_2, bass_3
static const char* TREBLE_KEY = "treble_%d";
static const char* BAL_KEY = "bal_%d";

// Значення за замовчуванням
static const uint8_t DEFAULT_VOLUME = 50;
static const int8_t DEFAULT_BASS = 0;
static const int8_t DEFAULT_TREBLE = 0;
static const int8_t DEFAULT_BALANCE = 0;

// Глобальна змінна стану
static TDA7318_State tdaState = {
    .volume = 50,
    .bass = 0,
    .treble = 0,
    .balance = 0,
    .input = INPUT_WIFI_RADIO,
    .muted = false
};

/**
 * @brief Завантажити налаштування для поточного входу
 * @param input Вхід для якого завантажувати налаштування
 */
static void tda7318LoadSettingsForInput(TDA7318_Input input) {
    if (!audioPrefs.begin(PREFS_KEY, true)) {
        return;
    }
    
    char key[32];
    
    sprintf(key, VOL_KEY, input);
    tdaState.volume = audioPrefs.getUChar(key, DEFAULT_VOLUME);
    
    sprintf(key, BASS_KEY, input);
    tdaState.bass = audioPrefs.getChar(key, DEFAULT_BASS);
    
    sprintf(key, TREBLE_KEY, input);
    tdaState.treble = audioPrefs.getChar(key, DEFAULT_TREBLE);
    
    sprintf(key, BAL_KEY, input);
    tdaState.balance = audioPrefs.getChar(key, DEFAULT_BALANCE);
    
    audioPrefs.end();
    
    Serial.printf("[TDA7318] Loaded settings for input %d: vol=%d, bass=%d, treble=%d, bal=%d\n",
                  input, tdaState.volume, tdaState.bass, tdaState.treble, tdaState.balance);
}

/**
 * @brief Завантажити налаштування (вхід та параметри)
 */
static void tda7318LoadSettings() {
    if (!audioPrefs.begin(PREFS_KEY, true)) {
        return;
    }
    
    // Завантажуємо збережений вхід
    uint8_t savedInput = audioPrefs.getUChar(INPUT_KEY, 0);
    if (savedInput > 3) savedInput = 0;
    tdaState.input = (TDA7318_Input)savedInput;
    
    audioPrefs.end();
    
    // Завантажуємо налаштування для збереженого входу
    tda7318LoadSettingsForInput(tdaState.input);
}

/**
 * @brief Зберегти налаштування для поточного входу
 */
static void tda7318SaveSettings() {
    if (!audioPrefs.begin(PREFS_KEY, false)) {
        return;
    }
    
    char key[32];
    
    // Зберігаємо поточний вхід
    audioPrefs.putUChar(INPUT_KEY, tdaState.input);
    
    // Зберігаємо налаштування для поточного входу
    sprintf(key, VOL_KEY, tdaState.input);
    audioPrefs.putUChar(key, tdaState.volume);
    
    sprintf(key, BASS_KEY, tdaState.input);
    audioPrefs.putChar(key, tdaState.bass);
    
    sprintf(key, TREBLE_KEY, tdaState.input);
    audioPrefs.putChar(key, tdaState.treble);
    
    sprintf(key, BAL_KEY, tdaState.input);
    audioPrefs.putChar(key, tdaState.balance);
    
    audioPrefs.end();
    
    Serial.printf("[TDA7318] Saved settings for input %d: vol=%d, bass=%d, treble=%d, bal=%d\n",
                  tdaState.input, tdaState.volume, tdaState.bass, tdaState.treble, tdaState.balance);
}

/**
 * @brief Конвертувати гучність 0-100 в регістр TDA7318 (0-63)
 */
static char tda7318VolumeToReg(uint8_t volume) {
    if (volume > 100) volume = 100;
    char temp = (char)map(volume, 0, 100, 0, 63);
    return 63 - temp;  // Інверсія: 0 = max attenuation, 63 = min attenuation
}

/**
 * @brief Конвертувати бас/високі в регістр TDA7318
 */
static char tda7318ToneToReg(int8_t value, char baseReg) {
    char temp = baseReg;
    if (value > 0) {
        temp += 8;
        temp += (7 - value);
    } else {
        temp += (7 + value);
    }
    return temp;
}

/**
 * @brief Встановити gain для динаміка
 */
static char tda7318SpeakerAtt(int speaker, int gain) {
    char temp = 0;
    if (speaker == 1) temp = 0b10000000;
    else if (speaker == 2) temp = 0b10100000;
    else if (speaker == 3) temp = 0b11000000;
    else if (speaker == 4) temp = 0b11100000;
    temp += gain;
    return temp;
}

/**
 * @brief Встановити вхід та gain
 */
static char tda7318AudioSwitch(int channel, int gain) {
    char temp = 0b01000000;  // MSB 010
    temp += ((3 - gain) << 3);
    temp += channel;
    return temp;
}

bool tda7318Init() {
    Serial.println(F("[TDA7318] Initializing..."));
    
    // Ініціалізація I2C
    Wire.begin();
    
    // Затримка для стабілізації
    delay(10);
    
    // Завантажуємо збережені налаштування
    tda7318LoadSettings();
    
    // Встановити початкові значення
    tda7318SetVolume(tdaState.volume);
    tda7318SetBass(tdaState.bass);
    tda7318SetTreble(tdaState.treble);
    tda7318SetBalance(tdaState.balance);
    tda7318SetInput(tdaState.input);
    
    // Встановити gain для всіх динаміків на максимум (0 = max gain)
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(tda7318SpeakerAtt(1, 0));
    Wire.write(tda7318SpeakerAtt(2, 0));
    Wire.write(tda7318SpeakerAtt(3, 0));
    Wire.write(tda7318SpeakerAtt(4, 0));
    Wire.endTransmission();
    
    Serial.println(F("[TDA7318] Initialized"));
    return true;
}

void tda7318SetVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    tdaState.volume = volume;
    
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(tda7318VolumeToReg(volume));
    Wire.endTransmission();
    
    // Розм'ютити при зміні гучності
    if (volume > 0) {
        tda7318UnMute();
    }
    
    tda7318SaveSettings();
    Serial.printf("[TDA7318] Volume set to: %d\n", volume);
}

uint8_t tda7318GetVolume() {
    return tdaState.volume;
}

void tda7318SetBass(int8_t value) {
    if (value < -7) value = -7;
    if (value > 7) value = 7;
    tdaState.bass = value;
    
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(tda7318ToneToReg(value, 96));  // 96 = 0b01100000
    Wire.endTransmission();
    
    tda7318SaveSettings();
    Serial.printf("[TDA7318] Bass set to: %d\n", value);
}

int8_t tda7318GetBass() {
    return tdaState.bass;
}

void tda7318SetTreble(int8_t value) {
    if (value < -7) value = -7;
    if (value > 7) value = 7;
    tdaState.treble = value;
    
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(tda7318ToneToReg(value, 112));  // 112 = 0b01110000
    Wire.endTransmission();
    
    tda7318SaveSettings();
    Serial.printf("[TDA7318] Treble set to: %d\n", value);
}

int8_t tda7318GetTreble() {
    return tdaState.treble;
}

void tda7318SetBalance(int8_t value) {
    if (value < -7) value = -7;
    if (value > 7) value = 7;
    tdaState.balance = value;
    
    // Розрахунок гучності для лівого та правого каналів
    // -7 = лівий макс, правий мін; +7 = правий макс, лівий мін
    char leftGain, rightGain;
    if (value < 0) {
        leftGain = 0;  // Лівий канал без змін
        rightGain = (char)map(abs(value), 0, 7, 0, 31);  // Правий канал затихається
    } else if (value > 0) {
        leftGain = (char)map(value, 0, 7, 0, 31);  // Лівий канал затихається
        rightGain = 0;  // Правий канал без змін
    } else {
        leftGain = 0;
        rightGain = 0;
    }
    
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(0b10000000 | leftGain);   // Лівий канал (Speaker 1)
    Wire.write(0b10100000 | rightGain);  // Правий канал (Speaker 2)
    Wire.endTransmission();
    
    tda7318SaveSettings();
    Serial.printf("[TDA7318] Balance set to: %d (L:%d, R:%d)\n", value, leftGain, rightGain);
}

int8_t tda7318GetBalance() {
    return tdaState.balance;
}

void tda7318SetInput(TDA7318_Input input, uint8_t gain) {
    if (input > INPUT_AUX) input = INPUT_WIFI_RADIO;
    if (gain > 3) gain = 3;
    
    // Зберігаємо налаштування для попереднього входу
    tda7318SaveSettings();
    
    // Зберігаємо новий вхід
    tdaState.input = input;
    
    if (!audioPrefs.begin(PREFS_KEY, false)) {
        return;
    }
    audioPrefs.putUChar(INPUT_KEY, input);
    audioPrefs.end();
    
    // Завантажуємо налаштування для нового входу
    tda7318LoadSettingsForInput(input);
     
    // Встановлюємо вхід на TDA7318
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(tda7318AudioSwitch(input, gain));
    Wire.endTransmission();
    
    // Встановлюємо збережені налаштування для нового входу
    tda7318SetVolume(tdaState.volume);
    tda7318SetBass(tdaState.bass);
    tda7318SetTreble(tdaState.treble);
    tda7318SetBalance(tdaState.balance);
    
    Serial.printf("[TDA7318] Input set to: %d, gain: %d\n", input, gain);
}

TDA7318_Input tda7318GetInput() {
    return tdaState.input;
}

void tda7318Mute() {
    if (tdaState.muted) return;  // Вже в mute
    tdaState.muted = true;
    
    // Відправляємо I2C команду - встановлюємо мінімальну гучність (max attenuation)
    // Регістр гучності: 0x3F = -47.5dB (практично тиша)
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(0x00);  // Sub address 0 = Volume
    Wire.write(0x3F);  // Max attenuation (mute)
    Wire.endTransmission();
    
    Serial.println(F("[TDA7318] Muted (I2C command sent)"));
}

void tda7318UnMute() {
    if (!tdaState.muted) return;  // Вже не в mute
    tdaState.muted = false;
    
    // Відправляємо I2C команду - відновлюємо реальну гучність
    uint8_t regVolume = tda7318VolumeToReg(tdaState.volume);
    Wire.beginTransmission(TDA7318_I2C_ADDR);
    Wire.write(0x00);  // Sub address 0 = Volume
    Wire.write(regVolume);
    Wire.endTransmission();
    
    Serial.printf("[TDA7318] Unmuted, volume restored to %d (reg=0x%02X)\n", tdaState.volume, regVolume);
}

void tda7318SetMute(bool muted) {
    if (muted) {
        tda7318Mute();
    } else {
        tda7318UnMute();
    }
}

bool tda7318IsMuted() {
    return tdaState.muted;
}

TDA7318_State tda7318GetState() {
    return tdaState;
}

void tda7318SetState(const TDA7318_State& state) {
    tda7318SetVolume(state.volume);
    tda7318SetBass(state.bass);
    tda7318SetTreble(state.treble);
    tda7318SetBalance(state.balance);
    tda7318SetInput(state.input);
    tda7318SetMute(state.muted);
}
