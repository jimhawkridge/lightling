// #include <string.h>
// #include "driver/i2c.h"
// #include "esp_log.h"
// #include "esp_mac.h"

// #include "chain.h"
// #include "leds.h"

// static const char* TAG = "I2C";

// static const int I2C_PORT = 0;
// static const uint8_t ID_1 = 0x43;
// static const uint8_t ID_2 = 0x34;
// static const uint8_t I2C_FADE_CMD = 0x01;
// #define I2C_SLAVE_TX_BUF_LEN 1024
// #define I2C_SLAVE_RX_BUF_LEN 1024

// static const uint8_t CHAIN_CHANS = 16 * 3;

// static int8_t addr = -1;
// static board_type_t type = -1;
// static void set_board_info() {
//   if (addr == -1) {
//     uint8_t chipid[6];
//     esp_read_mac(chipid, 0);
//     for (int i = 0; i < 6; i++) {
//       ESP_LOGI(TAG, "ChipID is %02x", chipid[i]);
//       // Jim 1| ...:b0
//       // Jim 2| ...:ac
//       // Dad 1| ...:44
//       // Dad 2| ...:a0
//       // Broke| ...:9c
//     }
//     switch (chipid[5]) {
//       case 0xb0:
//       case 0x44:
//         addr = 0;
//         type = LEADER;
//         break;
//       case 0xac:
//       case 0xa0:
//         addr = 1;
//         type = FOLLOWER;
//         break;
//       default:
//         addr = 15;
//         type = FOLLOWER;
//         break;
//     }
//     ESP_LOGI(TAG, "Type is %d | Addr is %d", type, addr);
//   }
// }

// uint8_t chain_addr() {
//   set_board_info();
//   return addr;
// }

// board_type_t chain_type() {
//   set_board_info();
//   return type;
// }

// void chain_leader_init() {
//   ESP_LOGI(TAG, "Chain leader init");
//   i2c_config_t conf;
//   memset(&conf, 0, sizeof(conf));
//   conf.mode = I2C_MODE_MASTER;
//   conf.sda_io_num = GPIO_NUM_21;
//   conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
//   conf.scl_io_num = GPIO_NUM_22;
//   conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
//   conf.master.clk_speed = 100000;
//   conf.clk_flags = 0;
//   i2c_param_config(I2C_PORT, &conf);
//   ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0));
// }

// void chain_follower_init() {
//   ESP_LOGI(TAG, "Chain follower init");
//   i2c_config_t conf;
//   memset(&conf, 0, sizeof(conf));
//   conf.mode = I2C_MODE_SLAVE;
//   conf.sda_io_num = GPIO_NUM_21;
//   conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
//   conf.scl_io_num = GPIO_NUM_22;
//   conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
//   conf.slave.addr_10bit_en = 0;
//   conf.slave.slave_addr = chain_addr();
//   conf.clk_flags = 0;
//   i2c_param_config(I2C_PORT, &conf);
//   ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, conf.mode,
//   I2C_SLAVE_RX_BUF_LEN,
//                                      I2C_SLAVE_TX_BUF_LEN, 0));
// }

// typedef struct __attribute__((__packed__)) {
//   uint8_t chan;
//   uint8_t level;
//   uint16_t time;
// } fade_cmd_t;

// static void chain_send_fade(uint8_t device,
//                             uint8_t chan,
//                             uint8_t level,
//                             uint16_t time) {
//   ESP_LOGI(TAG, "Request to device %d for fade of %d to %d over %d", device,
//            chan, level, time);
//   fade_cmd_t data;
//   data.chan = chan;
//   data.level = level;
//   data.time = time;

//   i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//   i2c_master_start(cmd);
//   i2c_master_write_byte(cmd, device << 1 | I2C_MASTER_WRITE, false);
//   i2c_master_write_byte(cmd, ID_1, false);
//   i2c_master_write_byte(cmd, ID_2, false);
//   i2c_master_write_byte(cmd, I2C_FADE_CMD, false);
//   i2c_master_write(cmd, (uint8_t*)&data, sizeof(data), false);
//   i2c_master_stop(cmd);
//   esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 /
//   portTICK_RATE_MS); i2c_cmd_link_delete(cmd); if (ret != ESP_OK) {
//     ESP_LOGE(TAG, "I2C Error");
//     return;
//   }
// }

// static int action_from_data(uint8_t* data, int len) {
//   if (len < 3) {
//     ESP_LOGE(TAG, "Short! Len is %d", len);
//   }
//   if (data[0] != ID_1 || data[1] != ID_2) {
//     ESP_LOGE(TAG, "No ID");
//     return 0;
//   }
//   switch (data[2]) {
//     case I2C_FADE_CMD:
//       ESP_LOGI(TAG, "Fade CMD");
//       uint8_t total_len = 3 + sizeof(fade_cmd_t);
//       if (len < total_len) {
//         ESP_LOGE(TAG, "Data short! Len is %d", len);
//       }
//       fade_cmd_t* cmd = (fade_cmd_t*)(data + 3);
//       ESP_LOGI(TAG, "Fade %d to %d over %d", cmd->chan, cmd->level,
//       cmd->time); leds_fade_channel(cmd->chan, cmd->level, cmd->time); return
//       len - total_len;
//   }
//   return 0;
// }

// static uint8_t rx_buf[I2C_SLAVE_RX_BUF_LEN];
// static void chain_follower_poll() {
//   int len = i2c_slave_read_buffer(I2C_PORT, rx_buf, I2C_SLAVE_RX_BUF_LEN,
//   10); if (len == -1) {
//     ESP_LOGE(TAG, "Error reading follower buffer");
//     return;
//   }
//   if (len == 0) {
//     return;
//   }

//   ESP_LOGI(TAG, "-------------------------");
//   for (int i = 0; i < len; i++) {
//     ESP_LOGI(TAG, "Payload was %02X", rx_buf[i]);
//   }

//   int left = len;
//   while (left > 0) {
//     left = action_from_data(rx_buf + (len - left), left);
//   }
// }

// void chain_follower_run() {
//   ESP_LOGI(TAG, "Waitinging");
//   while (true) {
//     chain_follower_poll();
//   }
// }

// void chain_init() {
//   if (chain_addr() == 0) {
//     chain_leader_init();
//   } else {
//     chain_follower_init();
//   }
// }

// void chain_fade_channel(uint8_t channel, uint8_t brightness, uint16_t time) {
//   if (channel < LED_CHANS) {
//     leds_fade_channel(channel, brightness, time);
//     return;
//   }
//   uint8_t target_addr = channel / LED_CHANS;
//   uint8_t target_chan = channel % LED_CHANS;
//   ESP_LOGI(TAG, "Request to channel %d on device %d", target_chan,
//   target_addr); chain_send_fade(target_addr, target_chan, brightness, time);
// }

// void chain_set_channel(uint8_t channel, uint8_t brightness) {
//   chain_fade_channel(channel, brightness, 50);
// }

// void chain_set_channels(int n_chans, uint8_t* brightness) {
//   if (n_chans > CHAIN_CHANS) {
//     n_chans = CHAIN_CHANS;
//   }

//   for (int i = 0; i < n_chans; i++) {
//     chain_set_channel(i, brightness[i]);
//   }
//   for (int i = 0; i < n_chans; i++) {
//     chain_set_channel(i, brightness[i]);
//   }
// }