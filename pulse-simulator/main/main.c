#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define PULSE_GPIO 18
static const char *TAG = "pulse-simulator";

void app_main(void)
{

    gpio_reset_pin(PULSE_GPIO);
    gpio_set_direction(PULSE_GPIO, GPIO_MODE_OUTPUT);
    long pulses = 0;
    while (1)
    {
        ESP_LOGI(TAG, "Setting to low");
        gpio_set_level(PULSE_GPIO, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Setting to high");
        gpio_set_level(PULSE_GPIO, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        pulses++;
        ESP_LOGI(TAG, "Pulses total:  %ld", pulses);
    }
}
