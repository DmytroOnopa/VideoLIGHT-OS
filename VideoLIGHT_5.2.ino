#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN 7
#define LED_COUNT 10
CRGB leds[LED_COUNT];

#define SELECT_PIN 6
#define NEXT_PIN 5

// EEPROM адреси
#define EEPROM_BRIGHTNESS 0
#define EEPROM_RED 1
#define EEPROM_GREEN 2
#define EEPROM_BLUE 3
#define EEPROM_EFFECT 4
#define EEPROM_INVERT 5

enum State { MENU, ADJUST, EFFECT, ABOUT, SPACEINVADERS, SPACEINVADERS_GAMEOVER, SCREENSAVER };
State state = MENU;

int menuScrollOffset = 0;
int currentMenu = 0;
int menuTopIndex = 0; // Перший видимий пункт меню
const int visibleMenuItems = 4; // Кількість пунктів на екрані

const char* mainMenu[] = {
  "Brightness",
  "Red",
  "Green",
  "Blue",
  "Effect",
  "Rotate Display",
  "Light/Dark mode",
  "Game",
  "About"
};
#define MENU_COUNT 9

bool invertDisplay = false;
int brightness = 128;
CRGB currentColor = CRGB::White;

int effectIndex = 0;
#define EFFECT_COUNT 7  // Кількість ефектів

// Таймер бездіяльності
unsigned long lastActivityTime = 0;
const unsigned long screensaverTimeout = 30000; // 30 секунд

// === Space Invaders змінні ===
const int PLAYER_WIDTH = 8;
const int PLAYER_HEIGHT = 4;
int playerX = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
int playerY = SCREEN_HEIGHT - PLAYER_HEIGHT - 2;
int lives = 3;
unsigned long lastShotTime = 0;
const unsigned long shotInterval = 400; // мс між пострілами

struct Bullet {
  int x, y;
  bool active;
};
Bullet bullet;

const int INVADER_ROWS = 3;
const int INVADER_COLS = 6;
bool invaders[INVADER_ROWS][INVADER_COLS];
int invaderX = 0;
int invaderY = 10;
int invaderDir = 1; // 1 = вправо, -1 = вліво
unsigned long lastMoveTime = 0;
const unsigned long invaderMoveInterval = 500; // кожні 500мс
int invaderStepDowns = 0;
bool gameOver = false;
int gameOverMenuIndex = 0;

bool rotateDisplay180 = false; // false — 0°, true — 180°

String aboutText = 
  "VideoLIGHT OS v5.2\n"
  "Current firmware:\n"
  "github.com/DmytroOnopa/VideoLIGHT-OS\n"
  "―\n"
  "My contact:\n"
  "ABTOMATAK.t.me\n"
  "https://geniusbar.site\n";

void setup() {
  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(NEXT_PIN, INPUT_PULLUP);

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  
  // Завантаження налаштувань з EEPROM
  brightness = EEPROM.read(EEPROM_BRIGHTNESS);
  if (brightness < 16 || brightness > 255) brightness = 128; // захист від кривих даних
  // Читаємо збережені кольори з EEPROM
  currentColor.r = EEPROM.read(EEPROM_RED);
  currentColor.g = EEPROM.read(EEPROM_GREEN);
  currentColor.b = EEPROM.read(EEPROM_BLUE);
  
  // Якщо значення не ініціалізовані (255), встановлюємо за замовчуванням
  if(currentColor.r == 255) currentColor.r = 255;
  if(currentColor.g == 255) currentColor.g = 255;
  if(currentColor.b == 255) currentColor.b = 255;
  effectIndex = EEPROM.read(EEPROM_EFFECT);
  if (effectIndex >= EFFECT_COUNT) effectIndex = 0;
  invertDisplay = EEPROM.read(EEPROM_INVERT);

  FastLED.setBrightness(brightness);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1); // якщо OLED не стартує - застрягаємо тут
  }

  display.invertDisplay(invertDisplay);
  showLoadingAnimation();
  drawMainMenu();
  lastActivityTime = millis();
}

