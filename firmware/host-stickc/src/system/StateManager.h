#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <stdint.h>
#include "BleScanner.h"
#include "DisplayManager.h"  // for RodUiState
#include "../../../../shared/BleProtocol.h"

/*
 * Manages the multi-rod state machine on the host side.
 * Tracks per-rod state, timeout/offline detection, alert priority.
 */

class StateManager {
public:
    StateManager();
    ~StateManager();

    // Initialize with max number of rods (default 8)
    void init(uint8_t maxRods);

    // Called when a BLE packet arrives
    void onPacketReceived(const BlePacket *pkt);

    // Main update tick: handle timeouts, state transitions
    void update();

    // Get current rod states for the UI
    void getRodStates(RodUiState out[MAX_RODS]) const;

    // Get the highest-priority alert type currently
    uint8_t getActiveAlertType() const;

    // User reset after a catch
    void resetRod(uint8_t rodId);

    // Reset all rods
    void resetAll();

private:
    RodUiState m_rods[MAX_RODS];
    uint8_t m_maxRods = 8;

    // Timeout: consider offline if no packet in this many ms
    static constexpr unsigned long OFFLINE_TIMEOUT_MS = 30000;  // 30s

    // Debounce: ignore rapid state transitions
    static constexpr unsigned long STATE_DEBOUNCE_MS = 2000;
    uint8_t m_prevState[MAX_RODS];
    unsigned long m_stateChangedAt[MAX_RODS];

    // Priority order for alerts (higher = more urgent)
    uint8_t alertPriority(uint8_t state) const;
};

#endif // STATE_MANAGER_H
