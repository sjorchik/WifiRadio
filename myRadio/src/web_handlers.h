/**
 * @file web_handlers.h
 * @brief Обробники HTTP запитів веб-сервера
 */

#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>

// Оголошення функцій обробників
void handleRoot();
void handleAPSetup();
void handleSettings();
void handleStations();
void handleIpAddr();

void handleAudioStatus();
void handleAudioPlay();
void handleAudioStop();
void handleAudioVolume();
void handleAudioStations();
void handleAudioStationsAdd();
void handleAudioStationsDel();
void handleAudioStationsMove();
void handleAudioStationsVolume();
void handleAudioStationsUpdate();
void handleAudioPlayStation();

// Обробники TDA7318
void handleSound();
void handleSoundSettings();
void handleSoundVolume();
void handleSoundBass();
void handleSoundTreble();
void handleSoundBalance();
void handleSoundInput();
void handleSoundMute();
void handleSoundReset();

// Оголошення функції для закриття списку станцій
void closeStationList();

#endif // WEB_HANDLERS_H
