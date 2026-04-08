/**
 * @file display.cpp
 * @brief Модуль для керування дисплеєм ST7789 170x320
 * Бібліотека: TFT_eSPI з підтримкою української мови та DMA
 */

#include "display.h"
#include <SPIFFS.h>

// Глобальний об'єкт дисплея
TFT_eSPI tft = TFT_eSPI();

// Глобальний буфер для UTF-8 конвертації
static char utf8Buffer[256];

// Стан підсвітки дисплея
static bool backlightState = true;

void displaySetBacklight(bool on) {
    backlightState = on;
    digitalWrite(TFT_BL, on ? HIGH : LOW);
    Serial.printf("[Display] Backlight %s (GPIO %d)\n", on ? "ON" : "OFF", TFT_BL);
}

void displayInit() {
    Serial.println(F("[Display] Initializing..."));
    
    // Ініціалізація підсвітки - увімкнути
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    delay(100);
    
    // Ініціалізація TFT
    tft.init();
    
    // Примусове увімкнення підсвітки після ініціалізації
    digitalWrite(TFT_BL, HIGH);
    
    tft.setRotation(1);  // Ландшафтна орієнтація (90 градусів)
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setSwapBytes(true);  // Для коректного відображення кольорів
    
    // Завантаження шрифту з українською мовою з SPIFFS
    if (SPIFFS.exists("/Arsenal-Bold26.vlw")) {
        tft.loadFont("Arsenal-Bold26");
        Serial.println(F("[Display] Font Arsenal-Bold26 loaded from SPIFFS"));
    } else {
        Serial.println(F("[Display] Font not found in SPIFFS"));
        Serial.println(F("[Display] Upload filesystem: pio run -t uploadfs"));
    }

    Serial.println(F("[Display] Initialized"));
    Serial.printf("[Display] Backlight ON (GPIO %d)\n", TFT_BL);
    Serial.printf("[Display] Size: %dx%d\n", tft.width(), tft.height());
}

void displayShowConnecting() {
    tft.fillScreen(COLOR_BLACK);
    tft.loadFont("Arsenal-Bold26");
    displayPrintCenter("Connecting...", 70, 0, COLOR_LIGHT_GREEN, COLOR_BLACK);
    tft.unloadFont();
}

void displayShowAPInfo(const char* ssid, const char* password, const char* ip) {
    tft.fillScreen(COLOR_BLACK);

    // Верхня панель
    const int barHeight = 25;
    tft.fillRect(0, 0, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);
    tft.loadFont("Arsenal-Bold15");
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);

    const char* headerText = "Access Point";
    int textX = 5;
    int textY = (barHeight - 9) / 2;
    tft.setCursor(textX, textY);
    tft.print(headerText);

    tft.drawLine(0, barHeight, DISPLAY_WIDTH, barHeight, COLOR_WHITE);
    tft.unloadFont();

    // Основна інформація
    tft.loadFont("Arsenal-Bold20");

    int yPos = 40;
    const int lineSpacing = 28;

    // SSID
    tft.setTextColor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    tft.setCursor(10, yPos);
    tft.print("SSID: ");
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.print(ssid);

    yPos += lineSpacing;

    // Password
    tft.setTextColor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    tft.setCursor(10, yPos);
    tft.print("Pass: ");
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.print(password);

    yPos += lineSpacing;

    // IP Address
    tft.setTextColor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    tft.setCursor(10, yPos);
    tft.print("URL: ");
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.print("http://");
    tft.print(ip);

    yPos += lineSpacing + 10;

    // Напис "Press OK" по центру
    tft.loadFont("Arsenal-Bold26");
    displayPrintCenter("Press OK", yPos, 0, COLOR_YELLOW, COLOR_BLACK);
    tft.unloadFont();

    tft.unloadFont();
}

void displayShowOffline() {
    tft.fillScreen(COLOR_BLACK);

    // Верхня панель
    const int barHeight = 25;
    tft.fillRect(0, 0, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);
    tft.loadFont("Arsenal-Bold15");
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);

    const char* headerText = "Offline Mode";
    int textX = 5;
    int textY = (barHeight - 9) / 2;
    tft.setCursor(textX, textY);
    tft.print(headerText);

    tft.drawLine(0, barHeight, DISPLAY_WIDTH, barHeight, COLOR_WHITE);
    tft.unloadFont();

    // Малюємо індикатори
    uint8_t tdaVolume = tda7318GetVolume();
    int8_t balance = tda7318GetBalance();

    // Індикатори
    displayDrawTDAVolumeBar(tdaVolume, true);
    displayDrawBalanceBar(balance, true);

    // Центральна зона - картинка входу
    displayUpdateOfflineCenter(tda7318GetInput());

    // Нижній бар з 3 кнопками
    displayDrawOfflineInputBar(tda7318GetInput());
}

void displayUpdateOfflineCenter(TDA7318_Input activeInput) {
    // Очищаємо центральну зону
    tft.fillRect(0, 26, DISPLAY_WIDTH, 122, COLOR_BLACK);

    // Малюємо індикатори
    uint8_t tdaVolume = tda7318GetVolume();
    int8_t balance = tda7318GetBalance();
    displayDrawTDAVolumeBar(tdaVolume, true);
    displayDrawBalanceBar(balance, true);

    // Картинка входу по центру
    if (activeInput == INPUT_COMPUTER) {
        displayDrawBmpCenter("/out_computer.bmp");
    } else if (activeInput == INPUT_TV_BOX) {
        displayDrawBmpCenter("/out_tvbox.bmp");
    } else if (activeInput == INPUT_AUX) {
        displayDrawBmpCenter("/out_aux.bmp");
    }
}

void displayClear(uint16_t color) {
    tft.fillScreen(color);
}

void displayPrint(const char* text, int x, int y, uint8_t font, uint16_t color, uint16_t bg) {
    tft.setTextColor(color, bg);
    
    // Встановлення шрифту
    switch(font) {
        case 1: tft.setTextFont(1); break;
        case 2: tft.setTextFont(2); break;
        case 4: tft.setTextFont(4); break;
        case 6: tft.setTextFont(6); break;
        case 7: tft.setTextFont(7); break;
        case 8: tft.setTextFont(8); break;
        default: tft.setTextFont(2); break;
    }
    
    tft.setCursor(x, y);
    tft.print(text);
}

void displayPrintCenter(const char* text, int y, uint8_t font, uint16_t color, uint16_t bg) {
    tft.setTextColor(color, bg);
    
    switch(font) {
        case 1: tft.setTextFont(1); break;
        case 2: tft.setTextFont(2); break;
        case 4: tft.setTextFont(4); break;
        case 6: tft.setTextFont(6); break;
        case 7: tft.setTextFont(7); break;
        case 8: tft.setTextFont(8); break;
        default: tft.setTextFont(2); break;
    }
    
    // Отримуємо ширину тексту для центрування
    int textWidth = tft.textWidth(text);
    int x = (DISPLAY_WIDTH - textWidth) / 2;
    
    tft.setCursor(x, y);
    tft.print(text);
}

void displayPrintNum(int value, int x, int y, uint8_t font, uint16_t color, uint16_t bg) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", value);
    displayPrint(buffer, x, y, font, color, bg);
}

void displayDrawRect(int x, int y, int w, int h, uint16_t color, uint16_t fill) {
    if (fill != COLOR_NONE) {
        tft.fillRect(x, y, w, h, fill);
    }
    tft.drawRect(x, y, w, h, color);
}

void displayDrawLine(int x0, int y0, int x1, int y1, uint16_t color, uint8_t thickness) {
    // TFT_eSPI не має setStrokeWidth, малюємо кілька ліній для товщини
    if (thickness <= 1) {
        tft.drawLine(x0, y0, x1, y1, color);
    } else {
        // Малюємо кілька паралельних ліній для імітації товщини
        for (int i = 0; i < thickness; i++) {
            tft.drawLine(x0, y0 + i, x1, y1 + i, color);
        }
    }
}

void displayDrawCircle(int x, int y, int r, uint16_t color, bool fill) {
    if (fill) {
        tft.fillCircle(x, y, r, color);
    } else {
        tft.drawCircle(x, y, r, color);
    }
}

