#include <sys/time.h>
#include <time.h>
#include "esp_log.h"
#include "esp_random.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fixturetypes.h"
#include "rig.h"

// static const char* TAG = "AUTO";
static const int OVERRIDE_OFF = 0;
static const int OVERRIDE_ON = 1;

static int hm(int h, int m) {
  return h * 60 * 60 + m * 60;
}

static bool in_range(int t, int t_on, int t_off) {
  if (t_on == t_off) {
    return false;
  }

  if (t_on < t_off) {
    return t >= t_on && t < t_off;
  } else {
    return t <= t_off || t > t_on;
  }
}

static int rand_range(int t_start, int t_end) {
  return (rand() % (t_end - t_start + 1)) + t_start;
}

static void sched_timed(Fixture* fixture,
                        int t,
                        int t_on1,
                        int t_off1,
                        int t_on2,
                        int t_off2) {
  bool on = in_range(t, t_on1, t_off1) || in_range(t, t_on2, t_off2);
  if (fixture->automator_state == OVERRIDE_OFF && fixture->on != on) {
    fixture_switch(fixture, on);
  }
  if (on) {
    fixture->automator_state =
        OVERRIDE_OFF;  // May have been on due to override but now it's just
                       // normally on (won't expire)
  }
}

static void sched_child(Fixture* fixture,
                        Fixture* parent,
                        int on_prob,
                        int off_prob) {
  if (!parent->on) {
    if (fixture->automator_state == OVERRIDE_ON) {
      fixture->automator_state = OVERRIDE_OFF;
      fixture_switch(fixture, false);
    }
    return;
  }

  if (fixture->on && rand() % off_prob == 1) {
    fixture_switch(fixture, false);
    fixture->automator_state = OVERRIDE_OFF;
  } else if (!fixture->on && rand() % on_prob == 1) {
    fixture_switch(fixture, true);
    fixture->automator_state = OVERRIDE_ON;
  }
}

