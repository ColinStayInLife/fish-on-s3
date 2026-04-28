#include "DisplayManager.h"
#include <Arduino.h>

// M5StickCPlus color shortcuts
#define COLOR_BG     BLACK
#define COLOR_GREEN  TFT_GREEN
#define COLOR_YELLOW TFT_YELLOW
#define COLOR_RED    TFT_RED
#define COLOR_GRAY   0x8410    // dark gray
#define COLOR_WHITE  TFT_WHITE
#define COLOR_BLUE   TFT_BLUE

DisplayManager::DisplayManager() {}
DisplayManager::~DisplayManager() {}

void DisplayManager::init() {
    M5.Lcd.setRotation(3);  // USB port on left, screen upright
    M5.Lcd.fillScreen(COLOR_BG);
    m_initialized = true;
}

void DisplayManager::showSplash(const char *title, const char *subtitle) {
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_WHITE, COLOR_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.println(title);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.print("Mode: ");
    M5.Lcd.println(subtitle);
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.println("Starting...");
}

void DisplayManager::updateSingleRod(float magnitude) {
    unsigned long now = millis();
    if (now - m_lastScreenUpdate < SCREEN_REFRESH_MS) return;
    m_lastScreenUpdate = now;

    // Clear only the data area (not full screen for performance)
    M5.Lcd.fillRect(0, 20, 135, 220, COLOR_BG);

    // Top bar
    drawStatusBar("Single");

    // Large magnitude display
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(COLOR_WHITE, COLOR_BG);
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.print("Amplitude:");

    // Color-code by severity
    uint16_t ampColor = COLOR_GREEN;
    if (magnitude > 3.5f)  ampColor = COLOR_RED;
    else if (magnitude > 1.2f) ampColor = COLOR_RED;
    else if (magnitude > 0.3f) ampColor = COLOR_YELLOW;

    M5.Lcd.setTextColor(ampColor, COLOR_BG);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(10, 65);
    M5.Lcd.print(magnitude, 2);
    M5.Lcd.print("g");

    // Simple waveform bar (horizontal moving graph)
    static int xPos = 0;
    int barHeight = (int)(magnitude * 20);  // scale
    if (barHeight > 100) barHeight = 100;
    M5.Lcd.fillRect(xPos, 180 - barHeight, 2, barHeight, ampColor);
    // Erase trailing
    M5.Lcd.fillRect(xPos + 2, 80, 2, 100, COLOR_BG);
    xPos = (xPos + 3) % 135;
}

void DisplayManager::updateMultiRod(const RodUiState rods[MAX_RODS]) {
    unsigned long now = millis();
    if (now - m_lastScreenUpdate < SCREEN_REFRESH_MS) return;
    m_lastScreenUpdate = now;

    M5.Lcd.fillRect(0, 16, 135, 224, COLOR_BG);

    drawStatusBar("Multi");

    // Show up to 8 rods in a 2-column, 4-row grid
    for (int i = 0; i < MAX_RODS; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 5 + col * 65;
        int y = 24 + row * 50;

        if (!rods[i].online) {
            // Offline rod — show ghost
            M5.Lcd.setTextColor(COLOR_GRAY, COLOR_BG);
            M5.Lcd.setCursor(x, y);
            M5.Lcd.print("R");
            M5.Lcd.print(i + 1);
            M5.Lcd.setTextSize(1);
            M5.Lcd.setCursor(x, y + 14);
            M5.Lcd.print("OFF");
            continue;
        }

        // Online rod
        uint16_t c = stateColor(rods[i].state);
        M5.Lcd.setTextColor(c, COLOR_BG);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(x, y);
        M5.Lcd.print("R");
        M5.Lcd.print(i + 1);

        // State indicator (arrow/icon)
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(x, y + 18);
        switch (rods[i].state) {
            case 0:  M5.Lcd.print("READY");  break;
            case 1:  M5.Lcd.print("NIBBLE"); break;
            case 2:  M5.Lcd.print("FISH!!"); break;
            case 3:  M5.Lcd.print("BLOWUP"); break;
            default: M5.Lcd.print("???");    break;
        }
    }
}

void DisplayManager::showMessage(const char *msg) {
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(COLOR_WHITE, COLOR_BG);
    M5.Lcd.setCursor(10, 60);
    M5.Lcd.println(msg);
    delay(1500);
}

void DisplayManager::clear() {
    M5.Lcd.fillScreen(COLOR_BG);
}

uint16_t DisplayManager::stateColor(uint8_t state) {
    switch (state) {
        case 0:  return COLOR_GREEN;
        case 1:  return COLOR_YELLOW;
        case 2:  return COLOR_RED;
        case 3:  return TFT_RED;
        default: return COLOR_GRAY;
    }
}

void DisplayManager::drawStatusBar(const char *modeStr) {
    M5.Lcd.fillRect(0, 0, 135, 16, TFT_NAVY);
    M5.Lcd.setTextColor(COLOR_WHITE, TFT_NAVY);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(2, 3);
    M5.Lcd.print("FishOn ");
    M5.Lcd.print(modeStr);
}
