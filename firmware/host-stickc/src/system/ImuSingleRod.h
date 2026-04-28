#ifndef IMU_SINGLE_ROD_H
#define IMU_SINGLE_ROD_H

#include <M5StickCPlus.h>
#include <stdint.h>

/*
 * Single-rod mode IMU reader.
 * Uses M5StickC Plus built-in MPU6886 to detect rod-tip vibration.
 *
 * Key design choices:
 * - High-pass filter to remove gravity DC component
 * - Running RMS over a short window to quantify vibration energy
 * - Simple threshold-based detection with hysteresis
 */

class ImuSingleRod {
public:
    ImuSingleRod();
    ~ImuSingleRod();

    // Call once in setup()
    void init();

    // Read latest acceleration for all 3 axes.
    // Returns true if data is fresh.
    bool readAccel(float *ax, float *ay, float *az);

    // Compute magnitude = sqrt(ax² + ay² + az²) - gravity
    float computeMagnitude(float ax, float ay, float az);

    // Update internal state machine with new magnitude.
    // Should be called at ~50-100Hz.
    void update(float magnitude);

    // Get current detection state (0=idle, 1=suspect, 2=fish-on, 3=blow-up)
    uint8_t getState() const { return m_state; }

    // Get smoothed amplitude (low-pass filtered)
    float getSmoothedAmplitude() const { return m_smoothAmp; }

private:
    // Calibration
    float m_baseX = 0, m_baseY = 0, m_baseZ = 0;
    bool  m_calibrated = false;
    int   m_calibCount = 0;
    static constexpr int CALIBRATION_SAMPLES = 50;

    // State machine
    uint8_t m_state = 0;

    // Low-pass filter coefficient (0.0-1.0)
    static constexpr float LPF_ALPHA = 0.3f;
    float m_smoothAmp = 0.0f;

    // Timing
    unsigned long m_lastEventTime = 0;
    unsigned long m_sustainedStart = 0;

    // Hysteresis thresholds (will be adaptive in later phases)
    // Currently using fixed starting values
    static constexpr float THRESH_SUSPECT  = 0.3f;   // g
    static constexpr float THRESH_FISH_ON  = 1.2f;   // g (sustained)
    static constexpr float THRESH_BLOW_UP  = 3.5f;   // g (instant)
    static constexpr float THRESH_IDLE_DN  = 0.15f;  // g (hysteresis)
    static constexpr int   FISH_ON_MS      = 2000;   // sustained requirement
};

#endif // IMU_SINGLE_ROD_H