void displayDrawVolumeIcon(int x, int y, uint8_t volume, uint16_t color) {
    // Малюємо іконку динаміка (трикутник + прямокутник)
    tft.fillTriangle(x, y, x, y + 20, x + 10, y + 10, color);
    tft.fillRect(x + 10, y + 5, 5, 10, color);
    
    // Малюємо хвилі гучності (кола без заповнення)
    if (volume > 30) {
        tft.drawCircle(x + 22, y + 10, 8, color);
    }
    if (volume > 60) {
        tft.drawCircle(x + 28, y + 10, 14, color);
    }
    if (volume > 80) {
        tft.drawCircle(x + 34, y + 10, 20, color);
    }
}

void displayShowRadioInfo(const char* stationName, const char* trackArtist, const char* trackTitle, uint8_t volume, TDA7318_Input activeInput, const char* wifiSSID, const char* wifiIP, uint8_t tdaVolume, int8_t balance) {
    // Очищення екрану
    tft.fillScreen(COLOR_BLACK);

    // Малюємо верхню панель з WiFi інформацією
    displayDrawTopBar(wifiSSID, wifiIP);

    // Малюємо індикатор гучності TDA7318 зліва
    displayDrawTDAVolumeBar(tdaVolume);

    // Малюємо індикатор балансу справа
    displayDrawBalanceBar(balance);

    // Для WiFi радіо показуємо інформацію про станцію
    if (activeInput == INPUT_WIFI_RADIO) {
        // Назва станції (по центру, великий шрифт Arsenal-Bold26)
        if (stationName && stationName[0] != '\0') {
            tft.loadFont("Arsenal-Bold26");
            displayPrintCenter(stationName, 45, 0, COLOR_LIGHT_GREEN, COLOR_BLACK);
            tft.unloadFont();
        }

        // Виконавець (по центру) - обрізаємо якщо не поміщається
        if (trackArtist && trackArtist[0] != '\0') {
            tft.loadFont("Arsenal-Bold20");
            int artistWidth = tft.textWidth(trackArtist);
            int maxWidth = DISPLAY_WIDTH - 125;  // Лівий бар 55px + Правий бар 50px + відступи 20px
            if (artistWidth > maxWidth) {
                // Обрізаємо текст з додаванням "..."
                char truncatedArtist[64];
                strncpy(truncatedArtist, trackArtist, sizeof(truncatedArtist) - 1);
                truncatedArtist[sizeof(truncatedArtist) - 1] = '\0';

                // Знаходимо потрібну довжину методом бінарного пошуку
                int maxLen = strlen(truncatedArtist);
                int minLen = 0;
                while (minLen < maxLen) {
                    int mid = (minLen + maxLen + 1) / 2;
                    truncatedArtist[mid] = '\0';
                    if (tft.textWidth(truncatedArtist) + tft.textWidth("...") <= maxWidth) {
                        minLen = mid;
                    } else {
                        maxLen = mid - 1;
                    }
                }
                truncatedArtist[minLen] = '\0';
                strcat(truncatedArtist, "...");
                displayPrintCenter(truncatedArtist, 80, 0, COLOR_LIGHTGRAY, COLOR_BLACK);
            } else {
                displayPrintCenter(trackArtist, 80, 0, COLOR_LIGHTGRAY, COLOR_BLACK);
            }
            tft.unloadFont();
        }

        // Назва треку (по центру) - обрізаємо якщо не поміщається
        if (trackTitle && trackTitle[0] != '\0') {
            tft.loadFont("Arsenal-Bold20");
            int titleWidth = tft.textWidth(trackTitle);
            int maxWidth = DISPLAY_WIDTH - 125;  // Лівий бар 55px + Правий бар 50px + відступи 20px
            if (titleWidth > maxWidth) {
                // Обрізаємо текст з додаванням "..."
                char truncatedTitle[64];
                strncpy(truncatedTitle, trackTitle, sizeof(truncatedTitle) - 1);
                truncatedTitle[sizeof(truncatedTitle) - 1] = '\0';

                // Знаходимо потрібну довжину методом бінарного пошуку
                int maxLen = strlen(truncatedTitle);
                int minLen = 0;
                while (minLen < maxLen) {
                    int mid = (minLen + maxLen + 1) / 2;
                    truncatedTitle[mid] = '\0';
                    if (tft.textWidth(truncatedTitle) + tft.textWidth("...") <= maxWidth) {
                        minLen = mid;
                    } else {
                        maxLen = mid - 1;
                    }
                }
                truncatedTitle[minLen] = '\0';
                strcat(truncatedTitle, "...");
                displayPrintCenter(truncatedTitle, 115, 0, COLOR_CYAN, COLOR_BLACK);
            } else {
                displayPrintCenter(trackTitle, 115, 0, COLOR_CYAN, COLOR_BLACK);
            }
            tft.unloadFont();
        }
    } else if (activeInput == INPUT_COMPUTER) {
        // Для Computer входу - показуємо картинку по центру
        displayDrawBmpCenter("/out_computer.bmp");
    } else if (activeInput == INPUT_TV_BOX) {
        // Для TV Box входу - показуємо картинку по центру
        displayDrawBmpCenter("/out_tvbox.bmp");
    } else if (activeInput == INPUT_AUX) {
        // Для AUX входу - показуємо картинку по центру
        displayDrawBmpCenter("/out_aux.bmp");
    }

    // Малюємо панель аудіо входів знизу (тільки при ініціалізації)
    displayDrawAudioInputBar(activeInput);
}

// Глобальна змінна для збереження попереднього входу
static TDA7318_Input g_lastInputForUpdate = INPUT_WIFI_RADIO;

