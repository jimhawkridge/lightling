void control_fixture_switch(Fixture *fixture, bool on);
void control_switch_ids(uint8_t group_id, uint8_t fixture_id, bool on);
void control_set_group_mode(FixtureGroup *group, bool manual);
void control_set_mode_from_id(uint8_t group_id, bool manual);
