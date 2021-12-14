// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_err.h"
// #include "esp_log.h"

#include "chain.h"
#include "e131.h"
#include "leds.h"
#include "rig.h"
#include "webapp.h"
#include "wifi.h"

// static const char *TAG = "MAIN";

void app_main()
{
    board_type_t board_type = chain_type();

    if (board_type == LEADER)
    {
        wifi_init();
        wifi_connect();
        e131_init();

        chain_init();
        leds_init();
        rig_init();

        webapp_start();
    }
    else
    {
        chain_follower_run();
    }
}
