#include <stdio.h>

#define LED_CHANS 16

void leds_fade_channel(int channel, uint8_t brightness, uint32_t time);
void leds_init();