#include <stdio.h>

#include "esp_event.h"
#include "esp_log.h"
#include "lwip/sockets.h"

static const char *TAG = "E131";

#define E131_PORT 5568
#define UNIVERSE 2

typedef struct __attribute__((__packed__))
{
    uint8_t padding1[113];
    uint16_t universe;
    uint8_t padding2[8];
    uint16_t chan_count;
    uint8_t start_code;
    uint8_t chan_data[512];
} E131_packet;

static void e131_listener(void *pvParameters)
{
    char rx_buffer[768];
    char addr_str[768];
    int addr_family;
    int ip_protocol;

    while (1)
    {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(E131_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", E131_PORT);

        while (1)
        {
            struct sockaddr_in6 source_addr;
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            else
            {
                E131_packet *packet = (E131_packet *)rx_buffer;
                if (ntohs(packet->universe) == UNIVERSE)
                {
                    // chain_set_channels(packet->chan_count, packet->chan_data);
                }
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

static void e131_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "Starting e131 listener");
        xTaskCreate(e131_listener, "e131_listener", 4096, NULL, 5, NULL);
    }
}

void e131_init()
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &e131_event_handler, NULL));
}