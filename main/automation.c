#include "fixturetypes.h"
#include "rig.h"

void automation_init() {
  Rig* rig = rig_get();

  for (int i = 0; i < rig->n_groups; i++) {
    FixtureGroup* group = rig->groups[i];
    if (group->automator == AlwaysOn) {
      for (int j = 0; j < group->n_fixtures; j++) {
        fixture_switch(group->fixtures[j], true);
      }
    } else if (group->automator == AlwaysOff) {
      for (int j = 0; j < group->n_fixtures; j++) {
        fixture_switch(group->fixtures[j], false);
      }
    } else if (group->automator == StreetAutomator) {
      for (int j = 0; j < group->n_fixtures; j++) {
        fixture_switch(group->fixtures[j], true);
      }
    } else if (group->automator == BookshopAutomator) {
      if (group->n_fixtures >= 4) {
        fixture_switch(group->fixtures[0], true);
        fixture_switch(group->fixtures[3], true);
      }
    } else if (group->automator == TownhouseAutomator) {
      if (group->n_fixtures >= 4) {
        fixture_switch(group->fixtures[1], true);
        fixture_switch(group->fixtures[3], true);
      }
    } else if (group->automator == OneUpOneDownAutomator) {
      if (group->n_fixtures >= 2) {
        fixture_switch(group->fixtures[0], true);
      }
    }
  }
}