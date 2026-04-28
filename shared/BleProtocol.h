#ifndef BLE_PROTOCOL_H
#define BLE_PROTOCOL_H

#include <stdint.h>

/*
 * ================================================
 * FishOn-S3 BLE Communication Protocol
 * ================================================
 *
 * Sub-node broadcasts via BLE advertising using
 * Manufacturer Specific Data (Apple format type).
 *
 * Total payload: 6 bytes
 *
 * ┌────────┬────────┬────────┬────────┬────────┬────────┐
 * │ Byte 0 │ Byte 1 │ Byte 2 │ Byte 3 │ Byte 4 │ Byte 5 │
 * ├────────┼────────┼────────┼────────┼────────┼────────┤
 * │ MAGIC  │ ROD_ID │ STATE  │ AMP_H  │ AMP_L  │ BATT   │
 * │ 0xA5   │ 0-15   │ 0x00-  │ msb    │ lsb    │ 0-100  │
 * │        │        │ 0x07   │        │        │        │
 * └────────┴────────┴────────┴────────┴────────┴────────┘
 *
 * 6 bytes total, fits in BLE 31-byte advertising payload.
 */

#define BLE_PROTOCOL_MAGIC      0xA5
#define BLE_PROTOCOL_PAYLOAD_LEN 6

/* ---------- Rod ID range ---------- */
#define ROD_ID_MIN              0
#define ROD_ID_MAX              15
#define ROD_ID_BROADCAST_MAX    8   /* typically 8 rods */

/* ---------- State codes ---------- */
enum NodeState : uint8_t {
    STATE_IDLE       = 0x00,   // Normal, no activity
    STATE_SUSPECT    = 0x01,   // Small fish nibbling
    STATE_FISH_ON    = 0x02,   // Fish hooked (sustained)
    STATE_BLOW_UP    = 0x03,   // Big fish blow-up
    STATE_DEEP_SLEEP = 0x04,   // Node in deep sleep (low batt)
    STATE_ERROR      = 0x05,   // Self-test / IMU error
    STATE_HEARTBEAT  = 0x06,   // Periodic alive signal
    STATE_PAIR_REQ   = 0xFE,   // Pairing request (special)
    STATE_OFFLINE    = 0xFF,   // Host-inferred offline state
};

/* ---------- Amplitude encoding ---------- */
// 16-bit unsigned: 0-65535 → 0.0g - 16.0g
// Resolution: 16.0 / 65536 ≈ 0.244 mg per LSB
#define AMPLITUDE_MAX_G         16.0f
#define AMP_ENCODE(g)           ((uint16_t)(((g) / AMPLITUDE_MAX_G) * 65535.0f))
#define AMP_DECODE(raw)         (((float)(raw) / 65535.0f) * AMPLITUDE_MAX_G)

/* ---------- BLE advertising params ---------- */
#define BLE_ADV_INTERVAL_IDLE_MS    5000    // 5s when idle
#define BLE_ADV_INTERVAL_ALERT_MS   200     // 200ms when fish detected
#define BLE_ADV_INTERVAL_PAIR_MS    1000    // 1s during pairing
#define BLE_ADV_COUNT_PER_EVENT     3       // 3 broadcasts per event

/* ---------- Packet helpers ---------- */

/* Build advertising payload into buf[6] */
static inline void buildBlePayload(
    uint8_t buf[BLE_PROTOCOL_PAYLOAD_LEN],
    uint8_t rodId,
    uint8_t state,
    float   amplitudeG,
    uint8_t batteryPct
) {
    if (!buf) return;
    uint16_t ampRaw = AMP_ENCODE(amplitudeG);
    buf[0] = BLE_PROTOCOL_MAGIC;
    buf[1] = (rodId > ROD_ID_MAX) ? 0 : rodId;
    buf[2] = state;
    buf[3] = (uint8_t)(ampRaw >> 8);     // MSB
    buf[4] = (uint8_t)(ampRaw & 0xFF);   // LSB
    buf[5] = (batteryPct > 100) ? 100 : batteryPct;
}

/* Parse a received 6-byte payload. Returns true if magic matches. */
static inline bool parseBlePayload(
    const uint8_t buf[BLE_PROTOCOL_PAYLOAD_LEN],
    uint8_t *rodId,
    uint8_t *state,
    float   *amplitudeG,
    uint8_t *batteryPct
) {
    if (!buf || buf[0] != BLE_PROTOCOL_MAGIC) return false;
    if (rodId)       *rodId       = buf[1];
    if (state)       *state       = buf[2];
    if (amplitudeG)  *amplitudeG  = AMP_DECODE(((uint16_t)buf[3] << 8) | buf[4]);
    if (batteryPct)  *batteryPct  = buf[5];
    return true;
}

#endif // BLE_PROTOCOL_H
