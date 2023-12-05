#ifndef PUNCHER_LOAD_CELL_H
#define PUNCHER_LOAD_CELL_H
#include <Arduino.h>
#include <HX711.h>
#include <etl/deque.h>
#include <etl/optional.h>
#include "utils.h"
#include "common.h"
class LoadCell {
private:
  gpio_num_t DOUT;
  gpio_num_t PD_SCK;
  HX711 Hx711{};

  size_t per_kg = common::LOAD_CELL_DEFAULT_UNIT_PER_KG;
  utils::MovingAverage<common::LOAD_CELL_BUF_SIZE> buf;

public:
  LoadCell(gpio_num_t d_out, gpio_num_t pd_sck) : DOUT(d_out), PD_SCK(pd_sck) {}

  bool begin() {
    Hx711.begin(DOUT, PD_SCK);
    Hx711.wait_ready_retry(10, 200);
    if (Hx711.is_ready()) {
      Hx711.tare(10);
    } else {
      return false;
    }
    return true;
  }

  void measure(uint8_t n = 1) {
    float temp = Hx711.get_units(n);
    if (temp > static_cast<float>(per_kg)) {
      buf.next(temp);
    }
  }

  etl::optional<float> average() {
    return buf.get();
  }
};

#endif