/**
 * @file wifitest.c
 * @brief Small Wi-Fi API example and smoke test.
 *
 * This app intentionally uses caller-owned storage for the scan response. It
 * is useful as a platform build test and as a minimal example for clients.
 * It does not change the saved Wi-Fi configuration.
 */

#include "fujinet-nio.h"

#include <stdio.h>
#include <string.h>

#define WIFI_TEST_SCAN_CAPACITY 1
#define WIFI_TEST_RESPONSE_CAPACITY \
    (3 + WIFI_TEST_SCAN_CAPACITY * (1 + FN_WIFI_MAX_SSID + 9))

static void print_bssid(const fn_wifi_bssid_t *bssid)
{
    unsigned i;

    if (!bssid->valid) {
        puts("(none)");
        return;
    }
    for (i = 0; i < 6; ++i)
        printf("%s%02X", i ? ":" : "", (unsigned)bssid->bytes[i]);
    putchar('\n');
}

static int report_result(const char *operation, uint8_t result)
{
    if (result == FN_OK)
        return 0;
    printf("%s: %u (%s)\n", operation, (unsigned)result,
           fn_error_string(result));
    return 1;
}

int main(void)
{
    fn_wifi_status_t status;
    fn_wifi_config_t config;
    fn_wifi_scan_record_t record[WIFI_TEST_SCAN_CAPACITY];
    uint8_t response[WIFI_TEST_RESPONSE_CAPACITY];
    uint8_t count = 0;
    uint8_t more = 0;
    uint8_t result;
    int failures = 0;

    puts("FujiNet-NIO Wi-Fi API test");

    result = fn_init();
    if (report_result("init", result))
        return 1;

    result = fn_wifi_get_status(&status);
    if (report_result("get status", result)) {
        failures++;
    } else {
        printf("link=%u enabled=%u RSSI=%d\n",
               (unsigned)status.link_state,
               (unsigned)status.configured_enabled,
               (int)status.rssi);
        printf("IP=%s gateway=%s DNS=%s\n",
               status.ip, status.gateway, status.dns);
        printf("BSSID=");
        print_bssid(&status.bssid);
    }

    result = fn_wifi_get_config(&config);
    if (report_result("get config", result)) {
        failures++;
    } else {
        printf("SSID=%s BSSID=%s password=%s\n",
               config.ssid, config.bssid,
               config.password_present ? "set" : "not set");
    }

    result = fn_wifi_scan(0, WIFI_TEST_SCAN_CAPACITY, record,
                          WIFI_TEST_SCAN_CAPACITY, &count, &more,
                          response, sizeof(response));
    if (report_result("scan", result)) {
        failures++;
    } else {
        printf("scan count=%u more=%u\n", (unsigned)count, (unsigned)more);
        if (count != 0) {
            printf("  %s channel=%u RSSI=%d BSSID=",
                   record[0].ssid, (unsigned)record[0].channel,
                   (int)record[0].rssi);
            print_bssid(&record[0].bssid);
        }
    }

    puts(failures ? "Wi-Fi API test failed" : "Wi-Fi API test passed");
    return failures ? 1 : 0;
}
