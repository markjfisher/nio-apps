/**
 * @file clock.c
 * @brief FujiNet Clock Device Example for nio-apps
 * 
 * Demonstrates how to use the fujinet-nio-lib clock device functions.
 * This example shows how to:
 *   - Get the current time from FujiNet in various formats
 *   - Set the time on FujiNet
 *   - Get/Set timezone
 *   - Display time strings formatted by FujiNet
 * 
 * Usage:
 *   Set environment variables to configure:
 *     FN_PORT     - Serial port device (default: /dev/ttyUSB0)
 * 
 * Build from nio-apps with:
 *   make TARGET=msdos clock
 *   make TARGET=atari clock
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fujinet-nio.h"

/**
 * Create a time value from a Unix timestamp.
 */
static void make_time(FN_TIME_T *time, uint32_t timestamp)
{
#ifdef __CC65__
    uint8_t i;
    for (i = 0; i < 4; i++) {
        time->b[i] = (uint8_t)((timestamp >> (8 * i)) & 0xFF);
    }
    /* Zero the high bytes */
    for (i = 4; i < 8; i++) {
        time->b[i] = 0;
    }
#else
    *time = (uint64_t)timestamp;
#endif
}

/**
 * Print raw time bytes for debugging.
 */
static void print_time_raw(FN_TIME_T *time)
{
    uint8_t i;
#ifndef __CC65__
    uint64_t t = *time;
#endif

    printf("Raw bytes: ");
#ifdef __CC65__
    for (i = 0; i < 8; i++) {
        printf("%02X ", time->b[i]);
    }
#else
    for (i = 0; i < 8; i++) {
        printf("%02X ", (unsigned)((t >> (8 * i)) & 0xFF));
    }
#endif
    printf("\n");
}

/* ============================================================================
 * Main Program
 * ============================================================================
 */

