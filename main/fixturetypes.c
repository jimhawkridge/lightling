#include "rig.h"

typedef enum FirePhaseEnum { OUT, WARMUP, STEADY, COOLDOWN } FirePhase;

typedef struct {
  FirePhase phase;
  int warmth;
} FireFixtureState;

typedef struct {
  int frame;
} FluroFixtureState;

typedef struct {
  int percent;
} FadeFixtureState;

typedef struct {
} FlickeringFixtureState;

void* initFireFixture(Fixture* f) {
  FireFixtureState* s = malloc(sizeof(FireFixtureState));
  s->phase = OUT;
  s->warmth = 0;
  return (void*)s;
}

void switchFireFixture(Fixture* f, bool on) {
  ((FireFixtureState*)f->state)->phase = on ? WARMUP : COOLDOWN;
}

void* initFluroFixture(Fixture* f) {
  FluroFixtureState* s = malloc(sizeof(FluroFixtureState));
  s->frame = 0;
  return (void*)s;
}

void switchFluroFixture(Fixture* f) {
  ((FluroFixtureState*)f->state)->frame = 0;
}

void* initFadeFixture(Fixture* f) {
  FadeFixtureState* s = malloc(sizeof(FadeFixtureState));
  s->percent = 0;
  return (void*)s;
}

void* initFlickeringFixture(Fixture* f) {
  return NULL;
}

void initFixture(Fixture* f) {
  void* state = NULL;

  switch (f->fixture_type) {
    case Fire:
      state = initFireFixture(f);
      break;
    case Fluoro:
      state = initFluroFixture(f);
      break;
    case Fade:
      state = initFadeFixture(f);
      break;
    case Flickering:
      state = initFlickeringFixture(f);
      break;

    default:
      break;
  }

  f->state = state;
  //   initFixtureTask(t);
}