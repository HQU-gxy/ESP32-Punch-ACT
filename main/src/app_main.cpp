#include <Arduino.h>
#include "instant.h"
#include "LoadCell.h"
#include "common.h"
#include <etl/flat_map.h>
#include "button.h"
#include "valve.h"
#include <driver/gpio.h>

using namespace common;

// https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html#timeralarm

extern "C" [[noreturn]] void app_main(void) {
  initArduino();
  static auto sensor    = LoadCell{pin::D_OUT, pin::DP_SCK};
  static auto valve     = peripheral::Valve{pin::VALVE_ADD, pin::VALVE_DECREASE};
  static auto punch_btn = peripheral::EdgeButton{pin::PUNCH_BTN};
  pinMode(pin::LED, OUTPUT);
  digitalWrite(pin::LED, HIGH);
  using BtnState = peripheral::ButtonState;

  punch_btn.on_press = []() {
    valve.enable();
  };
  punch_btn.on_release = []() {
    valve.disable();
  };
  sensor.begin();
  valve.begin();
  punch_btn.begin();

  auto loop = []() {
    constexpr auto TAG = "loop";

    punch_btn.poll();
    if (punch_btn.state() == BtnState ::Pressed) {
      valve.poll();
    } else {
      // nothing
    }
    sensor.measure_once();
  };
  while (true) {
    loop();
  }
}
