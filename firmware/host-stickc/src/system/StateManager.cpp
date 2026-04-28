#include "StateManager.h"
#include <Arduino.h>
#include <cstring>

StateManager::StateManager() {}
StateManager::~StateManager() {}

void StateManager::init(uint8_t maxRods) {
    m_maxRods = (maxRods > MAX_RODS) ? MAX_RODS : maxRods;
    for (int i = 0; i < MAX_RODS; i++) {
        m_rods[i].rodId      = i;
        m_rods[i].state      = STATE_IDLE;
        m_rods[i].amplitude  = 0;
        m_rods[i].batteryPct = 0;
        m_rods[i].online     = false;
        m_rods[i].lastSeen   = 0;
        m_prevState[i]       = STATE_IDLE;
        m_stateChangedAt[i]  = 0;
    }
    Serial.print("[State] Initialized, max rods: ");
    Serial.println(m_maxRods);
}

void StateManager::onPacketReceived(const BlePacket *pkt) {
    if (!pkt || pkt->rodId >= MAX_RODS) return;

    RodUiState *rod = &m_rods[pkt->rodId];
    rod->rodId      = pkt->rodId;
    rod->amplitude  = pkt->amplitudeG;
    rod->batteryPct = pkt->batteryPct;
    rod->online     = true;
    rod->lastSeen   = millis();

    // Apply debounce to state transitions
    if (pkt->state != rod->state) {
        unsigned long now = millis();
        if (now - m_stateChangedAt[pkt->rodId] > STATE_DEBOUNCE_MS) {
            m_prevState[pkt->rodId] = rod->state;
            rod->state = pkt->state;
            m_stateChangedAt[pkt->rodId] = now;
        }
    }
}

void StateManager::update() {
    // Check for offline rods
    unsigned long now = millis();
    for (int i = 0; i < m_maxRods; i++) {
        if (m_rods[i].online && (now - m_rods[i].lastSeen > OFFLINE_TIMEOUT_MS)) {
            m_rods[i].state  = STATE_OFFLINE;
            m_rods[i].online = false;
            Serial.printf("[State] Rod %d went offline\n", i + 1);
        }
    }
}

void StateManager::getRodStates(RodUiState out[MAX_RODS]) const {
    memcpy(out, m_rods, sizeof(RodUiState) * MAX_RODS);
}

uint8_t StateManager::getActiveAlertType() const {
    uint8_t highest = ALERT_NONE;  // from AudioAlert.h (0)

    for (int i = 0; i < m_maxRods; i++) {
        if (!m_rods[i].online) continue;
        uint8_t priority = alertPriority(m_rods[i].state);
        if (priority > highest) {
            highest = priority;
        }
    }

    // Map Rod state to AlertType
    // Priority: BLOW_UP(3) > FISH_ON(2) > SUSPECT(1) > OFFLINE(4)
    switch (highest) {
        case 3:  return 3;  // ALERT_BLOW_UP
        case 2:  return 2;  // ALERT_FISH_ON
        case 1:  return 1;  // ALERT_SUSPECT
        default:
            // Check if any rod is offline
            for (int i = 0; i < m_maxRods; i++) {
                if (m_rods[i].state == STATE_OFFLINE) {
                    return 4;  // ALERT_OFFLINE
                }
            }
            return 0;  // ALERT_NONE
    }
}

void StateManager::resetRod(uint8_t rodId) {
    if (rodId >= MAX_RODS) return;
    m_rods[rodId].state     = STATE_IDLE;
    m_rods[rodId].amplitude = 0;
    m_prevState[rodId]      = STATE_IDLE;
    Serial.printf("[State] Rod %d reset\n", rodId + 1);
}

void StateManager::resetAll() {
    for (int i = 0; i < m_maxRods; i++) {
        resetRod(i);
    }
}

uint8_t StateManager::alertPriority(uint8_t state) const {
    switch (state) {
        case STATE_BLOW_UP:  return 4;
        case STATE_FISH_ON:  return 3;
        case STATE_SUSPECT:  return 2;
        case STATE_IDLE:     return 0;
        case STATE_OFFLINE:  return 1;
        default:             return 0;
    }
}
