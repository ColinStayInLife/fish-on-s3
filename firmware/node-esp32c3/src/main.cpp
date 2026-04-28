/*
 * FishOn-S3 — Rod-tip Sub-node Firmware (ESP32-C3 + MPU6050)
 *
 * Phase 3 (to be developed after Phase 1+2 on host):
 * - Local IMU vibration analysis on MPU6050
 * - BLE broadcast of results
 * - Deep sleep power management
 *
 * Build for: PlatformIO + ESP32-C3 DevKitM-1 (or super-mini board)
 * Framework: Arduino
 * Libraries: MPU6050, NimBLE-Arduino
 */

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <NimBLEDevice.h>

#include "../../../shared/BleProtocol.h"    // for buildBlePayload

/* =============================================================
 * Pin Definitions (ESP32-C3 super-mini board)
 * ============================================================= */
#define PIN_I2C_SDA  6   // GPIO6 → MPU6050 SDA
#define PIN_I2C_SCL  5   // GPIO5 → MPU6050 SCL
#define PIN_MPU_INT  7   // GPIO7 → MPU6050 INT pin (optional)

// Battery voltage measurement (via voltage divider on GPIO3/ADC)
// CR2032 through a 100k+100k divider → 1.5V max on ADC
#define PIN_BATT_ADC 3
#define BATT_DIVIDER_RATIO 2.0f

/* =============================================================
 * Node Configuration
 * ============================================================= */
// Each node's rod ID (0-15). Set by hardcoding on flash, later via pairing.
// Change this per node before flashing.
#define ROD_ID 0

// How many seconds before MPU6050 motion interrupt timeout
#define SLEEP_TIMEOUT_SEC 10

/* =============================================================
 * Global objects
 * ============================================================= */
static MPU6050 g_imu;
static bool    g_imuReady = false;

typedef struct {
    float ax, ay, az;   // raw acceleration in g
} ImuSample;

// Sliding window for vibration analysis
static constexpr int WINDOW_SIZE = 50;  // 0.5s at 100Hz
static ImuSample g_window[WINDOW_SIZE];
static int g_windowIdx = 0;
static int g_sampleCount = 0;

// Baseline (gravity compensation)
static float g_baseX = 0, g_baseY = 0, g_baseZ = 0;
static bool  g_calibrated = false;

/* =============================================================
 * Forward declarations
 * ============================================================= */
static bool setupIMU();
static bool calibrateIMU();
static bool readIMU(float *ax, float *ay, float *az);
static void analyzeVibration(float *amplitude, int *overZeroCount);
static void reportState(float amplitude, int frequency, uint8_t battery);
static uint8_t readBatteryPct();
static void deepSleep();
static void setupBLE();

/* =============================================================
 * Setup
 * ============================================================= */
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== FishOn-S3 Sub-node ===");
    Serial.printf("Rod ID: %d\n", ROD_ID);

    // Initialize I2C for MPU6050
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(400000);  // 400kHz fast-mode

    // Setup IMU
    if (!setupIMU()) {
        Serial.println("[FATAL] IMU init failed, entering deep sleep");
        deepSleep();
        return;
    }

    // Calibrate baseline (stationary average)
    if (!calibrateIMU()) {
        Serial.println("[FATAL] Calibration failed");
        deepSleep();
        return;
    }

    // Setup BLE advertising
    setupBLE();

    Serial.println("[Node] Ready. Monitoring rod vibrations...");
}

/* =============================================================
 * Main loop
 * ============================================================= */
void loop() {
    float ax, ay, az;

    if (!readIMU(&ax, &ay, &az)) {
        return;  // skip if data not ready
    }

    // Store in window
    g_window[g_windowIdx].ax = ax;
    g_window[g_windowIdx].ay = ay;
    g_window[g_windowIdx].az = az;
    g_windowIdx = (g_windowIdx + 1) % WINDOW_SIZE;
    if (g_sampleCount < WINDOW_SIZE) g_sampleCount++;

    // Wait until we have a full window to analyze
    if (g_sampleCount < WINDOW_SIZE) {
        delay(10);  // ~100Hz sampling
        return;
    }

    // Analyze vibration
    float amplitude = 0;
    int overZeroCount = 0;
    analyzeVibration(&amplitude, &overZeroCount);

    // Read battery
    uint8_t battery = readBatteryPct();

    // Determine state and broadcast
    reportState(amplitude, overZeroCount, battery);

    // Check if rod has been idle > SLEEP_TIMEOUT_SEC
    // (Simplified: always stay awake in Phase 1)
    delay(10);
}

/* =============================================================
 * IMU Setup
 * ============================================================= */
static bool setupIMU() {
    Serial.println("[IMU] Initializing MPU6050...");
    Wire.beginTransmission(0x68);
    if (Wire.endTransmission() != 0) {
        Serial.println("[IMU] No MPU6050 found on I2C bus!");
        return false;
    }

    g_imu.initialize();
    if (!g_imu.testConnection()) {
        Serial.println("[IMU] MPU6050 connection test failed!");
        return false;
    }

    // Configure accelerometer: ±2g range (highest sensitivity)
    g_imu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);

    // Disable sleep mode
    g_imu.setSleepEnabled(false);

    Serial.println("[IMU] MPU6050 OK");
    return true;
}

/* =============================================================
 * Calibration
 * ============================================================= */
