#include <stdio.h>
#include "pka_wifi.h"
#include "pka_aws.h"
#include "pka_ntp.h"

#include "nvs_flash.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/pcnt.h"
#include <time.h>

#define AWS_IOT_PUBLISH_TOPIC "mojevec/elektrika"
#define AWS_IOT_SUBSCRIBE_TOPIC "mojevec/sub"

QueueHandle_t pcnt_evt_queueL1; // A queue to handle pulse counter events
QueueHandle_t pcnt_evt_queueL2;
QueueHandle_t pcnt_evt_queueL3;

/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
typedef struct
{
  int unit;        // the PCNT unit that originated an interrupt
  uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;

#define PCNT_H_LIM_VAL 32767
#define PCNT_L_LIM_VAL -32767

/* Initialize PCNT functions:
 *  - configure and initialize PCNT
 *  - set up the input filter
 *  - set up the counter events to watch
 */
static void pcnt_example_init(int unit, int pcntChannel, int gpio, int ctlGpio)
{
  /* Prepare configuration for the PCNT unit */
  pcnt_config_t pcnt_config = {
      // Set PCNT input signal and control GPIOs
      .pulse_gpio_num = gpio,
      .ctrl_gpio_num = ctlGpio,
      .channel = pcntChannel,
      .unit = unit,
      // What to do on the positive / negative edge of pulse input?
      .pos_mode = PCNT_COUNT_INC, // Count up on the positive edge
      .neg_mode = PCNT_MODE_KEEP, // Keep the counter value on the negative edge
      // What to do when control input is low or high?
      .hctrl_mode = PCNT_MODE_KEEP, // Keep the primary counter mode if high
      .lctrl_mode = PCNT_MODE_KEEP, // Reverse counting direction if low

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

static const char *TAG = "aws-pulse-counter";


// return 1 on failure, 0 on success 
  int tm_YearWeek(const struct tm *tmptr, int *year, int *week)
  {
    // work with local copy
    struct tm tm = *tmptr;
    // fully populate the yday and wday fields.
    if (mktime(&tm) == -1)
    {
      return 1;
    }

    // Find day-of-the-week: 0 to 6.
    // Week starts on Monday per ISO 8601
    // 0 <= DayOfTheWeek <= 6, Monday, Tuesday ... Sunday
    int DayOfTheWeek = (tm.tm_wday + (7 - 1)) % 7;

    // Offset the month day to the Monday of the week.
    tm.tm_mday -= DayOfTheWeek;
    // Offset the month day to the mid-week (Thursday) of the week, 3 days later.
    tm.tm_mday += 3;
    // Re-evaluate tm_year and tm_yday  (local time)
    if (mktime(&tm) == -1)
    {
      return 1;
    }

    *year = tm.tm_year + 1900;
    // Convert yday to week of the year, stating with 1.
    *week = tm.tm_yday / 7 + 1;
    return 0;
  }



void app_main(void)
{
  ESP_LOGI(TAG, "[APP] Startup..");
  ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  pka_wifi_init();
  pka_aws_init();
  pka_initialize_sntp();
  checkNTPUpdated();

  pcnt_example_init(PCNT_UNIT_0, PCNT_CHANNEL_0, 16,0);
  pcnt_example_init(PCNT_UNIT_1, PCNT_CHANNEL_0, 17,2);
  pcnt_example_init(PCNT_UNIT_2, PCNT_CHANNEL_0, 18,4);
  int16_t countL1 = 0;
  int16_t countL2 = 0;
  int16_t countL3 = 0;
  esp_log_level_set("*", ESP_LOG_NONE);
  while (/* condition */ 1)
  {
    /* code */
    vTaskDelay(60000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Sending .....");
  
    pcnt_get_counter_value(PCNT_UNIT_0, &countL1);
    pcnt_get_counter_value(PCNT_UNIT_1, &countL2);
    pcnt_get_counter_value(PCNT_UNIT_2, &countL3);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_1);
    pcnt_counter_clear(PCNT_UNIT_2);
    ESP_LOGI(TAG, "My Current counter value L1:%d L2:%d L3: %d", countL1, countL2, countL3);

    cJSON *root;
    root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "l1S0Count", countL1);
    cJSON_AddNumberToObject(root, "l2S0Count", countL2);
    cJSON_AddNumberToObject(root, "l3S0Count", countL3);
    cJSON_AddNumberToObject(root, "tarif", gpio_get_level(5));


    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    int myNow = now;

/*    char buffer[100];
    strftime(buffer, sizeof(buffer), "%FT%T%z", &timeinfo);
    cJSON_AddStringToObject(root, "timestamp", buffer);
    cJSON_AddNumberToObject(root, "dayOfWeek", timeinfo.tm_wday);
    cJSON_AddNumberToObject(root, "month", timeinfo.tm_mon);*/
    cJSON_AddNumberToObject(root, "now", myNow);
    /*
    cJSON_AddNumberToObject(root, "freeHeap", esp_get_free_heap_size() );
    
    int y = 0, w = 0;
    int err = tm_YearWeek(&timeinfo, &y, &w);
    int ww = w; 
    
    char myWeek[8];

    sprintf( myWeek, "%d-W%d",y,w);
    cJSON_AddStringToObject(root, "myWeek", myWeek);

    cJSON_AddNumberToObject(root, "weekOfYear", ww);
    cJSON_AddNumberToObject(root, "year", y);
*/
    char *my_json_string = cJSON_Print(root);
    ESP_LOGI(TAG, "my_json_string\n%s", my_json_string);
    cJSON_Delete(root);
    ESP_LOGI(TAG, "free my_json_string");
    pka_aws_publish("mojevec/elektrika", my_json_string);
    free(my_json_string);
  }

  
}