#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <stdint.h>
#include "../../../../shared/BleProtocol.h"

/*
 * BLE scanner for multi-rod mode.
 * Scans for advertising packets from rod-tip sub-nodes.
 * Filters by BLE_PROTOCOL_MAGIC.
 */

// Max number of discovered sub-nodes
#define MAX_SCANNED_NODES 16

struct BlePacket {
    uint8_t rodId;
    uint8_t state;       // NodeState code
    float   amplitudeG;
    uint8_t batteryPct;
    unsigned long timestamp;  // millis() when received
};

class BleScanner {
public:
    BleScanner();
    ~BleScanner();

    // Initialize BLE in scanner mode
    void init();

    // Poll for available packets.
    // Returns true if a packet was dequeued into `out`.
    bool poll(BlePacket *out);

    // Get number of unique sub-nodes seen
    uint8_t nodeCount() const { return m_nodeCount; }

    // Reset scanner (e.g. after mode change)
    void reset();

private:
    bool m_initialized = false;

    // Packet ring buffer
    static constexpr int RING_SIZE = 32;
    BlePacket m_ring[RING_SIZE];
    volatile int m_head = 0;
    volatile int m_tail = 0;

    uint8_t m_nodeCount = 0;
};

#endif // BLE_SCANNER_H
