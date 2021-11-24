// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_err.h"
// #include "esp_log.h"

#include "e131.h"
#include "leds.h"
#include "rig.h"
#include "webapp.h"
#include "wifi.h"

// static const char *TAG = "MAIN";

void app_main()
{
    // uint8_t addr = chain_addr();
    // board_type_t board_type = chain_type();

    // if (board_type == LEGO_LEADER) {
    wifi_init();
    wifi_connect();
    //     server_connect();
    // }
    // else if (board_type == RAILWAY_LEADER) {
    //     softap_init();
    // }

    e131_init();
    // chain_init();
    leds_init();
    rig_init();

    // if (addr == 0) {
    //     sched_run();
    webapp_start();
    // }
    // else {
    //     chain_follower_run();
    // }
}
