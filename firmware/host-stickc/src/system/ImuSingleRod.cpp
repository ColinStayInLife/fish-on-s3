#include "ImuSingleRod.h"
#include <Arduino.h>

ImuSingleRod::ImuSingleRod() {}
ImuSingleRod::~ImuSingleRod() {}

void ImuSingleRod::init() {
    Serial.println("[IMU] Initializing MPU6886...");
    // M5.begin() already calls IMU.Init()
    m_calibrated = false;
    m_calibCount = 0;
    m_state = 0;
    m_smoothAmp = 0.0f;
    m_sustainedStart = 0;
    m_lastEventTime = 0;
}

bool ImuSingleRod::readAccel(float *ax, float *ay, float *az) {
    // M5.IMU.getAccelData reads from MPU6886
    // Returns values in g (gravitational units)
    M5.IMU.getAccelData(ax, ay, az);

    // Auto-calibration: first 50 samples determine baseline
    if (!m_calibrated) {
        m_baseX += *ax;
        m_baseY += *ay;
        m_baseZ += *az;
        m_calibCount++;

        if (m_calibCount >= CALIBRATION_SAMPLES) {
            m_baseX /= CALIBRATION_SAMPLES;
            m_baseY /= CALIBRATION_SAMPLES;
            m_baseZ /= CALIBRATION_SAMPLES;
            m_calibrated = true;
            Serial.print("[IMU] Calibration done. Base: ");
            Serial.print(m_baseX); Serial.print(", ");
            Serial.print(m_baseY); Serial.print(", ");
            Serial.println(m_baseZ);
        }
        return false;  // still calibrating, no valid data yet
    }

    return true;
}

float ImuSingleRod::computeMagnitude(float ax, float ay, float az) {
    // Compute dynamic acceleration magnitude (minus gravity baseline)
    float dx = ax - m_baseX;
    float dy = ay - m_baseY;
    float dz = az - m_baseZ;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

void ImuSingleRod::update(float magnitude) {
    // Low-pass filter to smooth noise
    m_smoothAmp = LPF_ALPHA * magnitude + (1.0f - LPF_ALPHA) * m_smoothAmp;

    unsigned long now = millis();
    float amp = m_smoothAmp;

    switch (m_state) {
        case 0:  // IDLE
            if (amp > THRESH_BLOW_UP) {
                // Instant blow-up detection
                m_state = 3;
                m_lastEventTime = now;
                Serial.println("[IMU] BLOW-UP detected!");
            } else if (amp > THRESH_FISH_ON) {
                // Possible fish-on, start sustained timer
                m_sustainedStart = now;
                m_state = 2;
                // Don't alert yet, wait for sustained duration
            } else if (amp > THRESH_SUSPECT) {
                m_state = 1;
                m_lastEventTime = now;
                Serial.println("[IMU] Suspect nibble");
            }
            break;

        case 1:  // SUSPECT (nibbling)
            if (amp > THRESH_FISH_ON) {
                m_sustainedStart = now;
                m_state = 2;
            } else if (amp < THRESH_IDLE_DN) {
                m_state = 0;  // back to idle
            }
            break;

        case 2:  // FISH_ON
            if (amp > THRESH_BLOW_UP) {
                m_state = 3;  // Escalate
            } else if (amp > THRESH_FISH_ON) {
                // Check sustained duration for confirmation
                if (now - m_sustainedStart >= FISH_ON_MS) {
                    // Confirmed fish-on — stays in this state
                    // Host will trigger the alert based on state
                }
            } else if (amp < THRESH_IDLE_DN) {
                // Vibration stopped, fish may have escaped
                if (now - m_sustainedStart < FISH_ON_MS) {
                    m_state = 0; // False alarm
                } else {
                    m_state = 0; // Fish landed or escaped, reset
                    Serial.println("[IMU] Fish-on ended, reset to idle");
                }
            }
            break;

        case 3:  // BLOW_UP
            // Stay in blow-up until user resets (button press)
            // Or if quiet for > 10s, auto-reset
            if (amp < THRESH_IDLE_DN && (now - m_lastEventTime > 10000)) {
                m_state = 0;
                Serial.println("[IMU] Blow-up auto-reset");
            }
            break;
    }
}
