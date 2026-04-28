#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5StickCPlus.h>
#include <stdint.h>

/*
 * Manages the 1.14" 135×240 TFT display.
 * Handles splash screen, single-rod view, multi-rod grid.
 */

// Max number of rods displayed in multi-rod mode
#define MAX_RODS 8

// Data structure representing one rod's state for the UI
struct RodUiState {
    uint8_t rodId;
    uint8_t state;      // 0=idle, 1=suspect, 2=fish-on, 3=blow-up
    float   amplitude;
    uint8_t batteryPct;
    bool    online;     // true if received packet recently
    unsigned long lastSeen;  // timestamp
};

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();

    void init();

    // Splash screen at boot
    void showSplash(const char *title, const char *subtitle);

    // Single-rod mode: show amplitude + waveform bar
    void updateSingleRod(float magnitude);

    // Multi-rod mode: show grid of all rods
    void updateMultiRod(const RodUiState rods[MAX_RODS]);

    // Show a transient message on screen
    void showMessage(const char *msg);

    // Clear screen
    void clear();

private:
    bool m_initialized = false;
    unsigned long m_lastScreenUpdate = 0;
    static constexpr int SCREEN_REFRESH_MS = 50;  // ~20fps

    // Internal helper to get color for a state
    uint16_t stateColor(uint8_t state);

    // Draw the top status bar
    void drawStatusBar(const char *modeStr);
};

#endif // DISPLAY_MANAGER_H
