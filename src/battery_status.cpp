#include "battery_status.h"

#include <algorithm>
#include <limits>

#include "pico/critical_section.h"
#include "pico/time.h"

namespace {
constexpr uint64_t REPORT_STALE_US = 2'000'000;
constexpr uint8_t BATTERY_BYTE_INDEX = 52;
constexpr uint8_t BATTERY_UNKNOWN = 0xFF;
constexpr uint8_t POWER_STATE_CHARGING = 0x01;

critical_section_t battery_status_cs;
uint8_t level_raw = BATTERY_UNKNOWN;
uint8_t power_state = BATTERY_UNKNOWN;
uint64_t last_report_us = 0;
bool controller_connected = false;
} // namespace

void battery_status_init(void) {
    critical_section_init(&battery_status_cs);
    battery_status_on_disconnect();
}

void battery_status_update(uint8_t const *report, uint16_t len) {
    if (report == nullptr || len <= BATTERY_BYTE_INDEX) {
        return;
    }

    const uint8_t battery = report[BATTERY_BYTE_INDEX];

    critical_section_enter_blocking(&battery_status_cs);
    level_raw = battery & 0x0F;
    power_state = (battery >> 4) & 0x0F;
    last_report_us = time_us_64();
    controller_connected = true;
    critical_section_exit(&battery_status_cs);
}

void battery_status_on_disconnect(void) {
    critical_section_enter_blocking(&battery_status_cs);
    level_raw = BATTERY_UNKNOWN;
    power_state = BATTERY_UNKNOWN;
    last_report_us = 0;
    controller_connected = false;
    critical_section_exit(&battery_status_cs);
}

BatteryStatusReport battery_status_get(void) {
    BatteryStatusReport report{};
    const uint64_t now = time_us_64();

    critical_section_enter_blocking(&battery_status_cs);
    const uint8_t raw = level_raw;
    const uint8_t state = power_state;
    const uint64_t last_us = last_report_us;
    const bool connected = controller_connected;
    critical_section_exit(&battery_status_cs);

    const bool has_report = connected && last_us != 0;
    const uint64_t age_us = has_report ? now - last_us : 0;
    const bool fresh = has_report && age_us < REPORT_STALE_US;

    report.level_raw = BATTERY_UNKNOWN;
    report.percent = BATTERY_UNKNOWN;
    report.power_state = state;
    report.is_charging = state == POWER_STATE_CHARGING ? 1 : 0;
    report.is_connected = connected ? 1 : 0;
    report.is_fresh = fresh ? 1 : 0;
    report.last_report_age_ms = static_cast<uint32_t>(
        std::min<uint64_t>(age_us / 1000, std::numeric_limits<uint32_t>::max()));

    if (fresh && raw <= 10) {
        report.level_raw = raw;
        report.percent = raw * 10;
    }

    return report;
}
