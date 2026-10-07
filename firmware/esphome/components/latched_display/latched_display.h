#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/time/real_time_clock.h"

#include <driver/gpio.h>

namespace esphome {
namespace latched_display {

// Display geometry - adjust for your panel
// 4 digits + degree LED + PM indicator
static const uint8_t NUM_DIGITS = 4;
static const uint8_t SEGMENTS_PER_DIGIT = 7;  // a-g
static const uint8_t NUM_ANNCUNCATORS = 2;     // degree, PM

// Segment to bit mapping (common 7-segment)
enum Segment { SEG_A = 0, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G };

class LatchedDisplay : public Component {
 public:
  void set_pins(GPIOPin* le, GPIOPin* clk, GPIOPin* sdi,
                 GPIOPin* d1, GPIOPin* d0, GPIOPin* en) {
    le_ = le; clk_ = clk; sdi_ = sdi;
    d1_ = d1; d0_ = d0; en_ = en;
  }

  void set_time(RealTimeClock* time) { time_ = time; }
  void set_twelve_hour(bool twelve_hour) { twelve_hour_ = twelve_hour; }
  void set_panel(LatchedDisplayLight* light) { light_ = light; }

  void setup() override;
  void loop() override;
  void dump_config() override;

  // High-level API
  void set_brightness(float brightness);  // 0.0-1.0
  void set_digit(uint8_t digit, uint8_t value);  // digit 0-3, value 0-9 or 0xF for blank
  void set_annunciator(uint8_t annc, bool on);  // 0=degree, 1=PM
  void lamp_test(bool on);

 protected:
  GPIOPin* le_ = nullptr;
  GPIOPin* clk_ = nullptr;
  GPIOPin* sdi_ = nullptr;
  GPIOPin* d1_ = nullptr;
  GPIOPin* d0_ = nullptr;
  GPIOPin* en_ = nullptr;

  RealTimeClock* time_ = nullptr;
  LatchedDisplayLight* light_ = nullptr;
  bool twelve_hour_ = false;

  // Display buffer: [digit][segment]
  // Each entry is 0=off, 1=on (single color)
  bool display_buf_[NUM_DIGITS][SEGMENTS_PER_DIGIT] = {};
  bool annc_buf_[NUM_ANNCUNCATORS] = {};
  float brightness_ = 1.0f;

  // Low-level protocol - implement based on your display's driver
  void shift_byte(uint8_t data);
  void latch();
  void select_digit(uint8_t digit);
  void set_enable(bool on);

  // Refresh one digit (called from timer ISR or loop)
  void refresh();

  // Called each second to update time display
  void update_time();

 private:
  void display_time(int hours, int minutes, int seconds);
};

// Light output trait for ESPHome integration
class LatchedDisplayLight : public light::LightOutput {
 public:
  void set_panel(LatchedDisplay* panel) { panel_ = panel; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supports_brightness(true);
    traits.set_supports_color_temperature(false);
    traits.set_supports_rgb(false);
    return traits;
  }

  void write_state(light::LightState* state) override {
    float brightness;
    state->current_values_as_brightness(&brightness);

    // Single color - just use brightness to on/off or PWM
    // For now, treat as binary on/off based on brightness threshold
    if (brightness > 0.01f) {
      panel_->set_brightness(brightness);
    } else {
      panel_->set_brightness(0.0f);
    }
  }

 protected:
  LatchedDisplay* panel_ = nullptr;
};

}  // namespace latched_display
}  // namespace esphome
