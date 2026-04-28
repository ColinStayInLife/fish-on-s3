/*
 * FishOn-S3 — Host Firmware (M5StickC Plus)
 * 
 * Phase 1: StickC IMU Demo — Read MPU6886 waveform + buzzer test
 * Phase 2: BLE scanning + multi-rod identification
 *
 * Build for: PlatformIO + M5StickC Plus
 * Framework: Arduino
 */

#include <M5StickCPlus.h>
#include <Arduino.h>

// --- Phase 1: Single-rod mode includes ---
#include "system/ImuSingleRod.h"
#include "system/DisplayManager.h"
#include "system/AudioAlert.h"

// --- Phase 2: Multi-rod BLE includes ---
#include "system/BleScanner.h"
#include "system/StateManager.h"

/* =============================================================
 * Configuration
 * ============================================================= */
enum OperatingMode : uint8_t {
    MODE_SINGLE_ROD = 0,   // StickC clipped on a single rod, no BLE
    MODE_MULTI_ROD  = 1,   // StickC as receiver for multiple rods
};

// Change this based on how you're using it today
static OperatingMode g_mode = MODE_SINGLE_ROD;

/* =============================================================
 * Module Instances
 * ============================================================= */
static ImuSingleRod   g_imu;
static DisplayManager g_display;
static AudioAlert     g_alert;
static BleScanner     g_scanner;
static StateManager   g_state;

/* =============================================================
 * Setup — runs once at power-on
 * ============================================================= */
void setup() {
    // Initialize M5StickC (power, IMU, screen, speaker)
    M5.begin();
    M5.IMU.Init();

    // Ensure speaker is initialized
    pinMode(M5STICKCPLUS_BUZZER_PIN, OUTPUT);
    digitalWrite(M5STICKCPLUS_BUZZER_PIN, LOW);  // speaker off initially

    Serial.begin(115200);
    Serial.println("[FishOn-S3] Host starting...");

    // Initialize modules
    g_display.init();
    g_alert.init();
    g_imu.init();

    // Show splash
    g_display.showSplash("FishOn-S3", g_mode == MODE_MULTI_ROD ? "Multi-rod" : "Single Rod");

    if (g_mode == MODE_SINGLE_ROD) {
        // Single-rod mode: no BLE needed
        Serial.println("[FishOn-S3] Mode: Single Rod (local IMU only)");
    } else {
        // Multi-rod mode: start BLE scanner
        Serial.println("[FishOn-S3] Mode: Multi-Rod (BLE receiver)");
        g_scanner.init();
        g_state.init(ROD_ID_BROADCAST_MAX);
    }

    // Startup beep
    g_alert.beep(1000, 100);  // 1kHz, 100ms
    delay(500);
    g_alert.beep(1500, 100);

    Serial.println("[FishOn-S3] Host ready.");
}

/* =============================================================
 * Loop — runs continuously
 * ============================================================= */
void loop() {
    M5.update();  // read button states

    if (g_mode == MODE_SINGLE_ROD) {
        // ----- Single-rod mode: detect vibration locally -----
        float ax, ay, az;
        if (g_imu.readAccel(&ax, &ay, &az)) {
            float magnitude = g_imu.computeMagnitude(ax, ay, az);
            g_display.updateSingleRod(magnitude);
            g_imu.update(magnitude);
        }
    } else {
        // ----- Multi-rod mode: poll BLE scanner -----
        BlePacket packet;
        while (g_scanner.poll(&packet)) {
            g_state.onPacketReceived(&packet);
        }
        g_state.update();               // check states, transition logic
        g_display.updateMultiRod(g_state.getRodStates());
    }

    // Check button A → silent alarm
    if (M5.BtnA.wasPressed()) {
        g_alert.silence();
        g_display.showMessage("Alarm silenced");
    }

    // Check button B → mode toggle / detail view
    if (M5.BtnB.wasPressed()) {
        // Future: toggle to detail view for selected rod
        Serial.println("[FishOn-S3] Button B pressed");
    }

    // Audio refresh (handles ongoing alert patterns)
    g_alert.update(g_state.getActiveAlertType());

    delay(10);  // ~100Hz loop
}
