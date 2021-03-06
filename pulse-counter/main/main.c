/* Pulse counter module - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "driver/pcnt.h"
#include "esp_attr.h"
#include "esp_log.h"

static const char *TAG = "example";

/**
 * TEST CODE BRIEF
 *
 * Use PCNT module to count rising edges generated by LEDC module.
 *
 * Functionality of GPIOs used in this example:
 *   - GPIO18 - output pin of a sample 1 Hz pulse generator,
 *   - GPIO4 - pulse input pin,
 *   - GPIO5 - control input pin.
 *
 * Load example, open a serial port to view the message printed on your screen.
 *
 * To do this test, you should connect GPIO18 with GPIO4.
 * GPIO5 is the control signal, you can leave it floating with internal pull up,
 * or connect it to ground. If left floating, the count value will be increasing.
 * If you connect GPIO5 to GND, the count value will be decreasing.
 *
 * An interrupt will be triggered when the counter value:
 *   - reaches 'thresh1' or 'thresh0' value,
 *   - reaches 'l_lim' value or 'h_lim' value,
 *   - will be reset to zero.
 */
#define PCNT_H_LIM_VAL      32767
#define PCNT_L_LIM_VAL     -32767
#define PCNT_THRESH1_VAL    5
#define PCNT_THRESH0_VAL   -5
#define PCNT_INPUT_SIG_IO   4  // Pulse Input GPIO
#define PCNT_INPUT_CTRL_IO  5  // Control GPIO HIGH=count up, LOW=count down
#define LEDC_OUTPUT_IO      18 // Output GPIO of a sample 1 Hz pulse generator

QueueHandle_t pcnt_evt_queue;   // A queue to handle pulse counter events

/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;


/* Initialize PCNT functions:
 *  - configure and initialize PCNT
 *  - set up the input filter
 *  - set up the counter events to watch
 */
static void pcnt_example_init(int unit) {
    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
            // Set PCNT input signal and control GPIOs
            .pulse_gpio_num = PCNT_INPUT_SIG_IO,
            .ctrl_gpio_num = PCNT_INPUT_CTRL_IO,
            .channel = PCNT_CHANNEL_0,
            .unit = unit,
            // What to do on the positive / negative edge of pulse input?
            .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
            .neg_mode = PCNT_MODE_KEEP,   // Keep the counter value on the negative edge
            // What to do when control input is low or high?
            .lctrl_mode = PCNT_COUNT_INC, // Reverse counting direction if low
            .hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
            // Set the maximum and minimum limit values to watch
            .counter_h_lim = PCNT_H_LIM_VAL,
            .counter_l_lim = PCNT_L_LIM_VAL,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);


    /* Configure and enable the input filter */
    pcnt_set_filter_value(unit, 85);
    pcnt_filter_enable(unit);


    /* Initialize PCNT's counter */
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);


    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit);
}

void app_main(void) {
    int pcnt_unit = PCNT_UNIT_0;
    /* Initialize LEDC to generate sample pulse signal */


    /* Initialize PCNT event queue and PCNT functions */
    pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));
    pcnt_example_init(pcnt_unit);

    int16_t count = 0;
    pcnt_evt_t evt;
    portBASE_TYPE res;
    while (1) {
        /* Wait for the event information passed from PCNT's interrupt handler.
         * Once received, decode the event type and print it on the serial monitor.
         */
        res = xQueueReceive(pcnt_evt_queue, &evt, 1000 / portTICK_PERIOD_MS);
        if (res == pdTRUE) {
            pcnt_get_counter_value(pcnt_unit, &count);
            ESP_LOGI(TAG, "Event PCNT unit[%d]; cnt: %d", evt.unit, count);
            if (evt.status & PCNT_EVT_THRES_1) {
                ESP_LOGI(TAG, "THRES1 EVT");
            }
            if (evt.status & PCNT_EVT_THRES_0) {
                ESP_LOGI(TAG, "THRES0 EVT");
            }
            if (evt.status & PCNT_EVT_L_LIM) {
                ESP_LOGI(TAG, "L_LIM EVT");
            }
            if (evt.status & PCNT_EVT_H_LIM) {
                ESP_LOGI(TAG, "H_LIM EVT");
            }
            if (evt.status & PCNT_EVT_ZERO) {
                ESP_LOGI(TAG, "ZERO EVT");
            }
        } else {
            pcnt_get_counter_value(pcnt_unit, &count);
            ESP_LOGI(TAG, "MY Current counter value :%d", count);
        }
    }
}
