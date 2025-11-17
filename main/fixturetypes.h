#ifndef FIXTURETYPES_H
#define FIXTURETYPES_H

#include "rig.h"

void fixture_switch(Fixture* f, bool on);
void fixture_switch_off_fast(Fixture* f);
void fixture_switch_ids(uint8_t group_id, uint8_t fixture_id, bool on);
void fixturetypes_init_fixture(Fixture* f);

#endif  // FIXTURETYPES_H