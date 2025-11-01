#include <inttypes.h>
#include <stdbool.h>

#include "rig.h"

void control_fade_channel(int channel, uint8_t brightness, uint32_t time);
void control_set_group_mode(FixtureGroup* group, bool manual);
void control_set_mode_from_id(uint8_t group_id, bool manual);
