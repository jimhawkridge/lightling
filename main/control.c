#include "esp_log.h"

#include "leds.h"
#include "rig.h"
#include "tlc5940.h"

// static const char* TAG = "CONTROL";

void control_fade_channel(int channel, uint8_t brightness, uint32_t time) {
  if (channel < LED_CHANS) {
    leds_fade_channel(channel, brightness, time);
  } else {
    tlc_fade_channel(channel - LED_CHANS, brightness);
  }
}

void control_set_group_mode(FixtureGroup* group, bool manual) {
  group->manual = manual;
}

void control_set_mode_from_id(uint8_t group_id, bool manual) {
  Rig* rig = rig_get();
  if (rig == NULL || group_id >= rig->n_groups) {
    return;
  }

  control_set_group_mode(rig->groups[group_id], manual);
}