void loop() {
  // Таймер бездіяльності
  if (digitalRead(SELECT_PIN) && digitalRead(NEXT_PIN)) {
    if (state != SCREENSAVER && millis() - lastActivityTime > screensaverTimeout) {
      state = SCREENSAVER;
      display.clearDisplay();
      display.display();
    }
  } else {
    lastActivityTime = millis();
    if (state == SCREENSAVER) {
      state = MENU;
      drawMainMenu();
    }
  }

  switch (state) {
   case MENU: handleMenu(); break;
   case ADJUST: handleAdjust(); break;
   case ABOUT: handleAbout(); break;
   case SPACEINVADERS: spaceInvadersUpdate(); break;
   case SPACEINVADERS_GAMEOVER: spaceInvadersGameOverUpdate(); break;  //!
   case SCREENSAVER: screensaverUpdate(); break;
  }

  updateLEDs();
  FastLED.show();
  delay(30);
}

void drawMainMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  // Заголовок
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("VideoLIGHT Settings:"));
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  // Відображаємо видимі пункти меню
  for (int i = 0; i < visibleMenuItems; i++) {
    int itemIndex = menuTopIndex + i;
    if (itemIndex >= MENU_COUNT) break;
    
    display.setCursor(0, 12 + i*10);
    if (itemIndex == currentMenu) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    
    display.print(mainMenu[itemIndex]);
    
  }

  // Індикатор прокрутки (якщо меню довше ніж видима область)
  if (MENU_COUNT > visibleMenuItems) {
    int scrollbarHeight = visibleMenuItems * 10 - 2;
    int scrollbarPos = map(currentMenu, 0, MENU_COUNT-1, 12, 12 + scrollbarHeight - 4);
    display.drawLine(SCREEN_WIDTH-4, 12, SCREEN_WIDTH-4, 12 + scrollbarHeight, SSD1306_WHITE);
    display.fillRect(SCREEN_WIDTH-3, scrollbarPos, 3, 3, SSD1306_WHITE);
  }
  
    display.setTextColor(SSD1306_WHITE); // <-- оце обов'язково
    display.drawLine(0, 52, SCREEN_WIDTH, 52, SSD1306_WHITE);
    display.setCursor(19, 54);
    display.print(F("SELECT | CHANGE"));

  display.display();
}

void drawAdjustMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 2);
  display.print(F("Edit: "));
  display.println(mainMenu[currentMenu]);
  display.drawLine(0, 12, SCREEN_WIDTH, 12, SSD1306_WHITE);

  display.setCursor(0, 20);
  
  switch (currentMenu) {
    case 0: // Brightness
      display.print(F("Brightness: "));
      display.println(brightness);
      break;
    case 1: // Red
      display.print(F("Red: "));
      display.println(currentColor.r);
      break;
    case 2: // Green
      display.print(F("Green: "));
      display.println(currentColor.g);
      break;
    case 3: // Blue
      display.print(F("Blue: "));
      display.println(currentColor.b);
      break;
    case 4: // Effect
      display.print(F("Effect: "));
      display.println(effectName(effectIndex));
      break;
    case 5: // Invert
      display.print(F("Enable light mode: "));
      display.println(invertDisplay ? "Yes" : "No");
      break;
  }

  // Нижній текст з інструкцією
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(0, 52, SCREEN_WIDTH, 52, SSD1306_WHITE);
  display.setCursor(0, 54);
  display.print(F("Sel:Back Next:Change"));
  
  display.display();
}

const char* effectName(int idx) {
  switch (idx) {
    case 0: return "Static";
    case 1: return "Running Dot";
    case 2: return "Confetti";
    case 3: return "Staboscope";
    case 4: return "SOS Signal";
    case 5: return "Hazard Light";
    case 6: return "Police Mode";
    default: return "Unknown";
  }
}