void displayUpdateStationAndTrack(const char* stationName, const char* trackArtist, const char* trackTitle, bool isPlaying, uint8_t volume, uint8_t tdaVolume, TDA7318_Input activeInput, int8_t balance) {
    // Очищаємо всю центральну зону (від верхньої панелі до нижньої)
    // Верхня панель: 0-25 (біла лінія на 25), Нижня панель: 148-170
    tft.fillRect(0, 26, DISPLAY_WIDTH, 122, COLOR_BLACK);  // 170 - 26 - 22 = 122, починаємо після білої лінії

    // Малюємо індикатор гучності TDA7318 зліва (завжди показуємо)
    displayDrawTDAVolumeBar(tdaVolume);

    // Малюємо індикатор балансу справа
    displayDrawBalanceBar(balance);

    // Розраховуємо доступну ширину для тексту
    // Лівий бар: 6px + (14 * 3px) = 48px максимальна ширина + 5px відступ = 53px
    // Правий бар: 8px + (7 * 5px) = 43px + 5px відступ = 48px
    const int leftBarWidth = 55;  // Місце під індикатор гучності
    const int rightBarWidth = 50;  // Місце під індикатор балансу
    const int horizontalPadding = 10;  // Додатковий відступ по 5px з кожного боку
    const int maxWidth = DISPLAY_WIDTH - leftBarWidth - rightBarWidth - (horizontalPadding * 2);

    // Інформацію показуємо тільки для WiFi входу
    if (activeInput == INPUT_WIFI_RADIO) {
        // Назва станції (по центру, великий шрифт Arsenal-Bold26)
        if (stationName && stationName[0] != '\0') {
            tft.loadFont("Arsenal-Bold26");
            int stationWidth = tft.textWidth(stationName);
            if (stationWidth > maxWidth) {
                // Обрізаємо назву станції якщо задовга
                char truncatedStation[64];
                strncpy(truncatedStation, stationName, sizeof(truncatedStation) - 1);
                truncatedStation[sizeof(truncatedStation) - 1] = '\0';

                int maxLen = strlen(truncatedStation);
                int minLen = 0;
                while (minLen < maxLen) {
                    int mid = (minLen + maxLen + 1) / 2;
                    truncatedStation[mid] = '\0';
                    if (tft.textWidth(truncatedStation) + tft.textWidth("...") <= maxWidth) {
                        minLen = mid;
                    } else {
                        maxLen = mid - 1;
                    }
                }
                truncatedStation[minLen] = '\0';
                strcat(truncatedStation, "...");
                displayPrintCenter(truncatedStation, 45, 0, COLOR_LIGHT_GREEN, COLOR_BLACK);
            } else {
                displayPrintCenter(stationName, 45, 0, COLOR_LIGHT_GREEN, COLOR_BLACK);
            }
            tft.unloadFont();
        }

        if (isPlaying) {
            // Виконавець (по центру) - опущено на 5 пікселів
            if (trackArtist && trackArtist[0] != '\0') {
                // Перевіряємо чи це спеціальне повідомлення (PLAY, PREV, NEXT)
                bool isSpecialMessage = (strcmp(trackArtist, "PLAY") == 0 || 
                                         strcmp(trackArtist, "PREV") == 0 || 
                                         strcmp(trackArtist, "NEXT") == 0);
                
                if (isSpecialMessage) {
                    // Виводимо шрифтом Arsenal-Bold26
                    tft.loadFont("Arsenal-Bold26");
                    if (strcmp(trackArtist, "PLAY") == 0) {
                        displayPrintCenter(trackArtist, 80, 0, COLOR_GREEN, COLOR_BLACK);
                    } else {
                        displayPrintCenter(trackArtist, 80, 0, COLOR_YELLOW, COLOR_BLACK);
                    }
                    tft.unloadFont();
                } else {
                    // Звичайний виконавець - Arsenal-Bold20
                    tft.loadFont("Arsenal-Bold20");
                    int artistWidth = tft.textWidth(trackArtist);
                    if (artistWidth > maxWidth) {
                        // Обрізаємо текст з додаванням "..."
                        char truncatedArtist[64];
                        strncpy(truncatedArtist, trackArtist, sizeof(truncatedArtist) - 1);
                        truncatedArtist[sizeof(truncatedArtist) - 1] = '\0';

                        // Знаходимо потрібну довжину методом бінарного пошуку
                        int maxLen = strlen(truncatedArtist);
                        int minLen = 0;
                        while (minLen < maxLen) {
                            int mid = (minLen + maxLen + 1) / 2;
                            truncatedArtist[mid] = '\0';
                            if (tft.textWidth(truncatedArtist) + tft.textWidth("...") <= maxWidth) {
                                minLen = mid;
                            } else {
                                maxLen = mid - 1;
                            }
                        }
                        truncatedArtist[minLen] = '\0';
                        strcat(truncatedArtist, "...");
                        displayPrintCenter(truncatedArtist, 80, 0, COLOR_LIGHTGRAY, COLOR_BLACK);
                    } else {
                        displayPrintCenter(trackArtist, 80, 0, COLOR_LIGHTGRAY, COLOR_BLACK);
                    }
                    tft.unloadFont();
                }
            }

            // Назва треку (по центру) - зменшено інтервал (110 замість 115)
            if (trackTitle && trackTitle[0] != '\0') {
                tft.loadFont("Arsenal-Bold20");
                int titleWidth = tft.textWidth(trackTitle);
                if (titleWidth > maxWidth) {
                    // Обрізаємо текст з додаванням "..."
                    char truncatedTitle[64];
                    strncpy(truncatedTitle, trackTitle, sizeof(truncatedTitle) - 1);
                    truncatedTitle[sizeof(truncatedTitle) - 1] = '\0';

                    // Знаходимо потрібну довжину методом бінарного пошуку
                    int maxLen = strlen(truncatedTitle);
                    int minLen = 0;
                    while (minLen < maxLen) {
                        int mid = (minLen + maxLen + 1) / 2;
                        truncatedTitle[mid] = '\0';
                        if (tft.textWidth(truncatedTitle) + tft.textWidth("...") <= maxWidth) {
                            minLen = mid;
                        } else {
                            maxLen = mid - 1;
                        }
                    }
                    truncatedTitle[minLen] = '\0';
                    strcat(truncatedTitle, "...");
                    displayPrintCenter(truncatedTitle, 110, 0, COLOR_CYAN, COLOR_BLACK);
                } else {
                    displayPrintCenter(trackTitle, 110, 0, COLOR_CYAN, COLOR_BLACK);
                }
                tft.unloadFont();
            }
        } else {
            // Не грає - виводимо "STOP" шрифтом Arsenal-Bold26
            tft.loadFont("Arsenal-Bold26");
            displayPrintCenter("STOP", 80, 0, COLOR_RED, COLOR_BLACK);
            tft.unloadFont();
        }
    } else if (activeInput == INPUT_COMPUTER) {
        // Для Computer входу - показуємо картинку по центру
        displayDrawBmpCenter("/out_computer.bmp");
    } else if (activeInput == INPUT_TV_BOX) {
        // Для TV Box входу - показуємо картинку по центру
        displayDrawBmpCenter("/out_tvbox.bmp");
    } else if (activeInput == INPUT_AUX) {
        // Для AUX входу - показуємо картинку по центру
        displayDrawBmpCenter("/out_aux.bmp");
    }
    // Для інших входів центральна зона залишається чистою

    // Оновлюємо панель аудіо входів знизу (тільки зміни)
    displayUpdateAudioInputBar(activeInput);
}

void displayClearTrackInfo() {
    // Очищаємо тільки зону інформації про трек (не весь екран)
    tft.fillRect(0, 90, DISPLAY_WIDTH, 50, COLOR_BLACK);
}

void displayDrawVolumeBar(uint8_t volume) {
    if (volume > 100) volume = 100;

    const int baseBarWidth = 6;  // Базова ширина першого сегмента
    const int barX = DISPLAY_WIDTH - baseBarWidth - 5;  // Відступ справа 5 пікселів
    const int barY = 35;  // Починається після верхньої панелі
    const int segmentHeight = 6;  // Висота одного прямокутника
    const int segmentGap = 1;  // Проміжок між прямокутниками
    const int totalSegments = 15;  // Кількість прямокутників
    const int widthIncrement = 3;  // Збільшення ширини на кожен сегмент

    // Малюємо прямокутники знизу вгору
    for (int i = 0; i < totalSegments; i++) {
        int y = barY + (totalSegments - 1 - i) * (segmentHeight + segmentGap);
        
        // Кожен сегмент ширший на 3 пікселі за попередній (знизу вгору)
        int segmentWidth = baseBarWidth + (i * widthIncrement);
        
        // Всі сегменти жовті
        uint16_t segmentBaseColor = COLOR_YELLOW;
        
        // Розраховуємо заповнення для цього сегмента
        // Кожен сегмент представляє ~6.67% гучності
        int segmentMin = i * 100 / totalSegments;
        int segmentMax = (i + 1) * 100 / totalSegments;
        
        if (volume >= segmentMax) {
            // Сегмент повністю заповнений
            tft.fillRect(barX, y, segmentWidth, segmentHeight, segmentBaseColor);
        } else if (volume <= segmentMin) {
            // Сегмент пустий
            tft.fillRect(barX, y, segmentWidth, segmentHeight, COLOR_DARKGRAY);
        } else {
            // Сегмент частково заповнений - малюємо градієнт
            int segmentRange = segmentMax - segmentMin;
            int fillPercent = (volume - segmentMin) * 100 / segmentRange;
            int fillHeight = (segmentHeight * fillPercent) / 100;
            int emptyHeight = segmentHeight - fillHeight;
            
            // Малюємо заповнену частину (жовтий)
            if (fillHeight > 0) {
                tft.fillRect(barX, y + emptyHeight, segmentWidth, fillHeight, segmentBaseColor);
            }
            // Малюємо пусту частину (сірий)
            if (emptyHeight > 0) {
                tft.fillRect(barX, y, segmentWidth, emptyHeight, COLOR_DARKGRAY);
            }
        }
    }
}

