/**
 * @file wificonfigtest.c
 * @brief End-to-end Wi-Fi configuration test for Amiga and other targets.
 *
 * This deliberately exercises the public API, including caller-owned scan
 * storage, rather than reaching into the wire transport.
 */

#include "fujinet-nio.h"

#include <stdio.h>
#include <string.h>

#define SCAN_CAPACITY 2
#define SCAN_RESPONSE_CAPACITY \
    (FN_WIFI_SCAN_RESPONSE_HEADER_SIZE + \
     SCAN_CAPACITY * FN_WIFI_SCAN_RECORD_WIRE_MAX)

static int failures;

static void pass(const char *name)
{
    printf("%s OK\n", name);
}

static void fail(const char *name, const char *detail)
{
    printf("%s FAIL %s\n", name, detail);
    failures++;
}

static int result_ok(const char *name, uint8_t result)
{
    if (result == FN_OK)
        return 1;
    printf("%s FAIL %u(%s)\n", name, (unsigned)result,
           fn_error_string(result));
    failures++;
    return 0;
}

int main(void)
{
    fn_wifi_config_t config;
    fn_wifi_config_update_t update;
    fn_wifi_status_t status;
    fn_wifi_scan_record_t records[SCAN_CAPACITY];
    uint8_t response[SCAN_RESPONSE_CAPACITY];
    uint8_t count = 0;
    uint8_t more = 0;
    static const char *ssid = "amiga-e2e-network";
    static const char *bssid = "02:04:06:08:0A:0C";
    static const char *password = "amiga-e2e-password";

    puts("WIFICFGTEST START");
    if (!result_ok("INIT", fn_init()))
        return 1;

    memset(&update, 0, sizeof(update));
    update.fields = FN_WIFI_SET_ENABLED | FN_WIFI_SET_SSID |
                    FN_WIFI_SET_BSSID | FN_WIFI_SET_PASSWORD |
                    FN_WIFI_SET_PERSIST | FN_WIFI_SET_RECONNECT;
    update.enabled = 1;
    update.ssid = ssid;
    update.bssid = bssid;
    update.password = password;
    if (result_ok("SET", fn_wifi_set_config(&update)))
        pass("SET");

    if (result_ok("GET", fn_wifi_get_config(&config))) {
        if (!config.enabled || !config.password_present)
            fail("GET_FIELDS", "enabled/password");
        else if (strcmp(config.ssid, ssid) != 0)
            fail("GET_SSID", config.ssid);
        else if (strcmp(config.bssid, bssid) != 0)
            fail("GET_BSSID", config.bssid);
        else
            pass("GET");
    }

    if (result_ok("STATUS", fn_wifi_get_status(&status))) {
        if (status.ip[0] == 0 || status.gateway[0] == 0 || status.dns[0] == 0)
            fail("STATUS_DETAILS", "missing IP details");
        else
            pass("STATUS");
    }

    if (result_ok("SCAN", fn_wifi_scan(0, SCAN_CAPACITY, records,
                                        SCAN_CAPACITY, &count, &more,
                                        response, sizeof(response)))) {
        if (count == 0)
            fail("SCAN_RECORDS", "no records");
        else
            pass("SCAN");
    }

    printf("WIFICFGTEST PASS=%u FAIL=%u\n", failures ? 0U : 1U,
           (unsigned)failures);
    return failures ? 1 : 0;
}