int main(void)
{
    uint8_t result;
    FN_TIME_T current_time;
    FN_TIME_T test_time;
    char iso_time[FN_MAX_TIME_STRING];
    char tz_buf[FN_MAX_TIMEZONE_LEN];
    uint8_t binary_time[16];
    uint8_t i;
    
    printf("FujiNet-NIO Clock Device Example\n");
    printf("================================\n\n");
    
    /* Initialize the library */
    printf("Initializing...\n");
    result = fn_init();
    if (result != FN_OK) {
        printf("Init failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    /* Check if device is ready */
    if (!fn_is_ready()) {
        printf("FujiNet device not ready!\n");
        return 1;
    }
    
    printf("Device ready.\n\n");
    
    /* ========================================================================
     * Test 1: Get current time as FujiNet-formatted UTC ISO
     * ======================================================================== */
    printf("--- Test 1: Get Current Time (UTC ISO) ---\n");
    result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_UTC_ISO);
    if (result != FN_OK) {
        printf("Failed to get time: %s\n", fn_error_string(result));
        if (result == FN_ERR_NOT_READY) {
            printf("(Time may not be synchronized - check WiFi/NTP)\n");
        }
    } else {
        printf("UTC time: %s\n", iso_time);
    }
    printf("\n");

    /* ========================================================================
     * Test 2: Get current time (raw format)
     * ======================================================================== */
    printf("--- Test 2: Get Current Time (Raw) ---\n");
    result = fn_clock_get(&current_time);
    if (result != FN_OK) {
        printf("Failed to get time: %s\n", fn_error_string(result));
        if (result == FN_ERR_NOT_READY) {
            printf("(Time may not be synchronized - check WiFi/NTP)\n");
        }
    } else {
        print_time_raw(&current_time);
    }
    printf("\n");
    
    /* ========================================================================
     * Test 3: Get time in ISO 8601 format (with timezone)
     * ======================================================================== */
    printf("--- Test 3: Get Time (ISO 8601 with TZ) ---\n");
    result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_TZ_ISO);
    if (result == FN_OK) {
        printf("Local time: %s\n", iso_time);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 4: Get current timezone
     * ======================================================================== */
    printf("--- Test 4: Get Current Timezone ---\n");
    result = fn_clock_get_timezone(tz_buf);
    if (result == FN_OK) {
        printf("Current timezone: %s\n", tz_buf);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 5: Get time in binary formats
     * ======================================================================== */
    printf("--- Test 5: Binary Formats ---\n");
    
    /* Simple binary (7 bytes) */
    result = fn_clock_get_format(binary_time, FN_TIME_FORMAT_SIMPLE);
    if (result == FN_OK) {
        printf("Simple binary (7 bytes): ");
        for (i = 0; i < 7; i++) {
            printf("%02X ", binary_time[i]);
        }
        printf("\n");
        printf("  -> %04d-%02d-%02d %02d:%02d:%02d\n",
               (int)binary_time[0] * 100 + (int)binary_time[1],
               (int)binary_time[2], (int)binary_time[3],
               (int)binary_time[4], (int)binary_time[5], (int)binary_time[6]);
    }
    
    /* ProDOS binary (4 bytes) */
    result = fn_clock_get_format(binary_time, FN_TIME_FORMAT_PRODOS);
    if (result == FN_OK) {
        printf("ProDOS binary (4 bytes): ");
        for (i = 0; i < 4; i++) {
            printf("%02X ", binary_time[i]);
        }
        printf("\n");
    }
    
    /* ApeTime binary (6 bytes) */
    result = fn_clock_get_format(binary_time, FN_TIME_FORMAT_APETIME);
    if (result == FN_OK) {
        printf("ApeTime binary (6 bytes): ");
        for (i = 0; i < 6; i++) {
            printf("%02X ", binary_time[i]);
        }
        printf("\n");
    }
    printf("\n");
    
    /* ========================================================================
     * Test 6: Get time for a specific timezone
     * ======================================================================== */
    printf("--- Test 6: Time for Specific Timezone ---\n");
    
    /* Try Pacific Time */
    result = fn_clock_get_tz((uint8_t*)iso_time, "PST8PDT,M3.2.0,M11.1.0", FN_TIME_FORMAT_TZ_ISO);
    if (result == FN_OK) {
        printf("Pacific Time: %s\n", iso_time);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    
    /* Try Central European Time */
    result = fn_clock_get_tz((uint8_t*)iso_time, "CET-1CEST,M3.5.0,M10.5.0/3", FN_TIME_FORMAT_TZ_ISO);
    if (result == FN_OK) {
        printf("Central European Time: %s\n", iso_time);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 7: Set timezone (non-persistent)
     * ======================================================================== */
    printf("--- Test 7: Set Timezone (non-persistent) ---\n");
    result = fn_clock_set_timezone("EST5EDT,M3.2.0,M11.1.0");
    if (result == FN_OK) {
        printf("Timezone set to EST5EDT\n");
        
        /* Get time in new timezone */
        result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_TZ_ISO);
        if (result == FN_OK) {
            printf("Eastern Time: %s\n", iso_time);
        }
        
        /* Get current timezone to verify */
        result = fn_clock_get_timezone(tz_buf);
        if (result == FN_OK) {
            printf("Current timezone is now: %s\n", tz_buf);
        }
    } else {
        printf("Failed to set timezone: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 8: Set time (demonstration only)
     * ======================================================================== */
    printf("--- Test 8: Set Time (demonstration) ---\n");
    /* Set time to a test value (2024-01-01 00:00:00 UTC = 1704067200) */
    /* Note: This is just for demonstration - setting the time may not */
    /* be allowed on all FujiNet configurations */
    make_time(&test_time, 1704067200UL);  /* 2024-01-01 00:00:00 UTC */
    printf("Setting time to: 2024-01-01 00:00:00 UTC\n");
    
    result = fn_clock_set(&test_time);
    if (result != FN_OK) {
        printf("Failed to set time: %s\n", fn_error_string(result));
        printf("(This may be expected if time setting is disabled)\n");
    } else {
        printf("Time set successfully.\n");
    }
    
    /* Get the time back to verify */
    result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_UTC_ISO);
    if (result == FN_OK) {
        printf("UTC time: %s\n", iso_time);
    }
    printf("\n");
    
    /* ========================================================================
     * Test 9: Sync network time (restore from NTP)
     * ======================================================================== */
    printf("--- Test 9: Sync Network Time (restore from NTP) ---\n");
    printf("Requesting time sync from network...\n");
    
    result = fn_clock_sync_network_time(&current_time);
    if (result != FN_OK) {
        printf("Failed to sync time: %s\n", fn_error_string(result));
        printf("(This may fail if network is not available)\n");
    } else {
        printf("Time synchronized from network.\n");
        result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_UTC_ISO);
        if (result == FN_OK) {
            printf("UTC time: %s\n", iso_time);
        }
    }
    printf("\n");
    
    printf("Done.\n");
    return 0;
}
