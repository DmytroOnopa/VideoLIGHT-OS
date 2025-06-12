# VideoLIGHT Controller 🎮💡

A versatile Arduino-based controller for LED strips with OLED display, featuring color control, effects, and a Space Invaders mini-game!

![Demo](https://via.placeholder.com/400x200.png?text=VideoLIGHT+Demo) *(placeholder image)*

## Features ✨

- **LED Control**:
  - Adjustable brightness 🌟
  - RGB color selection 🎨
  - Multiple effects (static, rainbow, color flow, running dot, confetti) 🌈
  - Invert display option 🔄

- **Built-in Game**:
  - Space Invaders mini-game 👾
  - Lives system ❤️
  - Game over screen 💀

- **OLED Interface**:
  - Scrolling menu 📜
  - Screensaver with status display ⏰
  - About section with scrolling text ℹ️

- **EEPROM Storage**:
  - Saves all settings between power cycles 💾

## Hardware Requirements 🛠️

- Arduino board (tested with Nano)
- SSD1306 OLED display (128x64)
- WS2812B/NeoPixel LED strip
- 2 push buttons
- 10kΩ resistors (for buttons)

## Installation 📥

1. Clone this repository
2. Open `VideoLIGHT_5.2.ino` in Arduino IDE
3. Install required libraries:
   - `Adafruit_SSD1306`
   - `FastLED`
   - `Wire`
   - `EEPROM`
4. Upload to your Arduino

## Wiring Guide 🔌

| Component  | Arduino Pin |
|------------|-------------|
| OLED SDA   | A4          |
| OLED SCL   | A5          |
| LED Strip  | 7           |
| SELECT btn | 6           |
| NEXT btn   | 5           |

## Usage 🕹️

1. **Main Menu Navigation**:
   - `NEXT` button cycles through options
   - `SELECT` enters submenus

2. **Adjustment Mode**:
   - `NEXT` changes values
   - `SELECT` returns to main menu

3. **Space Invaders Game**:
   - `SELECT` moves left
   - `NEXT` moves right
   - Automatic shooting

## Customization 🛠️

- Edit `mainMenu[]` array to change menu items
- Add new effects in `updateLEDs()` function
- Adjust `screensaverTimeout` for inactivity delay

## Demo Video 🎥

[Watch on YouTube](https://youtu.be/your-video)

## License 📄

MIT License - Free to use and modify!

---

Made with ❤️ by [DmytroOnopa, DeepSeek, OpenAI] | [DmytroOnopa] | [https://geniusbar.site/]