#include "AudioAlert.h"
#include <M5StickCPlus.h>
#include <Arduino.h>

AudioAlert::AudioAlert() {}
AudioAlert::~AudioAlert() {}

void AudioAlert::init() {
    pinMode(M5STICKCPLUS_BUZZER_PIN, OUTPUT);
    digitalWrite(M5STICKCPLUS_BUZZER_PIN, LOW);
    m_alarmSilenced = false;
    m_currentPattern = ALERT_NONE;
    Serial.println("[Audio] Buzzer initialized");
}

void AudioAlert::beep(uint16_t freq, uint16_t durationMs) {
    // M5StickC Plus buzzer is driven via PWM on M5STICKCPLUS_BUZZER_PIN
    // Use ledc (PWM) for tone generation
    ledcWrite(0, 0);  // stop first
    ledcAttachPin(M5STICKCPLUS_BUZZER_PIN, 0);
    ledcSetup(0, freq, 8);
    ledcWrite(0, 128);  // 50% duty
    delay(durationMs);
    ledcWrite(0, 0);  // off
}

void AudioAlert::update(AlertType activeType) {
    if (m_alarmSilenced) return;
    if (activeType == m_currentPattern) return;  // already playing

    m_currentPattern = activeType;

    switch (activeType) {
        case ALERT_NONE:
            // Ensure buzzer is quiet
            ledcWrite(0, 0);
            break;
        case ALERT_SUSPECT:
            patternSuspect();
            break;
        case ALERT_FISH_ON:
            patternFishOn();
            break;
        case ALERT_BLOW_UP:
            patternBlowUp();
            break;
        case ALERT_OFFLINE:
            patternOffline();
            break;
    }
}

void AudioAlert::silence() {
    m_alarmSilenced = true;
    m_currentPattern = ALERT_NONE;
    ledcWrite(0, 0);
    Serial.println("[Audio] Silenced");
}

void AudioAlert::testPattern() {
    Serial.println("[Audio] Test pattern");
    for (int f = 400; f <= 2000; f += 200) {
        beep(f, 100);
        delay(50);
    }
}

// --- Pattern implementations (non-blocking version) ---

void AudioAlert::patternSuspect() {
    // Short single beep, non-blocking via millis check in loop
    // For simplicity in Phase 1, use blocking beep
    // In Phase 2/3, refactor to non-blocking state machine
    beep(TONE_SUSPECT, 80);
}

void AudioAlert::patternFishOn() {
    // Triple sustained beep
    for (int i = 0; i < 3; i++) {
        beep(TONE_FISH_ON, 400);
        delay(100);
    }
}

void AudioAlert::patternBlowUp() {
    // Rapid high-pitch sequence
    for (int i = 0; i < 10; i++) {
        beep(TONE_BLOW_UP, 100);
        delay(50);
    }
}

void AudioAlert::patternOffline() {
    beep(TONE_OFFLINE, 50);
}