void displayDrawTDAVolumeBar(uint8_t volume, bool isActive) {
    if (volume > 100) volume = 100;

    const int baseBarWidth = 6;  // Базова ширина першого сегмента
    const int barX = 5;  // Відступ зліва 5 пікселів
    const int barY = 35;  // Починається після верхньої панелі
    const int segmentHeight = 6;  // Висота одного прямокутника
    const int segmentGap = 1;  // Проміжок між прямокутниками
    const int totalSegments = 15;  // Кількість прямокутників
    const int widthIncrement = 3;  // Збільшення ширини на кожен сегмент

    // Колір сегментів: жовтий якщо активний, сірий якщо ні
    uint16_t segmentBaseColor = isActive ? COLOR_YELLOW : COLOR_DARKGRAY;

    // Малюємо прямокутники знизу вгору
    for (int i = 0; i < totalSegments; i++) {
        int y = barY + (totalSegments - 1 - i) * (segmentHeight + segmentGap);

        // Кожен сегмент ширший на 3 пікселі за попередній (знизу вгору)
        int segmentWidth = baseBarWidth + (i * widthIncrement);

        // Розраховуємо заповнення для цього сегмента
        // Кожен сегмент представляє ~6.67% гучності
        int segmentMin = i * 100 / totalSegments;
        int segmentMax = (i + 1) * 100 / totalSegments;

        if (volume >= segmentMax) {
            // Сегмент повністю заповнений
            tft.fillRect(barX, y, segmentWidth, segmentHeight, segmentBaseColor);
        } else if (volume <= segmentMin) {
            // Сегмент пустий - спочатку чорний, потім сіра рамка
            tft.fillRect(barX, y, segmentWidth, segmentHeight, COLOR_BLACK);
            tft.drawRect(barX, y, segmentWidth, segmentHeight, COLOR_DARKGRAY);
        } else {
            // Сегмент частково заповнений - малюємо градієнт
            int segmentRange = segmentMax - segmentMin;
            int fillPercent = (volume - segmentMin) * 100 / segmentRange;
            int fillHeight = (segmentHeight * fillPercent) / 100;
            int emptyHeight = segmentHeight - fillHeight;

            // Малюємо заповнену частину
            if (fillHeight > 0) {
                tft.fillRect(barX, y + emptyHeight, segmentWidth, fillHeight, segmentBaseColor);
            }
            // Малюємо пусту частину (спочатку чорний, потім сіра рамка)
            if (emptyHeight > 0) {
                tft.fillRect(barX, y, segmentWidth, emptyHeight, COLOR_BLACK);
                tft.drawRect(barX, y, segmentWidth, emptyHeight, COLOR_DARKGRAY);
            }
        }
    }
}

void displayDrawBalanceBar(int8_t balance, bool isActive) {
    // Баланс від -7 до +7 (0 = центр)
    // -7 = повністю вліво, 0 = центр, +7 = повністю вправо

    const int baseBarWidth = 8;  // Базова ширина середнього сегмента
    const int barY = 35;  // Починається після верхньої панелі
    const int segmentHeight = 6;  // Висота одного прямокутника
    const int segmentGap = 1;  // Проміжок між прямокутниками
    const int totalSegments = 15;  // Кількість прямокутників
    const int widthIncrement = 5;  // Збільшення ширини від центру

    // Максимальна ширина індикатора (найширший сегмент)
    const int maxBarWidth = baseBarWidth + (7 * widthIncrement);  // 8 + 35 = 43px

    // Позиція по правій стороні екрану (вирівнювання по правому краю)
    const int barX = DISPLAY_WIDTH - maxBarWidth - 5;  // Відступ справа 5 пікселів

    // Колір сегментів: блакитний якщо активний, сірий якщо ні
    uint16_t segmentBaseColor = isActive ? COLOR_CYAN : COLOR_DARKGRAY;

    // Малюємо прямокутники знизу вгору
    for (int i = 0; i < totalSegments; i++) {
        int y = barY + (totalSegments - 1 - i) * (segmentHeight + segmentGap);

        // Розраховуємо ширину сегмента (найвужчий в центрі, найширший по краях)
        int distanceFromCenter = abs(i - 7);  // Відстань від центру (0-7)
        int segmentWidth = baseBarWidth + (distanceFromCenter * widthIncrement);

        // Вирівнюємо по правій стороні (правий край всіх сегментів на одній лінії)
        int segmentX = barX + (maxBarWidth - segmentWidth);

        // Заповнення залежить від напрямку балансу
        // Сегмент 7 (центр, i=7) - заповнений завжди
        // Баланс > 0 (вправо): заповнюємо сегменти 7, 8, 9... (i > 7)
        // Баланс < 0 (вліво): заповнюємо сегменти 7, 6, 5... (i < 7)
        bool isFilled = false;

        if (balance >= 0) {
            // Центр або вправо: заповнюємо від центру вправо
            // Сегмент 7 завжди заповнений
            // Сегменти 8-14 заповнені якщо balance >= (i - 7)
            if (i >= 7) {
                // Праві сегменти (8-14) або центр (7)
                isFilled = (balance >= (i - 7));
            }
            // Ліві сегменти (0-6) не заповнені при balance >= 0
        } else {
            // Вліво: заповнюємо від центру вліво
            // Сегмент 7 завжди заповнений
            // Сегменти 6-0 заповнені якщо abs(balance) >= (7 - i)
            if (i <= 7) {
                // Ліві сегменти (0-6) або центр (7)
                isFilled = (abs(balance) >= (7 - i));
            }
            // Праві сегменти (8-14) не заповнені при balance < 0
        }

        if (isFilled) {
            // Сегмент заповнений кольором
            tft.fillRect(segmentX, y, segmentWidth, segmentHeight, segmentBaseColor);
        } else {
            // Сегмент пустий - спочатку чорний, потім сіра рамка
            tft.fillRect(segmentX, y, segmentWidth, segmentHeight, COLOR_BLACK);
            tft.drawRect(segmentX, y, segmentWidth, segmentHeight, COLOR_DARKGRAY);
        }
    }
}

void displayDrawToneBarVertical(int8_t value, int barX, uint16_t color, bool isActive) {
    // Тембр/Бас від -7 до +7
    // -7 = заповнений 1 сегмент (найнижчий)
    // 0 = заповнено 8 сегментів
    // +7 = заповнені всі 15 сегментів

    // Перетворюємо значення в кількість заповнених сегментів (0-15)
    int filledSegments = value + 8;  // -7→1, 0→8, +7→15
    if (filledSegments < 1) filledSegments = 1;
    if (filledSegments > 15) filledSegments = 15;

    const int baseBarWidth = 8;  // Базова ширина найнижчого сегмента
    const int barY = 35;  // Починається після верхньої панелі (так само як гучність і баланс)
    const int segmentHeight = 6;  // Висота одного прямокутника
    const int segmentGap = 1;  // Проміжок між прямокутниками
    const int totalSegments = 15;  // Кількість прямокутників
    const int widthIncrement = 4;  // Збільшення ширини від низу до верху

    // Максимальна ширина індикатора (найширший верхній сегмент)
    const int maxBarWidth = baseBarWidth + ((totalSegments - 1) * widthIncrement);  // 8 + 56 = 64px

    // Колір сегментів: вказаний якщо активний, сірий якщо ні
    uint16_t segmentBaseColor = isActive ? color : COLOR_DARKGRAY;

    // Очищаємо область індикатора перед малюванням
    tft.fillRect(barX, barY, maxBarWidth, totalSegments * (segmentHeight + segmentGap), COLOR_BLACK);

    // Малюємо прямокутники знизу вгору
    for (int i = 0; i < totalSegments; i++) {
        int y = barY + (totalSegments - 1 - i) * (segmentHeight + segmentGap);

        // Розраховуємо ширину сегмента (найвужчий внизу, найширший вгорі)
        int segmentWidth = baseBarWidth + (i * widthIncrement);

        // Центруємо сегмент відносно barX
        int segmentX = barX + (maxBarWidth - segmentWidth) / 2;

        // Заповнення знизу вгору
        bool isFilled = (i < filledSegments);

        if (isFilled) {
            // Сегмент заповнений кольором
            tft.fillRect(segmentX, y, segmentWidth, segmentHeight, segmentBaseColor);
        } else {
            // Сегмент пустий - спочатку чорний, потім сіра рамка
            tft.fillRect(segmentX, y, segmentWidth, segmentHeight, COLOR_BLACK);
            tft.drawRect(segmentX, y, segmentWidth, segmentHeight, COLOR_DARKGRAY);
        }
    }
}

