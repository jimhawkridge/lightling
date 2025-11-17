#ifndef RIG_H
#define RIG_H

#include <stdbool.h>
#include <stdio.h>
#include <strings.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RIG_ERROR_BUF_LEN 128

typedef enum FixtureTypeEnum {
  UnknownFixtureType,
  Light,
  Fire,
  Fluoro,
  Flickering,
  Blinking,
} FixtureType;

typedef void (*switcher_t)(void*, bool, bool);

typedef struct {
  FixtureType fixture_type;
  char* name;
  bool on;
  uint8_t n_channels;
  uint8_t* channels;
  uint8_t n_levels;
  uint8_t* levels;
  void* state;          // Used by the type to track animation state
  int automator_state;  // Can be used by the automator for whatever purpose
  TaskHandle_t task;
  switcher_t switcher;
} Fixture;

typedef enum AutomatorTypeEnum {
  UnknownAutomatorType,
  AlwaysOn,
  AlwaysOff,
  DoughnutShopAutomator,
  TownhouseAutomator,
  StreetAutomator,
  BookshopAutomator,
  PoliceStationAutomator,
} AutomatorType;

typedef struct {
  char* name;
  AutomatorType automator;
  bool manual;
  uint8_t n_fixtures;
  Fixture** fixtures;
} FixtureGroup;

typedef struct {
  uint8_t n_groups;
  FixtureGroup** groups;
} Rig;

FixtureType fixtureTypeFromString(const char* str);
AutomatorType automatorTypeFromString(const char* str);

void rig_init();
void rig_import(const char* buf, char* error_buf);
Rig* rig_get();
char* rig_get_json();

#endif