static void bookshop_task(void* pvParameters) {
  FixtureGroup* group = (FixtureGroup*)pvParameters;

  Fixture* attic = group->fixtures[0];
  Fixture* living = group->fixtures[1];
  Fixture* shop = group->fixtures[2];
  Fixture* lantern = group->fixtures[3];
  bool inited = false;

  while (true) {
    struct timeval now;
    gettimeofday(&now, NULL);
    int t = now.tv_sec % 86400;
    if (t < 65) {
      inited = false;
    }

    if (group->manual) {
      vTaskDelay(5000 / portTICK_RATE_MS);
      continue;
    }

    sched_timed(lantern, t, hm(6, 40), hm(22, 0), 0, 0);

    // Rooms
    static int t_wake, t_go_down, t_open_shop, t_bed_off, t_liv_off,
        t_close_shop, t_liv_on, t_sleep, t_go_up, t_bed_on;
    if (!inited) {
      t_wake = rand_range(hm(7, 30), hm(8, 25));
      t_go_down = rand_range(t_wake, t_wake + hm(0, 10));
      t_open_shop = rand_range(t_go_down, t_go_down + hm(0, 10));
      t_bed_off = rand_range(t_go_down + hm(0, 5), t_go_down + hm(0, 55));
      t_liv_off = rand_range(t_open_shop, t_open_shop + hm(0, 1));
      t_close_shop = rand_range(hm(17, 30), hm(18, 30));
      t_liv_on = rand_range(hm(17, 0), t_close_shop);
      t_sleep = rand_range(hm(22, 0), hm(23, 0));
      t_go_up = rand_range(t_sleep - hm(0, 30), t_sleep - hm(1, 0));
      t_bed_on = rand_range(t_go_up - hm(0, 1), t_go_up);
      inited = true;
    }

    sched_timed(attic, t, t_wake, t_bed_off, t_bed_on, t_sleep);
    sched_timed(living, t, t_go_down, t_liv_off, t_liv_on, t_go_up);
    sched_timed(shop, t, t_open_shop, t_close_shop, 0, 0);

    sched_child(attic, living, 90, 30);
    sched_child(shop, living, 90, 30);
    sched_child(living, attic, 90, 30);
    sched_child(living, shop, 90, 30);

    vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

static TaskHandle_t init_bookshop_automator(FixtureGroup* group) {
  TaskHandle_t hnd = NULL;
  xTaskCreate(bookshop_task, "bookshop_task", 4096, (void*)group, 5, &hnd);
  return hnd;
}

static void townhouse_task(void* pvParameters) {
  FixtureGroup* group = (FixtureGroup*)pvParameters;

  Fixture* bedroom = group->fixtures[0];
  Fixture* hallway = group->fixtures[1];
  Fixture* fire = group->fixtures[2];
  Fixture* door = group->fixtures[3];
  bool inited = false;

  while (true) {
    struct timeval now;
    gettimeofday(&now, NULL);
    int t = now.tv_sec % 86400;
    if (t < 65) {
      inited = false;
    }

    if (group->manual) {
      vTaskDelay(5000 / portTICK_RATE_MS);
      continue;
    }

    sched_timed(door, t, hm(16, 30), hm(22, 10), 0, 0);
    sched_timed(hallway, t, hm(7, 30), hm(20, 0), 0, 0);

    // Rooms
    static int t_wake, t_fire, t_bed_off, t_go_up, t_sleep;
    if (!inited) {
      t_wake = rand_range(hm(6, 40), hm(7, 0));
      t_fire = rand_range(t_wake + hm(0, 1), t_wake + hm(0, 10));
      t_bed_off = rand_range(t_fire, t_fire + hm(0, 5));
      t_go_up = rand_range(hm(21, 30), hm(22, 00));
      t_sleep = rand_range(t_go_up + hm(0, 5), t_go_up + hm(0, 30));
      inited = true;
    }

    sched_timed(bedroom, t, t_wake, t_bed_off, t_go_up, t_sleep);
    sched_timed(fire, t, t_fire, t_go_up, 0, 0);

    sched_child(bedroom, fire, 90, 30);

    vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

static TaskHandle_t init_townhouse_automator(FixtureGroup* group) {
  TaskHandle_t hnd = NULL;
  xTaskCreate(townhouse_task, "townhouse_task", 4096, (void*)group, 5, &hnd);
  return hnd;
}

static void police_task(void* pvParameters) {
  FixtureGroup* group = (FixtureGroup*)pvParameters;

  Fixture* antenna = group->fixtures[0];
  Fixture* integ_room = group->fixtures[1];
  Fixture* loo = group->fixtures[2];
  Fixture* landing = group->fixtures[3];
  Fixture* sit_room = group->fixtures[4];
  Fixture* desk_lh = group->fixtures[5];
  Fixture* desk_rh = group->fixtures[6];
  Fixture* reception = group->fixtures[7];
  Fixture* cell = group->fixtures[8];
  Fixture* lanterns = group->fixtures[9];
  bool inited = false;

  while (true) {
    struct timeval now;
    gettimeofday(&now, NULL);
    int t = now.tv_sec % 86400;
    if (t < 65) {
      inited = false;
    }

    if (group->manual) {
      vTaskDelay(5000 / portTICK_RATE_MS);
      continue;
    }

    fixture_switch(antenna, true);  // Always on
    sched_timed(lanterns, t, hm(8, 0), hm(19, 0), 0, 0);

    // Rooms
    static int t_open, t_close, t_sit, t_sit_off, t_desks_on, t_lh_home,
        t_rh_home, t_last_desk, t_cell_on, t_cell_off, t_cell_on2, t_cell_off2;
    if (!inited) {
      t_open = rand_range(hm(7, 59) - hm(0, 4), hm(7, 59));
      t_close = rand_range(hm(19, 0), hm(19, 0) + hm(0, 15));
      t_sit = rand_range(hm(8, 59) - hm(0, 4), hm(8, 59));
      t_sit_off = rand_range(hm(17, 30), hm(18, 15));
      t_desks_on = rand_range(t_sit_off - hm(0, 5), t_sit_off + hm(0, 1));
      t_lh_home = rand_range(t_desks_on, t_desks_on + hm(3, 0));
      t_rh_home = rand_range(t_desks_on, t_desks_on + hm(3, 0));
      t_last_desk = t_lh_home;
      if (t_rh_home > t_last_desk) {
        t_last_desk = t_rh_home;
      }
      t_cell_on = rand_range(hm(7, 0), hm(9, 0));
      t_cell_off = rand_range(hm(9, 1), hm(15, 0));
      t_cell_on2 = rand_range(hm(16, 0), hm(18, 0));
      t_cell_off2 = rand_range(hm(22, 1), hm(23, 30));
      inited = true;
    }

    sched_timed(reception, t, t_open, t_close, 0, 0);
    sched_timed(sit_room, t, t_sit, t_sit_off, 0, 0);
    sched_timed(desk_lh, t, t_desks_on, t_lh_home, 0, 0);
    sched_timed(desk_rh, t, t_desks_on, t_rh_home, 0, 0);
    sched_timed(landing, t, t_sit - 30, t_last_desk + 30, 0, 0);
    sched_timed(cell, t, t_cell_on, t_cell_off, t_cell_on2, t_cell_off2);

    sched_child(desk_lh, sit_room, 200, 100);
    sched_child(desk_rh, sit_room, 200, 100);
    sched_child(integ_room, sit_room, 120, 80);
    sched_child(loo, landing, 90, 30);

    vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

static TaskHandle_t init_police_automator(FixtureGroup* group) {
  TaskHandle_t hnd = NULL;
  xTaskCreate(police_task, "police_task", 4096, (void*)group, 5, &hnd);
  return hnd;
}

static void donut_task(void* pvParameters) {
  FixtureGroup* group = (FixtureGroup*)pvParameters;

  Fixture* flat = group->fixtures[0];
  Fixture* shop = group->fixtures[1];
  bool inited = false;

  while (true) {
    struct timeval now;
    gettimeofday(&now, NULL);
    int t = now.tv_sec % 86400;
    if (t < 65) {
      inited = false;
    }

    if (group->manual) {
      vTaskDelay(5000 / portTICK_RATE_MS);
      continue;
    }

    static int t_wake, t_open_shop, t_bed_off, t_go_up, t_shop_off, t_sleep;
    if (!inited) {
      t_wake = rand_range(hm(7, 0), hm(7, 5));
      t_open_shop = rand_range(hm(7, 18), hm(7, 28));
      t_bed_off = rand_range(t_open_shop + hm(0, 1), t_open_shop + hm(0, 5));
      t_go_up = rand_range(hm(15, 55), hm(16, 5));
      t_shop_off = rand_range(t_go_up + hm(0, 1), t_go_up + hm(0, 5));
      t_sleep = rand_range(hm(21, 0), hm(21, 45));
      inited = true;
    }

    sched_timed(flat, t, t_wake, t_bed_off, t_go_up, t_sleep);
    sched_timed(shop, t, t_open_shop, t_shop_off, 0, 0);

    sched_child(flat, shop, 90, 30);
    sched_child(shop, flat, 90, 30);

    vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

static TaskHandle_t init_donut_automator(FixtureGroup* group) {
  TaskHandle_t hnd = NULL;
  xTaskCreate(donut_task, "donut_task", 4096, (void*)group, 5, &hnd);
  return hnd;
}

static void street_task(void* pvParameters) {
  FixtureGroup* group = (FixtureGroup*)pvParameters;

  Fixture* streetlamp_l = group->fixtures[0];
  Fixture* streetlamp_r = group->fixtures[1];
  Fixture* newsstand = group->fixtures[2];
  Fixture* billboard = group->fixtures[3];

  while (true) {
    struct timeval now;
    gettimeofday(&now, NULL);
    int t = now.tv_sec % 86400;

    if (group->manual) {
      vTaskDelay(5000 / portTICK_RATE_MS);
      continue;
    }

    sched_timed(streetlamp_l, t, hm(7, 0), hm(8, 30), hm(16, 30), hm(22, 30));
    sched_timed(streetlamp_r, t, hm(7, 0), hm(8, 30), hm(16, 30), hm(22, 30));
    sched_timed(newsstand, t, hm(7, 30), hm(20, 0), 0, 0);
    sched_timed(billboard, t, hm(6, 30), hm(9, 0), hm(16, 0), hm(22, 45));

    vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

static TaskHandle_t init_street_automator(FixtureGroup* group) {
  TaskHandle_t hnd = NULL;
  xTaskCreate(street_task, "street_task", 4096, (void*)group, 5, &hnd);
  return hnd;
}

void automation_init() {
  Rig* rig = rig_get();

  for (int i = 0; i < rig->n_groups; i++) {
    FixtureGroup* group = rig->groups[i];

    for (int i = 0; i < group->n_fixtures; i++) {
      group->fixtures[i]->automator_state = OVERRIDE_OFF;
    }

    if (group->automator == AlwaysOn) {
      for (int j = 0; j < group->n_fixtures; j++) {
        fixture_switch(group->fixtures[j], true);
      }
    } else if (group->automator == AlwaysOff) {
      for (int j = 0; j < group->n_fixtures; j++) {
        fixture_switch(group->fixtures[j], false);
      }
    } else if (group->automator == StreetAutomator) {
      init_street_automator(group);
    } else if (group->automator == BookshopAutomator) {
      init_bookshop_automator(group);
    } else if (group->automator == TownhouseAutomator) {
      init_townhouse_automator(group);
    } else if (group->automator == DoughnutShopAutomator) {
      init_donut_automator(group);
    } else if (group->automator == PoliceStationAutomator) {
      init_police_automator(group);
    }
  }
}