void handleMenu() {
  if (!digitalRead(NEXT_PIN)) {
    currentMenu = (currentMenu + 1) % MENU_COUNT;

    // Прокрутка меню
    if (currentMenu >= menuTopIndex + visibleMenuItems) {
      menuTopIndex = currentMenu - visibleMenuItems + 1;
    } else if (currentMenu < menuTopIndex) {
      menuTopIndex = currentMenu;
    }

    drawMainMenu();
    delay(200);
  }

  if (!digitalRead(SELECT_PIN)) {
    switch (currentMenu) {
      case 0: case 1: case 2: case 3:  // Brightness, R, G, B
        state = ADJUST;
        drawAdjustMenu();
        break;
      case 4:  // Effect
        state = ADJUST;
        drawAdjustMenu();
        break;
      case 5:  // Rotate Display
        state = ADJUST;
        drawAdjustMenu();
        break;
      case 6:  // Invert Display
         state = ADJUST;
         drawAdjustMenu();
         break;
      case 7:  // Game
        state = SPACEINVADERS;
        spaceInvadersInit();
        break;
      case 8:  // About
        state = ABOUT;
        drawAbout();
        break;
    }
    delay(200);
  }
}

void handleAdjust() {
  static unsigned long lastAdjustTime = 0;
  static bool wasPressed = false;
  static int repeatDelay = 300; // Початковий інтервал

  bool nextPressed = digitalRead(NEXT_PIN) == LOW;
  bool selectPressed = digitalRead(SELECT_PIN) == LOW;

  unsigned long now = millis();

  // Обробка виходу через SELECT
  if (selectPressed) {
    state = MENU;
    drawMainMenu();
    delay(200); // захист від дрібежу
    return;
  }

  // Обробка кнопки NEXT з утриманням
  if (nextPressed) {
    if (!wasPressed || (now - lastAdjustTime >= repeatDelay)) {
      switch (currentMenu) {
        case 0:
          brightness += 2;
          if (brightness > 255) brightness = 16;
          FastLED.setBrightness(brightness);
          break;
        case 1:
          currentColor.r = (currentColor.r + 1) % 256;
          break;
        case 2:
          currentColor.g = (currentColor.g + 1) % 256;
          break;
        case 3:
          currentColor.b = (currentColor.b + 1) % 256;
          break;
        case 4:
          effectIndex = (effectIndex + 1) % EFFECT_COUNT;
          break;
        case 5: // Rotate Display
          rotateDisplay180 = !rotateDisplay180;
          display.setRotation(rotateDisplay180 ? 2 : 0);  // 2 — це 180 градусів у Adafruit_SSD1306
          break;
        case 6: // Invert Display
          invertDisplay = !invertDisplay;
          display.invertDisplay(invertDisplay);
          EEPROM.update(EEPROM_INVERT, invertDisplay);
          break;
      }

      saveSettings();
      drawAdjustMenu();

      lastAdjustTime = now;
      wasPressed = true;

      // Зменшення затримки, щоб прискорити зміну
      if (repeatDelay > 50) repeatDelay -= 40;
    }
  } else {
    // Кнопка відпущена — ресет
    wasPressed = false;
    repeatDelay = 200;
  }
}

int scrollPosY = SCREEN_HEIGHT;  // починаємо знизу
unsigned long lastScrollTime = 0;
const int scrollInterval = 50;
const int scrollSpeed = 1;

int countLines(const String& str) {
  int count = 1;
  for (int i = 0; i < str.length(); i++) {
    if (str.charAt(i) == '\n') count++;
  }
  return count;
}

void drawAbout() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int lineHeight = 8;

  int y = scrollPosY;

  int start = 0;
  int end = aboutText.indexOf('\n', start);
  while (end != -1) {
    String line = aboutText.substring(start, end);
    if (y >= 0 && (y + lineHeight) <= SCREEN_HEIGHT - 10) {
     display.setCursor(0, y);
      display.print(line);
    }
    y += lineHeight;
    start = end + 1;
    end = aboutText.indexOf('\n', start);
  }
// останній рядок
  String lastLine = aboutText.substring(start);
  if (y >= 0 && (y + lineHeight) <= SCREEN_HEIGHT - 10) {
   display.setCursor(0, y);
   display.print(lastLine);
  }

  // підказка внизу
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(0, 52, SCREEN_WIDTH, 52, SSD1306_WHITE);
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.println(F("Press SELECT to back"));

  display.display();

  if (millis() - lastScrollTime > scrollInterval) {
    scrollPosY -= scrollSpeed;

    int totalHeight = countLines(aboutText) * lineHeight;

    if (scrollPosY < -totalHeight) {
      scrollPosY = SCREEN_HEIGHT;
    }
    lastScrollTime = millis();
  }
}

