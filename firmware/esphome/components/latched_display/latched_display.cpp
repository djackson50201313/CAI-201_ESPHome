#include "latched_display.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/time_pt.h"

#include <driver/gptimer.h>
#include <soc/gpio_struct.h>

namespace esphome {
namespace latched_display {

static const char *const TAG = "latched_display";

// Digit patterns for 7-segment display (0-9, blank)
// a b c d e f g
static const uint8_t DIGIT_PATTERNS[] = {
    0b0111111,  // 0: a,b,c,d,e,f
    0b0000110,  // 1: b,c
    0b1011101,  // 2: a,b,d,e,g
    0b1001111,  // 3: a,b,c,d,g
    0b1100110,  // 4: b,c,f,g
    0b1101101,  // 5: a,c,d,f,g
    0b1111101,  // 6: a,c,d,e,f,g
    0b0000111,  // 7: a,b,c
    0b1111111,  // 8: all
    0b1101111,  // 9: a,b,c,d,f,g
    0b0000000,  // blank
};

void LatchedDisplay::setup() {
  // Initialize all pins as outputs, LOW
  le_->setup(); le_->digital_write(false);
  clk_->setup(); clk_->digital_write(false);
  sdi_->setup(); sdi_->digital_write(false);
  d1_->setup(); d1_->digital_write(false);
  d0_->setup(); d0_->digital_write(false);
  en_->setup(); en_->digital_write(false);

  // Initial display: all segments
  lamp_test(true);

  ESP_LOGCONFIG(TAG, "Latched Display initialized");
}

void LatchedDisplay::loop() {
  // Update time once per second
  static uint32_t last_update = 0;
  uint32_t now = millis();
  if (now - last_update >= 1000) {
    last_update = now;
    update_time();
  }

  // Refresh display continuously for multiplexing
  refresh();
}

void LatchedDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "Latched Display:");
  LOG_PIN("  LE Pin: ", le_);
  LOG_PIN("  CLK Pin: ", clk_);
  LOG_PIN("  SDI Pin: ", sdi_);
  LOG_PIN("  D1 Pin: ", d1_);
  LOG_PIN("  D0 Pin: ", d0_);
  LOG_PIN("  EN Pin: ", en_);
  ESP_LOGCONFIG(TAG, "  12-hour mode: %s", twelve_hour_ ? "yes" : "no");
}

// --- High-level API ---

void LatchedDisplay::set_brightness(float brightness) {
  brightness_ = clamp(brightness, 0.0f, 1.0f);
}

void LatchedDisplay::set_digit(uint8_t digit, uint8_t value) {
  if (digit >= NUM_DIGITS) return;
  if (value > 10) value = 10;  // blank

  uint8_t pattern = DIGIT_PATTERNS[value];
  for (int s = 0; s < SEGMENTS_PER_DIGIT; s++) {
    display_buf_[digit][s] = (pattern >> s) & 1;
  }
}

void LatchedDisplay::set_annunciator(uint8_t annc, bool on) {
  if (annc < NUM_ANNCUNCATORS) {
    annc_buf_[annc] = on;
  }
}

void LatchedDisplay::lamp_test(bool on) {
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < SEGMENTS_PER_DIGIT; s++) {
      display_buf_[d][s] = on;
    }
  }
  for (int a = 0; a < NUM_ANNCUNCATORS; a++) {
    annc_buf_[a] = on;
  }
}

// --- Low-level protocol ---
// TIMING: These delays may need tuning based on your driver chip's specs
// Typical 74HC595: ~100ns setup/hold, ~20MHz max
// Typical TPIC6B595: similar, but open-drain outputs

void IRAM_ATTR LatchedDisplay::shift_byte(uint8_t data) {
  // MSB first
  for (int i = 7; i >= 0; i--) {
    sdi_->digital_write((data >> i) & 1);
    clk_->digital_write(true);
    // Add delay if needed: ets_delay_us(1);
    clk_->digital_write(false);
  }
}

void IRAM_ATTR LatchedDisplay::latch() {
  le_->digital_write(true);
  // ets_delay_us(1);
  le_->digital_write(false);
}

void IRAM_ATTR LatchedDisplay::select_digit(uint8_t digit) {
  // D1:D0 binary selects digit 0-3
  // Adjust mapping based on your hardware
  d0_->digital_write(digit & 1);
  d1_->digital_write(digit & 2);
}

void IRAM_ATTR LatchedDisplay::set_enable(bool on) {
  en_->digital_write(on);
}

// --- Refresh loop ---
// Call this repeatedly (e.g., from a timer) to scan all digits

void LatchedDisplay::refresh() {
  // For single-color multiplexed display:
  // 1. Shift out segment data for current digit
  // 2. Latch
  // 3. Select digit
  // 4. Enable output
  // 5. Wait (multiplex interval)
  // 6. Disable
  // 7. Next digit

  // This is a placeholder - the exact protocol depends on the driver chip.
  // Common patterns:
  // - 74HC595: shift 8 bits (segments + annunciators), latch, then digit select
  // - TPIC6B595: similar but open-drain, may need different enable timing

  // Placeholder: just set all digits to "8" for testing
  for (int d = 0; d < NUM_DIGITS; d++) {
    select_digit(d);
    shift_byte(0xFF);  // All segments on
    latch();
    set_enable(true);
    // ets_delay_us(1000);  // 1ms per digit
    set_enable(false);
  }
}

void LatchedDisplay::update_time() {
  if (time_ == nullptr) return;

  auto t = time_->now();
  if (!t.is_valid()) return;

  int hours = t.hour;
  int minutes = t.minute;
  int seconds = t.second;

  if (twelve_hour_) {
    if (hours >= 12) {
      set_annunciator(1, true);  // PM
      hours = hours == 12 ? 12 : hours - 12;
    } else {
      set_annunciator(1, false);  // AM
      hours = hours == 0 ? 12 : hours;
    }
  }

  display_time(hours, minutes, seconds);
}

void LatchedDisplay::display_time(int hours, int minutes, int seconds) {
  // HH:MM format, seconds via blinking colon
  int h1 = hours / 10;
  int h2 = hours % 10;
  int m1 = minutes / 10;
  int m2 = minutes % 10;

  set_digit(0, h1);
  set_digit(1, h2);
  set_digit(2, m1);
  set_digit(3, m2);

  // Blink colon on even seconds
  // (handled in refresh loop based on time)
}

}  // namespace latched_display
}  // namespace esphome
