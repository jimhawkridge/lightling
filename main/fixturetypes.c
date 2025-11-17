#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "control.h"
#include "rig.h"

static const char* TAG = "FIX";

typedef enum FirePhaseEnum { OUT, WARMUP, STEADY, COOLDOWN } FirePhase;

typedef struct {
  FirePhase phase;
  int warmth;
} FireFixtureState;

typedef struct {
  int frame;
} FluoroFixtureState;

typedef struct {
  int max_between;
  int max_flicks;
  int remaining_flicks;
  int which_chan;
} FlickeringFixtureState;

typedef struct {
  int offtime;
  int ontime;
  bool state;
} BlinkingFixtureState;

void* initFireFixture(Fixture* f) {
  FireFixtureState* s = malloc(sizeof(FireFixtureState));
  s->phase = OUT;
  s->warmth = 0;
  return (void*)s;
}

void fire_switcher(void* f, bool on, bool fast) {
  Fixture* fixture = (Fixture*)f;
  FireFixtureState* ffs = (FireFixtureState*)fixture->state;
  ffs->phase = on ? WARMUP : COOLDOWN;
  if (on && ffs->warmth < 7) {
    ffs->warmth = 7;
  } else if (!on) {
    ffs->warmth /= 2;
  }
  fixture->on = on;

  if (fast) {
    ffs->phase = OUT;
    for (int i = 0; i < fixture->n_channels; i++) {
      control_fade_channel(fixture->channels[i], 0, 50);
    }
  }
}

void* initFluoroFixture(Fixture* f) {
  FluoroFixtureState* s = malloc(sizeof(FluoroFixtureState));
  s->frame = 0;
  return (void*)s;
}

void fluoro_switcher(void* f, bool on, bool fast) {
  Fixture* fixture = (Fixture*)f;
  fixture->on = on;
  ((FluoroFixtureState*)fixture->state)->frame = 0;
  if (!on) {
    for (int i = 0; i < fixture->n_channels; i++) {
      control_fade_channel(fixture->channels[i], 0, 50);
    }
  }
}

void* initFlickeringFixture(Fixture* f) {
  FlickeringFixtureState* s = malloc(sizeof(FlickeringFixtureState));
  s->max_between = 5000;
  s->max_flicks = 4;
  s->remaining_flicks = 0;
  s->which_chan = 0;
  return (void*)s;
}

static void blinking_fixture_task(void* pvParameters) {
  Fixture* f = (Fixture*)pvParameters;
  BlinkingFixtureState* state = (BlinkingFixtureState*)f->state;
  while (true) {
    if (f->on) {
      for (int i = 0; i < f->n_channels; i++) {
        int level = state->state ? f->levels[i] : 0;
        control_fade_channel(f->channels[i], level, 0);
      }
    }
    if (state->state) {
      vTaskDelay(state->ontime / portTICK_RATE_MS);
    } else {
      vTaskDelay(state->offtime / portTICK_RATE_MS);
    }
    state->state = state->state ? false : true;
  }
}

void* initBlinkingFixture(Fixture* f) {
  BlinkingFixtureState* s = malloc(sizeof(BlinkingFixtureState));
  s->offtime = 300;
  s->ontime = 1000;
  s->state = false;
  return (void*)s;
}

static void flickering_fixture_task(void* pvParameters) {
  ESP_LOGI(TAG, "Initialising flickering fixture task");
  Fixture* f = (Fixture*)pvParameters;
  FlickeringFixtureState* state = (FlickeringFixtureState*)f->state;
  while (true) {
    if (f->on) {
      for (int i = 0; i < f->n_channels; i++) {
        control_fade_channel(f->channels[i], f->levels[i], 0);
      }
    }
    int wait = esp_random() % state->max_between;
    vTaskDelay(wait / portTICK_RATE_MS);
    if (f->on) {
      int chan = esp_random() % f->n_channels;
      int flicks = (esp_random() % state->max_flicks) + 1;
      for (int i = 0; i < flicks; i++) {
        control_fade_channel(f->channels[chan], f->levels[chan] / 2, 0);
        vTaskDelay(50 / portTICK_RATE_MS);
        control_fade_channel(f->channels[chan], f->levels[chan], 0);
        vTaskDelay(50 / portTICK_RATE_MS);
      }
    }
  }
}