void handleAbout() {
  if (!digitalRead(SELECT_PIN)) {
    scrollPosY = SCREEN_HEIGHT;  // або 0 — залежно від логіки, краще починати знизу
    state = MENU;
    drawMainMenu();
    delay(200);
  } else {
    drawAbout(); // постійно малюємо About з прокруткою
  }
}

void saveSettings() {
  EEPROM.update(EEPROM_BRIGHTNESS, brightness);
  EEPROM.update(EEPROM_RED, currentColor.r);
  EEPROM.update(EEPROM_GREEN, currentColor.g);
  EEPROM.update(EEPROM_BLUE, currentColor.b);
  EEPROM.update(EEPROM_EFFECT, effectIndex);
  EEPROM.update(EEPROM_INVERT, invertDisplay);
}

void showLoadingAnimation() {
  const char* text = "theGrove";
  int len = strlen(text);
  for (int frame = 0; frame < 40; frame++) {
    display.clearDisplay();
    // Рухомі смужки
    for (int y = 0; y < SCREEN_HEIGHT; y += 6) {
      display.drawFastHLine(0, (y + frame * 3) % SCREEN_HEIGHT, SCREEN_WIDTH, SSD1306_WHITE);
    }
    // Текст з "гліч" ефектом
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    for (int i = 0; i < len; i++) {
      int x = (SCREEN_WIDTH - len * 12) / 2 + i * 12;
      int y = SCREEN_HEIGHT / 2 - 8;
      if (random(10) > 7 || frame > 30) {
        display.setCursor(x, y);
        display.print(text[i]);
      } else {
        display.setCursor(x, y + random(-2,3));
        display.print((char)random(33, 126));
      }
    }
    display.display();
    delay(40);
  }
  delay(600);
}

void updateLEDs() {
  switch (effectIndex) {
    case 0: effectStatic(); break;
    case 1: effectRunningDot(); break;
    case 2: effectConfetti(); break;
    case 3: effectStaboscope(); break;
    case 4: effectSOS(); break;
    case 5: effectHazard(); break;
    case 6: effectPolice(); break;
    default: effectStatic(); break;
  }
}

void effectStatic() {
  fill_solid(leds, LED_COUNT, CRGB(currentColor.r, currentColor.g, currentColor.b));
}

// === Running Dot ===
void effectRunningDot() {
  static int pos = 0;
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate > 50) {
    lastUpdate = millis();
    fadeToBlackBy(leds, LED_COUNT, 50);
    leds[pos] = CRGB(currentColor.r, currentColor.g, currentColor.b);
    pos = (pos + 1) % LED_COUNT;
  }
}

// === Confetti ===
void effectConfetti() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 30) {
    lastUpdate = millis();
    fadeToBlackBy(leds, LED_COUNT, 10);
    int pos = random8() % LED_COUNT;
    leds[pos] = CHSV(random8(), 255, 255);
  }
}

// === Staboscope ===
void effectStaboscope() {
  static unsigned long lastFlash = 0;
  static int flashCount = 0;
  static bool flashing = false;

  if (!flashing) {
    if (millis() - lastFlash > 15000) {
      flashing = true;
      flashCount = 0;
    }
    fill_solid(leds, LED_COUNT, CRGB::Black);
  } else {
    if (millis() - lastFlash > 80) {
      if (flashCount % 2 == 0) {
        fill_solid(leds, LED_COUNT, CRGB(255, 255, 255));
        FastLED.setBrightness(255);
      } else {
        fill_solid(leds, LED_COUNT, CRGB::Black);
      }
      flashCount++;
      lastFlash = millis();

      if (flashCount >= 18) {
        flashing = false;
      }
    }
  }
}

