#include <Arduino.h>
#include "instant.h"
#include "LoadCell.h"
#include "common.h"
#include <etl/flat_map.h>
#include "button.h"
#include "valve.h"
#include "wlan_manager.h"
#include "utils.h"
#include <driver/gpio.h>

#define stringify_literal(x)     #x
#define stringify_expanded(x)    stringify_literal(x)
#define stringify_with_quotes(x) stringify_expanded(stringify_expanded(x))

#ifndef WLAN_AP_SSID
#define WLAN_AP_SSID default
#endif

#ifndef WLAN_AP_PASSWORD
#define WLAN_AP_PASSWORD default
#endif

// https://learn.microsoft.com/en-us/cpp/preprocessor/stringizing-operator-hash?view=msvc-170
const char *WLAN_SSID     = stringify_expanded(WLAN_AP_SSID);
const char *WLAN_PASSWORD = stringify_expanded(WLAN_AP_PASSWORD);

using namespace common;

// https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html#timeralarm
extern "C" [[noreturn]] void app_main(void) {
  constexpr auto TAG = "main";
  initArduino();

  auto ap = wlan::AP{WLAN_SSID, WLAN_PASSWORD};
  ESP_LOGI(TAG, "ssid=%s; password=%s;", ap.ssid.c_str(), ap.password.c_str());
  auto evt_grp = xEventGroupCreate();
  auto manager = wlan::WlanManager(evt_grp);
  manager.set_ap(std::move(ap));
  ESP_ERROR_CHECK(manager.wifi_init());
  ESP_ERROR_CHECK(manager.start_connect_task());
  ESP_ERROR_CHECK(manager.mqtt_init());

  static auto sensor    = LoadCell{pin::D_OUT, pin::DP_SCK};
  static auto valve     = peripheral::Valve{pin::VALVE_ADD, pin::VALVE_DECREASE};
  static auto punch_btn = peripheral::EdgeButton{pin::PUNCH_BTN};
restart:
  pinMode(pin::LED, OUTPUT);
  digitalWrite(pin::LED, HIGH);

  punch_btn.on_press   = []() {};
  punch_btn.on_release = []() {
    if (valve.is_enabled()) {
      ESP_LOGI(TAG, "disable valve");
      valve.disable();
    } else {
      ESP_LOGI(TAG, "enable valve");
      valve.enable();
    }
  };

  bool ok = sensor.begin();
  if (!ok) {
    ESP_LOGE(TAG, "sensor begin failed");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    goto restart;
  }
  valve.begin();
  punch_btn.begin();

  auto loop = []() {
    constexpr auto TAG = "loop";

    punch_btn.poll();
    valve.poll();
    sensor.measure();
  };

  while (true) {
    loop();
  }
}