void displayUpdateToneBarSegment(int8_t oldValue, int8_t newValue, int barX, uint16_t color) {
    // Оновлення тільки змінених сегментів індикатора тембру
    // oldValue: старе значення (-7 до +7)
    // newValue: нове значення (-7 до +7)

    const int barY = 35;
    const int segmentHeight = 6;
    const int segmentGap = 1;
    const int totalSegments = 15;
    const int baseBarWidth = 8;
    const int widthIncrement = 4;
    const int maxBarWidth = baseBarWidth + ((totalSegments - 1) * widthIncrement);  // 8 + 56 = 64px

    // Перетворюємо значення в кількість заповнених сегментів
    int oldFilled = oldValue + 8;  // -7→1, 0→8, +7→15
    int newFilled = newValue + 8;
    if (oldFilled < 1) oldFilled = 1;
    if (oldFilled > 15) oldFilled = 15;
    if (newFilled < 1) newFilled = 1;
    if (newFilled > 15) newFilled = 15;

    // Визначаємо які сегменти потрібно оновити
    int startSegment = min(oldFilled, newFilled);
    int endSegment = max(oldFilled, newFilled);

    // Малюємо тільки змінені сегменти
    for (int i = startSegment - 1; i < endSegment; i++) {
        int y = barY + (totalSegments - 1 - i) * (segmentHeight + segmentGap);
        int segmentWidth = baseBarWidth + (i * widthIncrement);
        int segmentX = barX + (maxBarWidth - segmentWidth) / 2;

        // Визначаємо як малювати сегмент
        if (i < newFilled) {
            // Сегмент заповнений кольором
            tft.fillRect(segmentX, y, segmentWidth, segmentHeight, color);
        } else {
            // Сегмент пустий - спочатку чорний, потім сіра рамка
            tft.fillRect(segmentX, y, segmentWidth, segmentHeight, COLOR_BLACK);
            tft.drawRect(segmentX, y, segmentWidth, segmentHeight, COLOR_DARKGRAY);
        }
    }
}

void displayUpdateToneBarBassSegment(int8_t oldValue, int8_t newValue, int barX) {
    // Оновлення тільки змінених сегментів басу
    displayUpdateToneBarSegment(oldValue, newValue, barX, COLOR_LIGHT_GREEN);
}

void displayUpdateToneBarTrebleSegment(int8_t oldValue, int8_t newValue, int barX) {
    // Оновлення тільки змінених сегментів тембру
    displayUpdateToneBarSegment(oldValue, newValue, barX, COLOR_LIGHT_GREEN);
}

void displayUpdateActiveIndicator(uint8_t oldControl, uint8_t newControl, uint8_t volume, int8_t bass, int8_t treble, int8_t balance) {
    // Оновлення тільки змінених індикаторів при перемиканні активного елемента
    // Старий активний індикатор стає сірим, новий стає кольоровим
    
    const int bassX = 81;
    const int trebleX = 177;
    
    // Оновлюємо старий активний індикатор (стає сірим)
    switch (oldControl) {
        case 0:  // Гучність
            displayDrawTDAVolumeBar(volume, false);
            break;
        case 1:  // Бас
            displayDrawToneBarVertical(bass, bassX, COLOR_LIGHT_GREEN, false);
            break;
        case 2:  // Тембр
            displayDrawToneBarVertical(treble, trebleX, COLOR_LIGHT_GREEN, false);
            break;
        case 3:  // Баланс
            displayDrawBalanceBar(balance, false);
            break;
    }
    
    // Оновлюємо новий активний індикатор (стає кольоровим)
    switch (newControl) {
        case 0:  // Гучність
            displayDrawTDAVolumeBar(volume, true);
            break;
        case 1:  // Бас
            displayDrawToneBarVertical(bass, bassX, COLOR_LIGHT_GREEN, true);
            break;
        case 2:  // Тембр
            displayDrawToneBarVertical(treble, trebleX, COLOR_LIGHT_GREEN, true);
            break;
        case 3:  // Баланс
            displayDrawBalanceBar(balance, true);
            break;
    }

    // Оновлюємо нижню панель з назвами індикаторів
    displayUpdateSoundControlBar(newControl);
}

void displayUpdateActiveControlFrame(uint8_t oldControl, uint8_t newControl) {
    // Функція видалена - рамка більше не потрібна
    // Кольори тексту оновлюються в displayUpdateActiveIndicator()
}

void displayShowSoundSettings(uint8_t volume, int8_t balance, int8_t bass, int8_t treble, uint8_t activeControl) {
    // Очищаємо весь екран
    tft.fillScreen(COLOR_BLACK);

    // Малюємо верхню панель з написом "Налаштування звуку"
    displayDrawSoundSettingsHeader();

    // Розташування індикаторів (екран 320×170, повернутий на 90°):
    // DISPLAY_WIDTH = 320, DISPLAY_HEIGHT = 170
    // Гучність: x=6, ширина 43px (правий край = 49px)
    // Баланс: x=271 (DISPLAY_WIDTH - 43 - 6 = 320 - 49 = 271), ширина 43px
    // Вільний простір між ними: 271 - 49 = 222px
    
    // Для двох індикаторів тембрів шириною 64px (8 + 14×4) + 3 відступи по 32px:
    // 64 + 32 + 64 + 32 = 192px (вміщується в 222px, залишок 30px)
    
    const int volumeBarX = 6;  // Гучність зліва
    const int volumeBarWidth = 43;  // maxBarWidth гучності
    const int balanceBarX = DISPLAY_WIDTH - 43 - 6;  // Баланс справа (x=271)
    const int balanceBarWidth = 43;  // maxBarWidth балансу
    
    const int toneBarWidth = 64;  // Ширина індикаторів тембрів (8 + 14×4)
    const int gap = 32;  // Відступ між індикаторами
    
    // Позиції індикаторів тембрів
    const int bassX = volumeBarX + volumeBarWidth + gap;  // 6 + 43 + 32 = 81px
    const int trebleX = bassX + toneBarWidth + gap;  // 81 + 64 + 32 = 177px
    
    // Малюємо індикатори в ряд (всі починаються з y=35)
    // Тільки активний індикатор показує правильний колір, інші - сірий
    displayDrawTDAVolumeBar(volume, activeControl == 0);      // Ліворуч - гучність (x=6)
    displayDrawToneBarVertical(bass, bassX, COLOR_LIGHT_GREEN, activeControl == 1);    // Бас (x=81)
    displayDrawToneBarVertical(treble, trebleX, COLOR_LIGHT_GREEN, activeControl == 2);  // Тембр (x=177)
    displayDrawBalanceBar(balance, activeControl == 3);        // Праворуч - баланс (x=271)

    // Малюємо нижню панель з назвами індикаторів
    displayDrawSoundControlBar(activeControl);
}

void displayUpdateVolumeBars(uint8_t volume, uint8_t tdaVolume, int8_t balance) {
    // Оновлюємо тільки індикатори гучності та балансу без очищення всього екрану
    // isActive=true для сумісності (поза режимом налаштувань всі індикатори активні)
    displayDrawTDAVolumeBar(tdaVolume, true);
    displayDrawBalanceBar(balance, true);
}