void effectSOS() {
  static unsigned long t = 0;
  static int s = 0;
  static bool on = false;
  const int seq[] = {200,200,200,200,200,200,600,600,600,600,600,600,200,200,200};
  if (millis() - t < (on ? seq[s] : 200)) return;
  t = millis();
  on = !on;
  if (on) fill_solid(leds, LED_COUNT, CRGB(255,80,0));
  else {
    fill_solid(leds, LED_COUNT, CRGB::Black);
    s = (s + 1) % 15;
  }
  FastLED.show();
}

void effectHazard() {
  static unsigned long lastToggle = 0;
  static bool state = false;

  if (millis() - lastToggle > 500) {
    lastToggle = millis();
    state = !state;
    CRGB color = state ? CRGB(255, 60, 0) : CRGB::Black;

    leds[0] = leds[9] = leds[4] = leds[5] = color;
    // Інші вимикаємо
    for (int i = 0; i < LED_COUNT; i++) {
      if (i != 0 && i != 4 && i != 5 && i != 9) {
        leds[i] = CRGB::Black;
      }
    }
  }
}

void effectPolice() {
  static unsigned long lastUpdate = 0;
  static int blinkCount = 0;
  static bool leftSide = true;
  static bool isOn = false;

  unsigned long now = millis();
  const int blinkInterval = 100;
  const int pauseBetweenSides = 300;

  if (now - lastUpdate >= blinkInterval) {
    lastUpdate = now;

    // Вимикаємо всі
    fill_solid(leds, LED_COUNT, CRGB::Black);

    if (isOn) {
      // моргаємо
      if (leftSide) {
        // Ліва сторона: 0, 1, 9, 8, 7
        leds[0] = leds[1] = leds[9] = leds[8] = leds[7] = CRGB(0, 0, 255); // синій
      } else {
        // Права сторона: 2, 3, 4, 5, 6
        leds[2] = leds[3] = leds[4] = leds[5] = leds[6] = CRGB(255, 0, 0); // червоний
      }
      blinkCount++;
    } else {
      // Пауза між миготінням
    }

    FastLED.show();
    isOn = !isOn;

    if (!isOn && blinkCount >= 3) {
      // 3 блимання завершено — міняємо сторону
      leftSide = !leftSide;
      blinkCount = 0;
      lastUpdate = now + pauseBetweenSides;
    }
  }
}

void spaceInvadersInit() {
  playerX = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
  lives = 3;
  bullet.active = false;
  invaderX = 0;
  invaderY = 10;
  invaderDir = 1;
  invaderStepDowns = 0;
  gameOver = false;

  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      invaders[row][col] = true;
    }
  }
}

