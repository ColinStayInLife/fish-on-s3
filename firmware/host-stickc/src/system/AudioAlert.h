#ifndef AUDIO_ALERT_H
#define AUDIO_ALERT_H

#include <stdint.h>

/*
 * Fish bite audio alert management.
 * Uses M5StickC Plus built-in buzzer (passive).
 *
 * Alert patterns:
 * - IDLE:        silent
 * - SUSPECT:     single short beep every 5s
 * - FISH_ON:     triple sustained beep
 * - BLOW_UP:     rapid high-pitch beeps
 * - OFFLINE:     short beep every 10s
 */

enum AlertType : uint8_t {
    ALERT_NONE     = 0,
    ALERT_SUSPECT  = 1,
    ALERT_FISH_ON  = 2,
    ALERT_BLOW_UP  = 3,
    ALERT_OFFLINE  = 4,
};

class AudioAlert {
public:
    AudioAlert();
    ~AudioAlert();

    void init();

    // Called every loop iteration to drive ongoing patterns
    void update(AlertType activeType);

    // Play a single beep (frequency Hz, duration ms)
    void beep(uint16_t freq, uint16_t durationMs);

    // Silence all alarms
    void silence();

    // Quick test beep sweep
    void testPattern();

private:
    bool m_alarmSilenced = false;
    AlertType m_currentPattern = ALERT_NONE;
    unsigned long m_patternStart = 0;
    int m_beepCount = 0;

    // Pattern definitions
    static constexpr uint16_t TONE_SUSPECT  = 800;    // Hz
    static constexpr uint16_t TONE_FISH_ON  = 1200;   // Hz
    static constexpr uint16_t TONE_BLOW_UP  = 2000;   // Hz
    static constexpr uint16_t TONE_OFFLINE  = 400;    // Hz

    // Drive patterns non-blocking
    void patternSuspect();
    void patternFishOn();
    void patternBlowUp();
    void patternOffline();
};

#endif // AUDIO_ALERT_H
