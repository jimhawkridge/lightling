#define TLC_CHANS 32  // Must be a multiple of 2

void tlc_init();
void tlc_fade_channel(int channel, uint8_t brightness);