static void fire_fixture_task(void* pvParameters) {
  Fixture* f = (Fixture*)pvParameters;
  FireFixtureState* state = (FireFixtureState*)f->state;
  while (true) {
    vTaskDelay(1000 / portTICK_RATE_MS);
    while (state->phase != OUT) {
      if (state->phase == WARMUP) {
        state->warmth++;
        if (state->warmth == 255) {
          state->phase = STEADY;
        }
      } else if (state->phase == COOLDOWN) {
        state->warmth--;
        if (state->warmth == 0) {
          state->phase = OUT;
        }
      }
      for (int i = 0; i < f->n_channels; i++) {
        int half_max = state->warmth * f->levels[i] / 255;
        int level = half_max == 0 ? 0 : (esp_random() % half_max + half_max);
        control_fade_channel(f->channels[i], level, 200);
      }
      vTaskDelay(200 / portTICK_RATE_MS);
    }
  }
}

static void fluoro_fixture_task(void* pvParameters) {
  Fixture* f = (Fixture*)pvParameters;
  FluoroFixtureState* state = (FluoroFixtureState*)f->state;
  while (true) {
    vTaskDelay(500 / portTICK_RATE_MS);
    while (f->on && state->frame <= 60) {
      for (int i = 0; i < f->n_channels; i++) {
        bool on = state->frame > 30 ? esp_random() % 10 : esp_random() % 3;
        if (state->frame > 50 + (8 * i) % 10) {  // Try to stagger final on time
          on = true;
        }
        control_fade_channel(f->channels[i], on ? f->levels[i] : 0, 0);
      }
      state->frame++;
      vTaskDelay(50 / portTICK_RATE_MS);
    }
  }
}

static TaskHandle_t initFixtureTask(Fixture* f) {
  TaskFunction_t task;
  switch (f->fixture_type) {
    case Blinking:
      task = blinking_fixture_task;
      break;
    case Flickering:
      task = flickering_fixture_task;
      break;
    case Fire:
      task = fire_fixture_task;
      break;
    case Fluoro:
      task = fluoro_fixture_task;
      break;
    default:
      task = NULL;
  }

  TaskHandle_t hnd = NULL;
  if (task != NULL) {
    xTaskCreate(task, "fixture_task", 4096, (void*)f, 5, &hnd);
  }
  return hnd;
}

void default_switcher(void* f, bool on, bool fast) {
  Fixture* fixture = (Fixture*)f;
  for (int i = 0; i < fixture->n_channels; i++) {
    uint8_t brightness = on ? fixture->levels[i] : 0;
    control_fade_channel(fixture->channels[i], brightness, 50);
  }
  fixture->on = on;
}

// Switches everything off, when it's off time. But leaves it to the task to
// deal with on.
void off_only_switcher(void* f, bool on, bool fast) {
  Fixture* fixture = (Fixture*)f;
  if (!on) {
    for (int i = 0; i < fixture->n_channels; i++) {
      uint8_t brightness = 0;
      control_fade_channel(fixture->channels[i], brightness, 50);
    }
  }
  fixture->on = on;
}

switcher_t getFixtureSwitcher(Fixture* f) {
  switch (f->fixture_type) {
    case Fire:
      return fire_switcher;
    case Fluoro:
      return fluoro_switcher;
    case Blinking:
      return off_only_switcher;
    case Flickering:
    default:
      return default_switcher;
  }
}

void fixture_switch(Fixture* f, bool on) {
  f->switcher(f, on, false);
}

void fixture_switch_off_fast(Fixture* f) {
  f->switcher(f, false, true);
}

void fixture_switch_ids(uint8_t group_id, uint8_t fixture_id, bool on) {
  Rig* rig = rig_get();
  if (rig == NULL || group_id > rig->n_groups ||
      fixture_id > rig->groups[group_id]->n_fixtures) {
    return;
  }

  Fixture* f = rig->groups[group_id]->fixtures[fixture_id];
  fixture_switch(f, on);
}

void fixturetypes_init_fixture(Fixture* f) {
  void* state = NULL;
  switch (f->fixture_type) {
    case Fire:
      state = initFireFixture(f);
      break;
    case Fluoro:
      state = initFluoroFixture(f);
      break;
    case Flickering:
      state = initFlickeringFixture(f);
      break;
    case Blinking:
      state = initBlinkingFixture(f);
      break;
    default:
      break;
  }
  f->state = state;

  switcher_t switcher = getFixtureSwitcher(f);
  f->switcher = switcher;

  TaskHandle_t task = initFixtureTask(f);
  f->task = task;
}