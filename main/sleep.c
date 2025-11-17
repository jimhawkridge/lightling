#include <sys/time.h>
#include <time.h>
#include "driver/touch_pad.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "control.h"
#include "fixturetypes.h"
#include "rig.h"

static const char* TAG = "TOUCH";
#define TOUCHPAD 8
#define THRESH 400
#define SLEEP_CPU 0
#define WAKE_H 6
#define WAKE_M 30

bool touch;

static void sleep_do_sleep() {
  ESP_LOGI(TAG, "Shutdown");
  ESP_ERROR_CHECK(esp_sleep_enable_touchpad_wakeup());
  int sleep_secs = 30;
  esp_sleep_enable_timer_wakeup(sleep_secs * 1000000);
  // rtc_gpio_pulldown_en(22);
  esp_deep_sleep_start();
}

static void sleep_intr(void* arg) {
  touch_pad_clear_status();
  touch = true;
}

static void sleep_task(void* pvParameter) {
  int l = 0;
  while (true) {
    if (touch) {
      ESP_LOGI(TAG, "Touched");

      Rig* rig = rig_get();
      bool all_manual = true;
      for (int i = 0; i < rig->n_groups; i++) {
        FixtureGroup* group = rig->groups[i];
        if (!group->manual) {
          all_manual = false;
          break;
        }
      }
      if (!all_manual) {
        for (int i = 0; i < rig->n_groups; i++) {
          FixtureGroup* group = rig->groups[i];
          group->manual = true;
          for (int j = 0; j < group->n_fixtures; j++) {
            Fixture* fixture = group->fixtures[j];
            fixture_switch_off_fast(fixture);
          }
        }
        vTaskDelay(5000 / portTICK_RATE_MS);
        if (SLEEP_CPU) {
          sleep_do_sleep();
        }
      } else {
        for (int i = 0; i < rig->n_groups; i++) {
          FixtureGroup* group = rig->groups[i];
          group->manual = false;
        }
      }
      vTaskDelay(3000 / portTICK_RATE_MS);
      touch = false;
    }

    l++;
    if (l == 5 * 60) {
      l = 0;
      // Every minute (200ms *5 *60) check if it's "morning"
      // If so then reset everything back to auto
      int waketime = WAKE_H * 60 * 60 + WAKE_M * 60;
      struct timeval now;
      gettimeofday(&now, NULL);
      int t = now.tv_sec % 86400;

      if (t >= waketime && t < waketime + 120) {
        Rig* rig = rig_get();
        for (int i = 0; i < rig->n_groups; i++) {
          FixtureGroup* group = rig->groups[i];
          group->manual = false;
        }
      }
    }
    vTaskDelay(200 / portTICK_RATE_MS);
  }
}

void sleep_init() {
  touch = false;

  ESP_ERROR_CHECK(touch_pad_init());
  ESP_ERROR_CHECK(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER));
  ESP_ERROR_CHECK(touch_pad_config(TOUCHPAD, 0));  // GPIO 33
  // touch_fsm_mode_t fsm_mode;
  // ESP_ERROR_CHECK(touch_pad_get_fsm_mode(&fsm_mode));
  // ESP_LOGI(TAG, "tp FSM mode is %d", fsm_mode);

  ESP_ERROR_CHECK(touch_pad_filter_start(10));

  // uint16_t value;
  // ESP_ERROR_CHECK(touch_pad_read(TOUCHPAD, &value));
  // ESP_LOGI(TAG, "value is %d", value);
  ESP_LOGI(TAG, "Setting touchpad %d threshold to %d", TOUCHPAD, THRESH);
  ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCHPAD, THRESH));
  touch_pad_isr_register(sleep_intr, NULL);
  touch_pad_intr_enable();

  xTaskCreate(&sleep_task, "sleep_task", 4096, NULL, 5, NULL);
}