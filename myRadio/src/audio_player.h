/**
 * @file audio_player.h
 * @brief Модуль для відтворення інтернет-радіо через I2S
 * Використовує ESP32-audioI2S library v2.3.0
 */

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include <Audio.h>
#include <Preferences.h>
#include "config.h"

// Піни для I2S DAC

// Структура радіостанції
typedef struct {
    char name[32];
    char url[128];
    uint8_t volume;  // Гучність для цієї станції (0-100)
} RadioStation;

// Стан плеєра
typedef enum {
    PLAYER_STOPPED,
    PLAYER_CONNECTING,
    PLAYER_PLAYING,
    PLAYER_ERROR
} PlayerState;

bool audioPlayerInit();
bool audioPlayerConnect(const char* url, const char* name, uint8_t volume = 20);
int audioPlayerLoop();
void audioPlayerStop();
PlayerState audioPlayerGetState();
const char* audioPlayerGetStationName();
void audioPlayerSetVolume(uint8_t volume);
uint8_t audioPlayerGetVolume();
void audioPlayerEnd();
const char* audioPlayerGetArtist();
const char* audioPlayerGetTitle();

// Функції для роботи з радіостанціями
bool audioPlayerAddStation(const char* name, const char* url, uint8_t volume = 20);
bool audioPlayerRemoveStation(int index);
bool audioPlayerGetStation(int index, RadioStation* station);
int audioPlayerGetStationCount();
void audioPlayerClearStations();
const char* audioPlayerGetStationNameByIndex(int index);
bool audioPlayerMoveStation(int index, int direction);
bool audioPlayerUpdateStationVolume(int index, uint8_t volume);
bool audioPlayerUpdateStation(int index, const char* name, const char* url, uint8_t volume);
uint8_t audioPlayerGetCurrentVolume();
void audioPlayerSetCurrentVolume(uint8_t volume);
int audioPlayerGetCurrentStationIndex();
void audioPlayerSetCurrentStationIndex(int index);

// Збереження та завантаження індексу станції
void audioPlayerSaveCurrentStation();
int audioPlayerLoadCurrentStation();

// Отримати поточного виконавця
const char* audioPlayerGetArtist();

// Отримати поточну назву композиції
const char* audioPlayerGetTitle();

#endif // AUDIO_PLAYER_H
