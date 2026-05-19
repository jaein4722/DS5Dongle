#pragma once

#include <cstdint>

struct __attribute__((packed)) BatteryStatusReport {
    uint8_t level_raw; // 0..10, or 0xFF when unknown
    uint8_t percent; // 0..100, or 0xFF when unknown
    uint8_t power_state;
    uint8_t is_charging;
    uint8_t is_connected;
    uint8_t is_fresh;
    uint32_t last_report_age_ms;
};

void battery_status_init(void);
void battery_status_update(uint8_t const *report, uint16_t len);
void battery_status_on_disconnect(void);
BatteryStatusReport battery_status_get(void);
