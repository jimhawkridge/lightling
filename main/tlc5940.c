#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

#include "tlc5940.h"

#define SCLK 18
#define MOSI 23
#define XLAT 21
#define GSCLK 22
#define BLANK 19

// GPIO 2 is a strapping pin

static const char* TAG = "TLC";

TaskHandle_t tlc_gsclk_task_handle;
TaskHandle_t tlc_spi_task_handle;

uint16_t frame[TLC_CHANS];

void tlc_fade_channel(int channel, uint8_t brightness) {
  frame[channel] = brightness * 4;
  xTaskNotifyGive(tlc_spi_task_handle);
}

bool tx_cb(rmt_channel_handle_t tx_chan,
           const rmt_tx_done_event_data_t* edata,
           void* user_ctx) {
  ESP_ERROR_CHECK(gpio_set_level(BLANK, 1));
  ets_delay_us(5);
  ESP_ERROR_CHECK(gpio_set_level(BLANK, 0));
  // xTaskNotifyFromISR(tlc_gsclk_task_handle, 0, eNoAction, NULL);
  xTaskNotifyGive(tlc_gsclk_task_handle);
  return false;
};

void send_gsclck() {
  static rmt_channel_handle_t gsclck_chan = NULL;
  static rmt_encoder_handle_t gsclck_copy_enc;

  if (gsclck_chan == NULL) {
    rmt_tx_channel_config_t gsclck_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1 * 1000 * 1000,
        .mem_block_symbols = 64,
        .gpio_num = GSCLK,
        .trans_queue_depth = 10,
        .flags =
            {
                .invert_out = 0,
                .with_dma = 0,
                .io_loop_back = 0,
                .io_od_mode = 0,
            },
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&gsclck_chan_config, &gsclck_chan));
    ESP_LOGI(TAG, "GSCLCK tx chan initd");

    rmt_carrier_config_t gsclck_carrier_conf = {
        .frequency_hz = 1 * 1000 * 1000,
        .duty_cycle = 0.5,
        .flags =
            {
                .polarity_active_low = 0,
                .always_on = 0,
            },
    };
    ESP_ERROR_CHECK(rmt_apply_carrier(gsclck_chan, &gsclck_carrier_conf));
    ESP_LOGI(TAG, "GSCLCK carrier initd");

    rmt_tx_event_callbacks_t gsclck_cbs = {.on_trans_done = tx_cb};
    rmt_tx_register_event_callbacks(gsclck_chan, &gsclck_cbs, NULL);
    ESP_LOGI(TAG, "GSCLCK tx chan callbacks initd");

    ESP_ERROR_CHECK(rmt_enable(gsclck_chan));
    ESP_LOGI(TAG, "GSCLCK tx chan enabled");

    rmt_copy_encoder_config_t gsclck_copy_enc_conf;
    ESP_ERROR_CHECK(
        rmt_new_copy_encoder(&gsclck_copy_enc_conf, &gsclck_copy_enc));
    ESP_LOGI(TAG, "GSCLCK tx chan encoder prepared");
  }

  static const uint16_t gsclck_payload[2] = {4096 | 0x8000, 0};
  static const rmt_transmit_config_t gsclck_trans_conf = {
      .loop_count = 0,
  };

  ESP_ERROR_CHECK(rmt_transmit(gsclck_chan, gsclck_copy_enc, gsclck_payload, 8,
                               &gsclck_trans_conf));
}

static void tlc_gsclk_task(void* pvParameters) {
  ESP_LOGI(TAG, "tlc_gsclk_task started");
  while (true) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
    send_gsclck();
  };
}

static void tlc_spi_task(void* pvParameters) {
  ESP_LOGI(TAG, "tlc_spi_task started");

  spi_bus_config_t bus_config = {
      .mosi_io_num = MOSI,
      .sclk_io_num = SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "SPI bus initd");

  spi_device_interface_config_t dev_config = {
      .clock_speed_hz = 100 * 1000,
      .mode = 0,
      .spics_io_num = -1,
      .queue_size = 1,
      .address_bits = 0,
      .command_bits = 0,
  };
  spi_device_handle_t spi_hnd;
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_config, &spi_hnd));

  ESP_LOGI(TAG, "SPI device added to bus");

  const int send_buffer_len = TLC_CHANS * 1.5;
  char send_buffer[send_buffer_len];

  while (true) {
    int send_idx = 0;
    for (int i = 0; i < TLC_CHANS; i += 2) {
      uint16_t a = frame[i];
      uint16_t b = frame[i + 1];
      uint16_t low = (a & 0x0ff0) >> 4;
      uint16_t mid = ((a & 0x000f) << 4) | ((b & 0x0f00) >> 8);
      uint16_t high = b & 0x00ff;

      send_buffer[send_idx] = (char)low;
      send_buffer[send_idx + 1] = (char)mid;
      send_buffer[send_idx + 2] = (char)high;
      send_idx += 3;
    }

    spi_transaction_t trans = {
        .flags = 0,
        .length = send_buffer_len * 8,  // This is in bits
        .tx_buffer = send_buffer,
        .rx_buffer = NULL,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi_hnd, &trans));

    ESP_ERROR_CHECK(gpio_set_level(XLAT, 1));
    ets_delay_us(1);
    ESP_ERROR_CHECK(gpio_set_level(XLAT, 0));

    vTaskDelay(1 / portTICK_RATE_MS);
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
  }
}

void tlc_init() {
  gpio_config_t xlat_config = {
      .pin_bit_mask = 1ULL << XLAT,
      .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&xlat_config));
  ESP_ERROR_CHECK(gpio_set_level(XLAT, 0));

  gpio_config_t blank_config = {
      .pin_bit_mask = 1ULL << BLANK,
      .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&blank_config));
  ESP_ERROR_CHECK(gpio_set_level(BLANK, 0));

  ESP_ERROR_CHECK(gpio_set_level(XLAT, 1));
  send_gsclck();
  ESP_ERROR_CHECK(gpio_set_level(XLAT, 0));

  xTaskCreate(tlc_gsclk_task, "tlc_gsclk_task", 4096, NULL, 5,
              &tlc_gsclk_task_handle);
  xTaskCreate(tlc_spi_task, "tlc_spi_task", 4096, NULL, 5,
              &tlc_spi_task_handle);
}
