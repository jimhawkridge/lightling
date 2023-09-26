#include "esp_log.h"

#include "chain.h"
#include "rig.h"

static const char* TAG = "CONTROL";

void control_fixture_switch(Fixture* fixture, bool on) {
  for (int i = 0; i < fixture->n_channels; i++) {
    uint8_t brightness = on ? fixture->levels[i] : 0;
    ESP_LOGI(TAG, "Channel %d bright %d", fixture->channels[i], brightness);
    chain_fade_channel(fixture->channels[i], brightness, 50);
  }
  fixture->on = on;
}

void control_switch_ids(uint8_t group_id, uint8_t fixture_id, bool on) {
  Rig* rig = rig_get();
  if (rig == NULL || group_id > rig->n_groups ||
      fixture_id > rig->groups[group_id]->n_fixtures) {
    return;
  }

  control_fixture_switch(rig->groups[group_id]->fixtures[fixture_id], on);
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