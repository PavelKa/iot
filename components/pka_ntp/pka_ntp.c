#include <stdio.h>
#include "pka_ntp.h"
#include "esp_sntp.h"



void pka_initialize_sntp(void)
{

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "europe.pool.ntp.org");
    sntp_setservername(2, "uk.pool.ntp.org ");
    sntp_setservername(3, "us.pool.ntp.org");
    sntp_setservername(4, "time1.google.com");
    sntp_init();
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
}

void checkNTPUpdated(void)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    while (timeinfo.tm_year < (2019 - 1900))
    {
        printf("Time not set, trying...\n");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    static char buffer[100];
    strftime(buffer, sizeof(buffer), "%FT%T%z", &timeinfo);
    return buffer;
}
