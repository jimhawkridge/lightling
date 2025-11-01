// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_err.h"
// #include "esp_log.h"

#include "automation.h"
#include "e131.h"
#include "leds.h"
#include "rig.h"
#include "tlc5940.h"
#include "webapp.h"
#include "wifi.h"

// static const char *TAG = "MAIN";

void app_main() {
  wifi_init();
  wifi_connect();
  // e131_init();

  leds_init();
  tlc_init();
  rig_init();
  automation_init();

  webapp_start();
}