void displayDrawSoundControlBar(uint8_t activeControl) {
    // Назви індикаторів
    const char* controlNames[] = {"Volume", "Bass", "Treble", "Balance"};
    const int numControls = 4;

    // Параметри панелі
    const int barHeight = 20;
    const int barY = DISPLAY_HEIGHT - barHeight - 2;  // Відступ знизу 2 пікселі
    const int controlWidth = (DISPLAY_WIDTH - (numControls + 1) * 2) / numControls;  // Ширина кожного індикатора + відступи

    // Малюємо фон панелі
    tft.fillRect(0, barY, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);

    // Малюємо кожен індикатор
    for (int i = 0; i < numControls; i++) {
        int x = 2 + i * (controlWidth + 2);  // Відступ 2 пікселі між секціями

        // Визначаємо кольори
        uint16_t bgColor, textColor;
        if (i == activeControl) {
            bgColor = COLOR_GREEN;      // Активний індикатор - зелений фон
            textColor = COLOR_BLACK;    // Чорний текст
        } else {
            bgColor = COLOR_GRAY;       // Неактивний - сірий фон
            textColor = COLOR_WHITE;    // Білий текст
        }

        // Малюємо прямокутник індикатора
        tft.fillRect(x, barY, controlWidth, barHeight, bgColor);

        // Малюємо рамку
        tft.drawRect(x, barY, controlWidth, barHeight, COLOR_WHITE);

        // Виводимо назву індикатора по центру (шрифт Arsenal-Bold15)
        tft.loadFont("Arsenal-Bold15");
        tft.setTextColor(textColor, bgColor);

        // Отримуємо ширину тексту для центрування
        int textWidth = tft.textWidth(controlNames[i]);
        int textX = x + (controlWidth - textWidth) / 2;
        int textY = barY + (barHeight - 14) / 2;  // Центрування по вертикалі

        tft.setCursor(textX, textY);
        tft.print(controlNames[i]);

        tft.unloadFont();  // Вивантажити шрифт
    }

    // Відновлюємо основний шрифт
    tft.loadFont("Arsenal-Bold26");
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
}

void displayUpdateSoundControlBar(uint8_t activeControl) {
    // Зберігаємо попередній активний індикатор
    static uint8_t lastActiveControl = 0;

    // Якщо індикатор не змінився - нічого не робимо
    if (activeControl == lastActiveControl) {
        return;
    }

    // Назви індикаторів
    const char* controlNames[] = {"Volume", "Bass", "Treble", "Balance"};
    const int numControls = 4;

    // Параметри панелі
    const int barHeight = 20;
    const int barY = DISPLAY_HEIGHT - barHeight - 2;
    const int controlWidth = (DISPLAY_WIDTH - (numControls + 1) * 2) / numControls;

    // Перемалюємо тільки змінені секції
    for (int i = 0; i < numControls; i++) {
        // Малюємо тільки якщо це попередній або новий активний індикатор
        if (i == lastActiveControl || i == activeControl) {
            int x = 2 + i * (controlWidth + 2);

            // Визначаємо кольори
            uint16_t bgColor, textColor;
            if (i == activeControl) {
                bgColor = COLOR_GREEN;      // Активний індикатор - зелений фон
                textColor = COLOR_BLACK;    // Чорний текст
            } else {
                bgColor = COLOR_GRAY;       // Неактивний - сірий фон
                textColor = COLOR_WHITE;    // Білий текст
            }

            // Малюємо прямокутник індикатора
            tft.fillRect(x, barY, controlWidth, barHeight, bgColor);

            // Малюємо рамку
            tft.drawRect(x, barY, controlWidth, barHeight, COLOR_WHITE);

            // Виводимо назву індикатора по центру
            tft.loadFont("Arsenal-Bold15");
            tft.setTextColor(textColor, bgColor);

            int textWidth = tft.textWidth(controlNames[i]);
            int textX = x + (controlWidth - textWidth) / 2;
            int textY = barY + (barHeight - 14) / 2;

            tft.setCursor(textX, textY);
            tft.print(controlNames[i]);

            tft.unloadFont();
        }
    }

    // Зберігаємо поточний активний індикатор
    lastActiveControl = activeControl;
}

void displayDrawSoundSettingsHeader() {
    const int barHeight = 25;
    const int barY = 0;

    // Малюємо фон панелі
    tft.fillRect(0, barY, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);

    // Малюємо напис "Налаштування звуку" по центру
    tft.setFreeFont(NULL);
    tft.loadFont("Arsenal-Bold15");
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);

    const char* headerText = "Sound Settings";
    int textX = 5;
    int textY = barY + (barHeight - 9) / 2;
    tft.setCursor(textX, textY);
    tft.print(headerText);

    // Лінія розділювач знизу
    tft.drawLine(0, barHeight, DISPLAY_WIDTH, barHeight, COLOR_WHITE);

    // Відновлюємо колір тексту та основний шрифт
    tft.unloadFont();
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.loadFont("Arsenal-Bold26");
}

void displayDrawStationListHeader(int page, int stationsPerPage, int count) {
    const int barHeight = 25;
    const int barY = 0;

    // Малюємо фон панелі
    tft.fillRect(0, barY, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);

    // Малюємо напис "Список станцій" зліва
    tft.setFreeFont(NULL);
    tft.loadFont("Arsenal-Bold15");
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);

    const char* headerText = "Station List";
    int textX = 5;
    int textY = barY + (barHeight - 9) / 2;
    tft.setCursor(textX, textY);
    tft.print(headerText);

    // Виводимо номер сторінки і діапазон станцій справа
    int firstIndex = page * stationsPerPage;
    int lastIndexDisplay = min(firstIndex + stationsPerPage, count) - 1;
    char pageInfo[48];
    snprintf(pageInfo, sizeof(pageInfo), "pg. %d (%d-%d of %d)", page + 1, firstIndex + 1, lastIndexDisplay + 1, count);
    tft.setTextColor(COLOR_LIGHTGRAY, COLOR_DARKGRAY);
    int pageInfoWidth = tft.textWidth(pageInfo);
    tft.setCursor(DISPLAY_WIDTH - pageInfoWidth - 5, textY);
    tft.print(pageInfo);

    // Лінія розділювач знизу
    tft.drawLine(0, barHeight, DISPLAY_WIDTH, barHeight, COLOR_WHITE);

    // Відновлюємо колір тексту та основний шрифт
    tft.unloadFont();
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.loadFont("Arsenal-Bold26");
}

void displayDrawAudioInputBar(TDA7318_Input activeInput) {
    // Назви входів
    const char* inputNames[] = {"WiFi", "Computer", "TV Box", "AUX"};
    const int numInputs = 4;

    // Параметри панелі
    const int barHeight = 20;
    const int barY = DISPLAY_HEIGHT - barHeight - 2;  // Відступ знизу 2 пікселі
    const int inputWidth = (DISPLAY_WIDTH - (numInputs + 1) * 2) / numInputs;  // Ширина кожного входу + відступи

    // Малюємо фон панелі
    tft.fillRect(0, barY, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);

    // Малюємо кожен вхід
    for (int i = 0; i < numInputs; i++) {
        int x = 2 + i * (inputWidth + 2);  // Відступ 2 пікселі між секціями

        // Визначаємо кольори
        uint16_t bgColor, textColor;
        if (i == activeInput) {
            bgColor = COLOR_GREEN;      // Активний вхід - зелений фон
            textColor = COLOR_BLACK;    // Чорний текст
        } else {
            bgColor = COLOR_GRAY;       // Неактивний - сірий фон
            textColor = COLOR_WHITE;    // Білий текст
        }

        // Малюємо прямокутник входу
        tft.fillRect(x, barY, inputWidth, barHeight, bgColor);

        // Малюємо рамку
        tft.drawRect(x, barY, inputWidth, barHeight, COLOR_WHITE);

        // Виводимо назву входу по центру (шрифт Arsenal-Bold15)
        tft.loadFont("Arsenal-Bold15");
        tft.setTextColor(textColor, bgColor);

        // Отримуємо ширину тексту для центрування
        int textWidth = tft.textWidth(inputNames[i]);
        int textX = x + (inputWidth - textWidth) / 2;
        int textY = barY + (barHeight - 14) / 2;  // Центрування по вертикалі

        tft.setCursor(textX, textY);
        tft.print(inputNames[i]);

        tft.unloadFont();  // Вивантажити шрифт
    }

    // Відновлюємо основний шрифт
    tft.loadFont("Arsenal-Bold26");
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
}