static bool calibrateIMU() {
    Serial.println("[IMU] Calibrating gravity baseline (1s)...");
    const int samples = 100;
    float sumX = 0, sumY = 0, sumZ = 0;

    for (int i = 0; i < samples; i++) {
        int16_t rax, ray, raz;
        g_imu.getAcceleration(&rax, &ray, &raz);
        sumX += rax / 16384.0f;  // convert to g (LSB/g at ±2g)
        sumY += ray / 16384.0f;
        sumZ += raz / 16384.0f;
        delay(10);
    }

    g_baseX = sumX / samples;
    g_baseY = sumY / samples;
    g_baseZ = sumZ / samples;
    g_calibrated = true;

    Serial.printf("  Baseline: ax=%.3f ay=%.3f az=%.3f g\n", g_baseX, g_baseY, g_baseZ);
    return true;
}

/* =============================================================
 * Read IMU
 * ============================================================= */
static bool readIMU(float *ax, float *ay, float *az) {
    if (!g_imuReady) {
        int16_t rax, ray, raz;
        g_imu.getAcceleration(&rax, &ray, &raz);
        *ax = rax / 16384.0f;
        *ay = ray / 16384.0f;
        *az = raz / 16384.0f;
        return true;
    }
    return false;
}

/* =============================================================
 * Vibration Analysis
 * ============================================================= */
static void analyzeVibration(float *amplitude, int *overZeroCount) {
    // Compute dynamic magnitude relative to baseline
    float sumSq = 0;
    float prevZ = 0;
    int zeroCount = 0;
    float maxAmp = 0;

    for (int i = 0; i < g_sampleCount; i++) {
        float dx = g_window[i].ax - g_baseX;
        float dy = g_window[i].ay - g_baseY;
        float dz = g_window[i].az - g_baseZ;
        float mag = sqrt(dx*dx + dy*dy + dz*dz);

        sumSq += mag * mag;

        if (mag > maxAmp) maxAmp = mag;

        // Over-zero detection (rough frequency estimation)
        if (i > 0) {
            // Check if signal crossed through zero in any axis
            float deltaX = (g_window[i].ax - g_baseX) * (g_window[i-1].ax - g_baseX);
            if (deltaX < 0) zeroCount++;
        }
    }

    // RMS amplitude (g)
    *amplitude = sqrt(sumSq / g_sampleCount);

    // Frequency estimation: over-zero count per second
    // Window is 0.5s at 100Hz, so multiply by 2
    *overZeroCount = zeroCount * 2;

    // Keep around the max amplitude for broadcast
    // (Use peak for alert, RSS for display)
    *amplitude = maxAmp;  // Use peak amplitude for alerting
}

/* =============================================================
 * State Determination and Broadcast
 * ============================================================= */
static void reportState(float amplitude, int frequency, uint8_t battery) {
    uint8_t state;

    // Frequency filter: >20Hz is likely wind/water noise
    if (frequency > 20 && amplitude < 2.0f) {
        state = STATE_IDLE;  // ignore high-frequency noise
    }
    // Blow-up: instantaneous high peak
    else if (amplitude > 3.5f) {
        state = STATE_BLOW_UP;
        Serial.printf("[ALERT] BLOW-UP! amp=%.2f freq=%dHz\n", amplitude, frequency);
    }
    // Fish on: sustained medium amplitude
    else if (amplitude > 1.2f) {
        state = STATE_FISH_ON;
        Serial.printf("[ALERT] FISH-ON! amp=%.2f freq=%dHz\n", amplitude, frequency);
    }
    // Suspect: low amplitude
    else if (amplitude > 0.3f) {
        state = STATE_SUSPECT;
        Serial.printf("[ALERT] Nibble amp=%.2f\n", amplitude);
    }
    else {
        state = STATE_IDLE;
    }

    // Build BLE payload and broadcast
    uint8_t payload[BLE_PROTOCOL_PAYLOAD_LEN];
    buildBlePayload(payload, ROD_ID, state, amplitude, battery);

    // NimBLE advertising update (simplified — will be fleshed out in Phase 3)
    // For now, we just print the packet
    Serial.printf("[BLE] Broadcast: ID=%d ST=%d AMP=%.2f BATT=%d%%\n",
                  ROD_ID, state, amplitude, battery);
}

/* =============================================================
 * Battery Reading
 * ============================================================= */
static uint8_t readBatteryPct() {
    // CR2032: 3.2V full, 2.5V empty
    // Through 2:1 divider → ADC reads 1.6V full, 1.25V empty
    // ESP32-C3 internal ADC reference ~1.1V, so we need external ref
    // For Phase 1: return dummy 75%
    (void)PIN_BATT_ADC;
    (void)BATT_DIVIDER_RATIO;
    return 75;
}

/* =============================================================
 * Deep Sleep
 * ============================================================= */
static void deepSleep() {
    Serial.println("[Power] Entering deep sleep...");
    esp_deep_sleep_start();
}

/* =============================================================
 * BLE Setup (Phase 3 stub)
 * ============================================================= */
static void setupBLE() {
    Serial.println("[BLE] Initializing NimBLE advertiser...");
    NimBLEDevice::init("FishOn-Node");
    NimBLEDevice::setPower(ESP_PWR_LVL_P7);  // moderate power to save battery

    // NimBLE advertising configuration will be added in Phase 3
    Serial.println("[BLE] Advertiser initialized (stub)");
}