void spaceInvadersUpdate() {
  // --- Керування ---
  if (!digitalRead(SELECT_PIN)) {
    playerX = constrain(playerX - 2, 0, SCREEN_WIDTH - PLAYER_WIDTH);
  }
  if (!digitalRead(NEXT_PIN)) {
    playerX = constrain(playerX + 2, 0, SCREEN_WIDTH - PLAYER_WIDTH);
  }

  // --- Авто стрільба ---
  if (!bullet.active && millis() - lastShotTime > shotInterval) {
    bullet.x = playerX + PLAYER_WIDTH / 2;
    bullet.y = playerY - 1;
    bullet.active = true;
    lastShotTime = millis();
  }

  if (bullet.active) {
    bullet.y -= 3;
    if (bullet.y < 0) bullet.active = false;
  }

  // --- Рух ворогів ---
  if (millis() - lastMoveTime > invaderMoveInterval) {
    lastMoveTime = millis();
    invaderX += invaderDir * 5;

    // Перевірка країв
    if (invaderX + INVADER_COLS * 12 > SCREEN_WIDTH || invaderX < 0) {
      invaderDir *= -1;
      invaderY += 6;
      invaderStepDowns++;

      if (invaderStepDowns >= 3) {
        lives = 0;
        gameOver = true;
      }
    }
  }

  // --- Перевірка попадання ---
  if (bullet.active) {
    for (int row = 0; row < INVADER_ROWS; row++) {
      for (int col = 0; col < INVADER_COLS; col++) {
        if (invaders[row][col]) {
          int ix = invaderX + col * 12;
          int iy = invaderY + row * 8;

          if (bullet.x >= ix && bullet.x <= ix + 8 &&
              bullet.y >= iy && bullet.y <= iy + 5) {
            invaders[row][col] = false;
            bullet.active = false;
          }
        }
      }
    }
  }

  // --- Перевірка зіткнення гравця з ворогом ---
  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      if (invaders[row][col]) {
        int ix = invaderX + col * 12;
        int iy = invaderY + row * 8;
        if (iy + 5 >= playerY &&
            ix + 8 >= playerX &&
            ix <= playerX + PLAYER_WIDTH) {
          lives--;
          invaderStepDowns++;
          if (lives <= 0) gameOver = true;
          break;
        }
      }
    }
  }

  // --- Гейм овер ---
  if (gameOver) {
    state = SPACEINVADERS_GAMEOVER;
    drawGameOver(gameOverMenuIndex);
    return;
  }

  // --- Малювання ---
  display.clearDisplay();

  // Гравець
  display.fillRect(playerX, playerY, PLAYER_WIDTH, PLAYER_HEIGHT, SSD1306_WHITE);

  // Постріл
  if (bullet.active) {
    display.drawPixel(bullet.x, bullet.y, SSD1306_WHITE);
  }

  // Вороги
  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      if (invaders[row][col]) {
        int ix = invaderX + col * 12;
        int iy = invaderY + row * 8;
        display.drawRect(ix, iy, 8, 5, SSD1306_WHITE);
      }
    }
  }

  // Життя
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Lives: ");
  display.print(lives);

  display.display();
}

void drawGameOver(int gameOverMenuIndex) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.println("Game Over");

  display.setTextSize(1);

  const char* options[] = { "New Game", "Exit" };
  int yPositions[] = { 45, 55 };

  for (int i = 0; i < 2; i++) {
    if (i == gameOverMenuIndex) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(20, yPositions[i]);
    display.println(options[i]);
  }

  display.display();
}

void spaceInvadersGameOverUpdate() {
  if (!digitalRead(NEXT_PIN)) {
    gameOverMenuIndex = (gameOverMenuIndex + 1) % 2;
    drawGameOver(gameOverMenuIndex);
    delay(200);
  }
  
  if (!digitalRead(SELECT_PIN)) {
    if (gameOverMenuIndex == 0) {
      state = SPACEINVADERS;
      spaceInvadersInit();  // тут правильна ініціалізація
    } else {
      state = MENU;
      drawMainMenu();
    }
    delay(200);
  }
}

void drawGameOverSelection() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Game Over");
  display.setTextSize(1);
  
  if (gameOverMenuIndex == 0) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(10, 45);
    display.println("Restart");
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 55);
    display.println("Menu");
  } else {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 45);
    display.println("Restart");
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(10, 55);
    display.println("Menu");
  }
  
  display.display();
}

// ========== Screensaver ==========

void screensaverUpdate() {
  display.clearDisplay();

  // Заголовок по центру
  const char* title = "geniusbar.site/";
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 2);
  display.println(title);

  // Лінія під заголовком
  display.drawLine(0, 15, SCREEN_WIDTH, 15, SSD1306_WHITE);

  // Обʼєднана рамка для RGB + Brightness
  display.drawRect(0, 20, SCREEN_WIDTH, 38, SSD1306_WHITE);

  // RGB всередині рамки
  display.setCursor(5, 24);
  display.print("RGB: ");
  display.print(currentColor.r);
  display.print(",");
  display.print(currentColor.g);
  display.print(",");
  display.print(currentColor.b);

  // Brightness текст
  display.setCursor(5, 36);
  display.print("Brightness: ");
  display.print(brightness);
  display.print("/255");

  // Brightness bar
  int barWidth = map(brightness, 0, 255, 0, SCREEN_WIDTH - 10);
  display.drawRect(5, 48, SCREEN_WIDTH - 10, 5, SSD1306_WHITE);
  display.fillRect(5, 48, barWidth, 5, SSD1306_WHITE);

  display.display();
}
