#include <inttypes.h>
#include <stdio.h>

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

#include "leds.h"

static const char* TAG = "LIGHT";

static uint8_t last_values[LED_CHANS];

void leds_fade_channel(int channel, uint8_t brightness, uint32_t time) {
  if (last_values[channel] == brightness) {
    return;
  }
  last_values[channel] = brightness;

  uint32_t value = brightness << 5;
  ledc_mode_t mode = LEDC_HIGH_SPEED_MODE;
  ESP_LOGI(TAG, "Lighting channel %d to %u (%lu) over %lums", channel,
           brightness, value, time);
  if (channel > 7) {
    mode = LEDC_LOW_SPEED_MODE;
    channel -= 8;
  }
  ledc_set_fade_time_and_start(mode, channel, value, time, LEDC_FADE_NO_WAIT);
}

void leds_init() {
  ledc_timer_config_t ledc_timer = {
      .duty_resolution = LEDC_TIMER_13_BIT,
      .freq_hz = 5000,
      .speed_mode = LEDC_HIGH_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ledc_timer_config(&ledc_timer);

  ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num = LEDC_TIMER_1;
  ledc_timer_config(&ledc_timer);

  int gpios[LED_CHANS] = {19, 23, 18, 5,  10, 9,  4,  2,
                          33, 25, 26, 27, 14, 12, 13, 15};
  for (int i = 0; i < LED_CHANS; i++) {
    bool is_low = i > 7;
    ledc_channel_config_t chan = {
        .channel = i % 8,
        .duty = 0,
        .gpio_num = gpios[i],
        .speed_mode = is_low ? LEDC_LOW_SPEED_MODE : LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = is_low ? LEDC_TIMER_1 : LEDC_TIMER_0};
    ledc_channel_config(&chan);
    last_values[i] = 0;
  }
  ledc_fade_func_install(0);
}