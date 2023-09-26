#include "control.h"
#include "rig.h"

void automation_init() {
  Rig* rig = rig_get();

  for (int i = 0; i < rig->n_groups; i++) {
    FixtureGroup* group = rig->groups[i];
    if (group->automator == StreetAutomator) {
      for (int j = 0; j < group->n_fixtures; j++) {
        control_fixture_switch(group->fixtures[j], true);
      }
    } else if (group->automator == BookshopAutomator) {
      if (group->n_fixtures >= 4) {
        control_fixture_switch(group->fixtures[0], true);
        control_fixture_switch(group->fixtures[3], true);
      }
    } else if (group->automator == TownhouseAutomator) {
      if (group->n_fixtures >= 4) {
        control_fixture_switch(group->fixtures[1], true);
        control_fixture_switch(group->fixtures[3], true);
      }
    } else if (group->automator == OneUpOneDownAutomator) {
      if (group->n_fixtures >= 2) {
        control_fixture_switch(group->fixtures[0], true);
      }
    }
  }
}