# Інструкція з налаштування української мови для TFT_eSPI

## Крок 1: Створення шрифту з кирилицею

1. Встановіть Processing IDE: https://processing.org/download
2. Завантажте створення шрифтів з TFT_eSPI: `TFT_eSPI/Tools/Create_Smooth_Font/Create_Smooth_Font.pde`
3. Відкрийте Create_Smooth_Font.pde в Processing
4. У файлі виберіть шрифт (наприклад, "Final-Fixed-24")
5. Додайте символів кирилиці до списку:
   ```
   АБВГҐДЕЄЖЗИІЇЙКЛМНОПРСТУФХЦЧШЩЬЮЯ
   абвгґдеєжзиіїйклмнопрстуфхцчшщьюя
   ```
6. Збережіть шрифт як `Final-Fixed-24.vlw` у папку `data/fonts/`

## Крок 2: Додавання шрифту в SPIFFS

1. Створіть папку `data/fonts/` у проекті
2. Скопіюйте файл `Final-Fixed-24.vlw` у `data/fonts/`
3. Завантажте SPIFFS через PlatformIO:
   ```
   pio run -t uploadfs
   ```

## Крок 3: Використання шрифту

```cpp
// Завантаження шрифту з SPIFFS
tft.loadFont("Final-Fixed-24");

// Вивід тексту українською
tft.print("Привіт світ!");

// Вивантаження шрифту
tft.unloadFont();
```

## Альтернативний варіант: Використання вбудованих шрифтів

Якщо немає можливості створити Smooth Font, можна використати вбудовані шрифти:

```cpp
// Font 1, 2, 4, 6, 7, 8 мають базову підтримку символів
tft.setTextFont(2);  // Шрифт 16x16
tft.print("Text");  // Латиниця працює
```

**Примітка:** Вбудовані шрифти не підтримують кирилицю за замовчуванням. Для повної підтримки української мови потрібно створити Smooth Font.

## Макроси для platformio.ini

Вже додано в `platformio.ini`:
```ini
-D SMOOTH_FONT
-D LOAD_GFXFF
```

## Посилання

- TFT_eSPI документація: https://github.com/Bodmer/TFT_eSPI
- Smooth Font creation: https://github.com/Bodmer/TFT_eSPI/blob/master/docs/Smooth%20Font/README.md