void displayUpdateAudioInputBar(TDA7318_Input activeInput) {
    // Зберігаємо попередній активний вхід (глобальна змінна)
    static TDA7318_Input lastActiveInput = INPUT_WIFI_RADIO;

    // Якщо вхід не змінився - нічого не робимо
    if (activeInput == lastActiveInput) {
        return;
    }

    // Назви входів
    const char* inputNames[] = {"WiFi", "Computer", "TV Box", "AUX"};
    const int numInputs = 4;

    // Параметри панелі
    const int barHeight = 20;
    const int barY = DISPLAY_HEIGHT - barHeight - 2;
    const int inputWidth = (DISPLAY_WIDTH - (numInputs + 1) * 2) / numInputs;

    // Оновлюємо тільки попередній активний вхід (тепер неактивний)
    int prevIndex = (int)lastActiveInput;
    int prevX = 2 + prevIndex * (inputWidth + 2);

    // Очищаємо і малюємо неактивну кнопку
    tft.fillRect(prevX + 1, barY + 1, inputWidth - 2, barHeight - 2, COLOR_GRAY);
    tft.loadFont("Arsenal-Bold15");
    tft.setTextColor(COLOR_WHITE, COLOR_GRAY);
    int prevTextWidth = tft.textWidth(inputNames[prevIndex]);
    int prevTextX = prevX + (inputWidth - prevTextWidth) / 2;
    int prevTextY = barY + (barHeight - 14) / 2;
    tft.setCursor(prevTextX, prevTextY);
    tft.print(inputNames[prevIndex]);
    tft.unloadFont();

    // Оновлюємо тільки новий активний вхід
    int newIndex = (int)activeInput;
    int newX = 2 + newIndex * (inputWidth + 2);

    // Очищаємо і малюємо активну кнопку
    tft.fillRect(newX + 1, barY + 1, inputWidth - 2, barHeight - 2, COLOR_GREEN);
    tft.loadFont("Arsenal-Bold15");
    tft.setTextColor(COLOR_BLACK, COLOR_GREEN);
    int newTextWidth = tft.textWidth(inputNames[newIndex]);
    int newTextX = newX + (inputWidth - newTextWidth) / 2;
    int newTextY = barY + (barHeight - 14) / 2;
    tft.setCursor(newTextX, newTextY);
    tft.print(inputNames[newIndex]);
    tft.unloadFont();

    // Відновлюємо основний шрифт
    tft.loadFont("Arsenal-Bold26");
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);

    // Зберігаємо поточний вхід для наступного разу
    lastActiveInput = activeInput;
}

void displayDrawOfflineInputBar(TDA7318_Input activeInput) {
    // Назви входів (тільки 3 без WiFi)
    const char* inputNames[] = {"Computer", "TV Box", "AUX"};
    const int numInputs = 3;

    // Корегуємо вхід якщо це WiFi
    TDA7318_Input displayInput = activeInput;
    if (displayInput == INPUT_WIFI_RADIO) {
        displayInput = INPUT_COMPUTER;
    } else if (displayInput > INPUT_AUX) {
        displayInput = INPUT_COMPUTER;
    }

    // Зміщуємо індекс на 1 назад (без WiFi)
    int displayIndex = (int)displayInput - 1;
    if (displayIndex < 0) displayIndex = 0;
    if (displayIndex >= numInputs) displayIndex = numInputs - 1;

    // Параметри панелі
    const int barHeight = 20;
    const int barY = DISPLAY_HEIGHT - barHeight - 2;
    const int inputWidth = (DISPLAY_WIDTH - (numInputs + 1) * 2) / numInputs;

    // Малюємо фон панелі
    tft.fillRect(0, barY, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);

    // Малюємо кожен вхід
    for (int i = 0; i < numInputs; i++) {
        int x = 2 + i * (inputWidth + 2);

        // Визначаємо кольори
        uint16_t bgColor, textColor;
        if (i == displayIndex) {
            bgColor = COLOR_GREEN;
            textColor = COLOR_BLACK;
        } else {
            bgColor = COLOR_GRAY;
            textColor = COLOR_WHITE;
        }

        // Малюємо прямокутник входу
        tft.fillRect(x, barY, inputWidth, barHeight, bgColor);

        // Малюємо рамку
        tft.drawRect(x, barY, inputWidth, barHeight, COLOR_WHITE);

        // Виводимо назву входу по центру
        tft.loadFont("Arsenal-Bold15");
        tft.setTextColor(textColor, bgColor);

        int textWidth = tft.textWidth(inputNames[i]);
        int textX = x + (inputWidth - textWidth) / 2;
        int textY = barY + (barHeight - 14) / 2;

        tft.setCursor(textX, textY);
        tft.print(inputNames[i]);

        tft.unloadFont();
    }

    tft.loadFont("Arsenal-Bold26");
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
}

void displayUpdateOfflineInputBar(TDA7318_Input activeInput) {
    static TDA7318_Input lastActiveInput = INPUT_COMPUTER;

    // Корегуємо вхід якщо це WiFi
    TDA7318_Input displayInput = activeInput;
    if (displayInput == INPUT_WIFI_RADIO) {
        displayInput = INPUT_COMPUTER;
    } else if (displayInput > INPUT_AUX) {
        displayInput = INPUT_COMPUTER;
    }

    if (displayInput == lastActiveInput) {
        return;
    }

    const char* inputNames[] = {"Computer", "TV Box", "AUX"};
    TDA7318_Input inputValues[] = {INPUT_COMPUTER, INPUT_TV_BOX, INPUT_AUX};
    const int numInputs = 3;
    const int barHeight = 20;
    const int barY = DISPLAY_HEIGHT - barHeight - 2;
    const int inputWidth = (DISPLAY_WIDTH - (numInputs + 1) * 2) / numInputs;

    // Перемальовуємо весь бар з нуля
    for (int i = 0; i < numInputs; i++) {
        int x = 2 + i * (inputWidth + 2);
        bool isActive = (inputValues[i] == displayInput);

        // Фон кнопки
        tft.fillRect(x + 1, barY + 1, inputWidth - 2, barHeight - 2, isActive ? COLOR_GREEN : COLOR_GRAY);

        // Текст кнопки
        tft.loadFont("Arsenal-Bold15");
        tft.setTextColor(isActive ? COLOR_BLACK : COLOR_WHITE, isActive ? COLOR_GREEN : COLOR_GRAY);
        int textWidth = tft.textWidth(inputNames[i]);
        tft.setCursor(x + (inputWidth - textWidth) / 2, barY + (barHeight - 14) / 2);
        tft.print(inputNames[i]);
        tft.unloadFont();
    }

    tft.loadFont("Arsenal-Bold26");
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);

    lastActiveInput = displayInput;
}

void displayDrawTopBar(const char* ssid, const char* ip) {
    const int barHeight = 25;
    const int barY = 0;
    
    // Малюємо фон панелі
    tft.fillRect(0, barY, DISPLAY_WIDTH, barHeight, COLOR_DARKGRAY);
    
    // Розділяємо на дві частини: зліва SSID, справа IP
    const int halfWidth = DISPLAY_WIDTH / 2;
    
    // Ліва частина - WiFi SSID
    tft.fillRect(0, barY, halfWidth, barHeight, COLOR_GRAY);
    tft.setFreeFont(NULL);
    tft.loadFont("Arsenal-Bold15");
    tft.setTextColor(COLOR_WHITE, COLOR_GRAY);
    
    if (ssid && ssid[0] != '\0') {
        int textWidth = tft.textWidth(ssid);
        int textX = 5;
        int textY = barY + (barHeight - 9) / 2;
        tft.setCursor(textX, textY);
        tft.print(ssid);
    } else {
        tft.setCursor(5, barY + (barHeight - 9) / 2);
        tft.print("No WiFi");
    }
    
    // Права частина - IP адреса
    tft.fillRect(halfWidth, barY, halfWidth, barHeight, COLOR_GRAY);
    tft.setTextColor(COLOR_WHITE, COLOR_GRAY);
    
    if (ip && ip[0] != '\0') {
        int textWidth = tft.textWidth(ip);
        int textX = halfWidth + (halfWidth - textWidth - 5);
        int textY = barY + (barHeight - 9) / 2;
        tft.setCursor(textX, textY);
        tft.print(ip);
    } else {
        tft.setCursor(halfWidth + 5, barY + (barHeight - 9) / 2);
        tft.print("No IP");
    }
    
    // Лінія розділювач знизу
    tft.drawLine(0, barHeight, DISPLAY_WIDTH, barHeight, COLOR_WHITE);
    
    // Відновлюємо колір тексту та основний шрифт
    tft.unloadFont();
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.loadFont("Arsenal-Bold26");
}

