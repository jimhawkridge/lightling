#include <stdio.h>
#include <strings.h>

#define RIG_ERROR_BUF_LEN 128

typedef enum FixtureTypeEnum
{
    UnknownFixtureType,
    Light,
    Fire
} FixtureType;

typedef struct
{
    FixtureType fixture_type;
    char *name;
    uint8_t n_channels;
    uint8_t *channels;
} Fixture;

typedef enum AutomatorTypeEnum
{
    UnknownAutomatorType,
    AlwaysOn,
    AlwaysOff,
    OneUpOneDownAutomator,
    TownhouseAutomator,
    StreetAutomator
} AutomatorType;

typedef struct
{
    char *name;
    AutomatorType automator;
    uint8_t n_fixtures;
    Fixture **fixtures;
} FixtureGroup;

typedef struct
{
    uint8_t n_groups;
    FixtureGroup **groups;
} Rig;

FixtureType fixtureTypeFromString(const char *str);
AutomatorType automatorTypeFromString(const char *str);

void rig_init();
void rig_import(const char *buf, char *error_buf);
void rig_load();
