#include "BleScanner.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include <cstring>

/*
 * BLE scanning callback — called by NimBLE when an advertisement is received.
 * This runs in interrupt context, must be fast and non-blocking.
 */

static BleScanner *g_scannerInstance = nullptr;

class ScannerCallback : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {
        if (!g_scannerInstance) return;

        uint8_t payload[BLE_PROTOCOL_PAYLOAD_LEN];

        // Read manufacturer specific data
        std::string manufData = advertisedDevice->getManufacturerData();
        if (manufData.length() < BLE_PROTOCOL_PAYLOAD_LEN) return;

        // The manufacturer data typically starts with 2-byte company ID
        // Our protocol starts at byte 2
        memcpy(payload, manufData.data(), BLE_PROTOCOL_PAYLOAD_LEN);

        // Parse
        uint8_t rodId, state, batteryPct;
        float amp;
        if (!parseBlePayload(payload, &rodId, &state, &amp, &batteryPct)) {
            return;  // magic mismatch — not our device
        }

        // Push into ring buffer
        int nextHead = (g_scannerInstance->m_head + 1) % BleScanner::RING_SIZE;
        if (nextHead != g_scannerInstance->m_tail) {  // buffer not full
            BlePacket *pkt = &g_scannerInstance->m_ring[g_scannerInstance->m_head];
            pkt->rodId      = rodId;
            pkt->state      = state;
            pkt->amplitudeG = amp;
            pkt->batteryPct = batteryPct;
            pkt->timestamp  = millis();
            g_scannerInstance->m_head = nextHead;
        }
    }
};

/* ---- BleScanner implementation ---- */

BleScanner::BleScanner() {}
BleScanner::~BleScanner() {}

void BleScanner::init() {
    if (m_initialized) return;

    Serial.println("[BLE] Initializing scanner...");
    g_scannerInstance = this;

    NimBLEDevice::init("FishOn-Host");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Max TX power

    BLEScan *scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new ScannerCallback(), true /* want duplicates */);
    scan->setInterval(100);   // 100ms scan interval
    scan->setWindow(99);      // 99ms active window (nearly continuous)
    scan->setActiveScan(false); // Passive scan to save power
    scan->start(0, nullptr, false); // continuous scan (duration=0)

    m_initialized = true;
    m_head = 0;
    m_tail = 0;
    m_nodeCount = 0;
    Serial.println("[BLE] Scanner started (passive scan)");
}

bool BleScanner::poll(BlePacket *out) {
    if (!out || !m_initialized) return false;

    // Dequeue from ring buffer (disable interrupts for safety)
    noInterrupts();
    if (m_head == m_tail) {
        interrupts();
        return false;  // empty
    }

    *out = m_ring[m_tail];
    m_tail = (m_tail + 1) % RING_SIZE;
    interrupts();

    // Track node count
    if (out->rodId + 1 > m_nodeCount) {
        m_nodeCount = out->rodId + 1;
    }

    return true;
}

void BleScanner::reset() {
    m_head = 0;
    m_tail = 0;
    m_nodeCount = 0;
}