void displayShowStationList(const RadioStation* stations, int count, int selectedIndex, int page, int stationsPerPage) {
    // Очищаємо весь екран
    tft.fillScreen(COLOR_BLACK);

    // Малюємо верхню панель з написом "Список станцій"
    displayDrawStationListHeader(page, stationsPerPage, count);

    // Завантажуємо шрифт для списку
    tft.loadFont("Arsenal-Bold20");

    // Параметри списку - піднято максимально високо
    const int itemHeight = 28;  // Висота елемента
    const int listTop = 28;  // Піднято ще на 4px вище (щоб вмістилося 5 станцій)
    const int listWidth = DISPLAY_WIDTH - 10;
    const int listX = 5;
    const int textX = listX + 5;  // Зміщено текст ліворуч на 5

    // Обчислюємо перший і останній індекс на сторінці
    int firstIndex = page * stationsPerPage;
    int lastIndex = min(firstIndex + stationsPerPage, count);

    // Малюємо елементи списку на поточній сторінці
    for (int i = firstIndex; i < lastIndex; i++) {
        int y = listTop + (i - firstIndex) * itemHeight;
        bool isSelected = (i == selectedIndex);

        // Малюємо назву станції - білий текст для всіх
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);

        // Формуємо рядок з індексом та назвою
        char displayName[64];
        snprintf(displayName, sizeof(displayName), "%d. %s", i + 1, stations[i].name);

        const char* stationName = displayName;
        int textY = y + 4;  // Відступ по вертикалі для центрування

        tft.setCursor(textX, textY);

        // Обрізаємо якщо задовга
        int maxWidth = listWidth - 10;
        int textWidth = tft.textWidth(stationName);
        if (textWidth > maxWidth) {
            char truncatedName[32];
            strncpy(truncatedName, stationName, sizeof(truncatedName) - 1);
            truncatedName[sizeof(truncatedName) - 1] = '\0';

            int maxLen = strlen(truncatedName);
            int minLen = 0;
            while (minLen < maxLen) {
                int mid = (minLen + maxLen + 1) / 2;
                truncatedName[mid] = '\0';
                if (tft.textWidth(truncatedName) + tft.textWidth("...") <= maxWidth) {
                    minLen = mid;
                } else {
                    maxLen = mid - 1;
                }
            }
            truncatedName[minLen] = '\0';
            strcat(truncatedName, "...");
            tft.print(truncatedName);
        } else {
            tft.print(stationName);
        }

        // Малюємо рамку для вибраної станції (жовта)
        if (isSelected) {
            tft.drawRect(listX, y, listWidth, itemHeight, COLOR_YELLOW);
        }
    }
    
    // Вивантажуємо шрифт в кінці
    tft.unloadFont();
}

void displayUpdateStationListSelection(const RadioStation* stations, int count, int prevIndex, int newIndex, int page, int stationsPerPage) {
    // Параметри списку - ті самі що в displayShowStationList
    const int itemHeight = 28;
    const int listTop = 28;  // Піднято ще на 4px вище (щоб вмістилося 5 станцій)
    const int listWidth = DISPLAY_WIDTH - 10;
    const int listX = 5;

    // Перевіряємо чи обидва індекси на поточній сторінці
    int firstIndex = page * stationsPerPage;
    int lastIndex = min(firstIndex + stationsPerPage, count);

    // Завантажуємо шрифт ОДИН раз
    tft.loadFont("Arsenal-Bold20");

    // Крок 1: Стираємо жовту рамку малюючи чорну рамку
    if (prevIndex >= firstIndex && prevIndex < lastIndex) {
        int y = listTop + (prevIndex - firstIndex) * itemHeight;
        tft.drawRect(listX, y, listWidth, itemHeight, COLOR_BLACK);
    }

    // Крок 2: Малюємо жовту рамку на новому місці
    if (newIndex >= firstIndex && newIndex < lastIndex) {
        int y = listTop + (newIndex - firstIndex) * itemHeight;
        tft.drawRect(listX, y, listWidth, itemHeight, COLOR_YELLOW);
    }

    // Вивантажуємо шрифт
    tft.unloadFont();
}

void displayDrawBmpCenter(const char* filename) {
    // Відкриваємо BMP файл з SPIFFS
    File bmpFile = SPIFFS.open(filename, "r");
    if (!bmpFile) {
        Serial.printf("[Display] BMP file not found: %s\n", filename);
        return;
    }

    // Читаємо заголовок BMP файлу
    // BMP header: 14 bytes file header + 40 bytes DIB header
    if (bmpFile.read() != 'B' || bmpFile.read() != 'M') {
        Serial.println("[Display] Not a valid BMP file");
        bmpFile.close();
        return;
    }

    // Пропускаємо розмір файлу (4 bytes) та зарезервовані поля (4 bytes)
    bmpFile.seek(10);
    uint32_t dataOffset = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

    // Читаємо ширину та висоту
    bmpFile.seek(18);
    int32_t width = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);
    int32_t height = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

    // Читаємо біти на піксель
    bmpFile.seek(28);
    uint16_t bitsPerPixel = bmpFile.read() | (bmpFile.read() << 8);

    if (bitsPerPixel != 24 && bitsPerPixel != 16) {
        Serial.printf("[Display] Unsupported BMP bits per pixel: %d\n", bitsPerPixel);
        bmpFile.close();
        return;
    }

    // Обчислюємо позицію для центрування
    int x = (DISPLAY_WIDTH - width) / 2;
    int y = (DISPLAY_HEIGHT - height) / 2;

    // Обмежуємо розміри якщо картинка більша за екран
    if (width > DISPLAY_WIDTH) width = DISPLAY_WIDTH;
    if (height > DISPLAY_HEIGHT) height = DISPLAY_HEIGHT;

    // Виділяємо буфер для рядка пікселів
    uint16_t *lineBuffer = (uint16_t *)malloc(width * 2);
    if (!lineBuffer) {
        Serial.println("[Display] Failed to allocate line buffer");
        bmpFile.close();
        return;
    }

    // BMP зберігається знизу вгору, читаємо по рядках
    int rowSize = ((bitsPerPixel * width + 31) / 32) * 4;  // Рядок вирівнюється до 4 байт
    uint8_t *rowBuffer = (uint8_t *)malloc(rowSize);
    if (!rowBuffer) {
        free(lineBuffer);
        bmpFile.close();
        Serial.println("[Display] Failed to allocate row buffer");
        return;
    }

    // Починаємо читати дані зображення
    bmpFile.seek(dataOffset);

    // BMP зберігається знизу вгору: row 0 = нижній рядок зображення
    // Віддзеркалюємо по вертикалі: читаємо рядки у зворотному порядку
    for (int row = 0; row < height; row++) {
        bmpFile.read(rowBuffer, rowSize);

        int screenY = y + (height - 1 - row);  // Віддзеркалення по вертикалі
        if (screenY < 0 || screenY >= DISPLAY_HEIGHT) continue;

        for (int col = 0; col < width; col++) {
            if (bitsPerPixel == 24) {
                // 24-bit BMP: BGR порядок байтів
                uint8_t b = rowBuffer[col * 3];
                uint8_t g = rowBuffer[col * 3 + 1];
                uint8_t r = rowBuffer[col * 3 + 2];
                // Конвертуємо в 16-bit RGB565
                lineBuffer[col] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            } else {
                // 16-bit BMP: RGB565 порядок (little-endian)
                uint8_t lo = rowBuffer[col * 2];
                uint8_t hi = rowBuffer[col * 2 + 1];
                lineBuffer[col] = (hi << 8) | lo;
            }
        }

        // Малюємо рядок на дисплеї
        tft.setSwapBytes(true);
        tft.pushImage(x, screenY, width, 1, lineBuffer);
    }

    free(lineBuffer);
    free(rowBuffer);
    bmpFile.close();

    tft.setSwapBytes(true);
    Serial.printf("[Display] BMP displayed: %s (%dx%d)\n", filename, width, height);
}

void displayUpdate() {
    // TFT_eSPI малює безпосередньо на дисплей, буфер не потрібен
    // Функція залишена для сумісності інтерфейсу
}

TFT_eSPI& displayGetTFT() {
    return tft;
}
