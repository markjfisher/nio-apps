#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <stdio.h>
#include <stdint.h>

#include "fn_protocol.h"

extern uint8_t fn_calc_checksum_asm(const uint8_t *, uint16_t);
struct Device *TimerBase;

static uint64_t eclock_ticks(const struct EClockVal *v)
{ return ((uint64_t)v->ev_hi << 32) | v->ev_lo; }

static int run_case(uint16_t size, uint32_t count, uint32_t frequency,
                    uint8_t *buffer)
{
    struct EClockVal start, end;
    uint8_t expected, result;
    uint64_t elapsed;
    uint32_t i;
    for (i = 0; i < size; ++i) buffer[i] = (uint8_t)(i * 37U + size);
    expected = fn_calc_checksum(buffer, size);
    ReadEClock(&start);
    for (i = 0; i < count; ++i) {
        result = fn_calc_checksum(buffer, size);
        if (result != expected) return 0;
    }
    ReadEClock(&end);
    elapsed = eclock_ticks(&end) - eclock_ticks(&start);
    printf("%5u %8lu C %10lu %10lu %10lu\n", size, (unsigned long)count,
           (unsigned long)elapsed, (unsigned long)(elapsed / count),
           (unsigned long)((elapsed * 1000000ULL) / ((uint64_t)frequency * count)));
    ReadEClock(&start);
    for (i = 0; i < count; ++i) {
        result = fn_calc_checksum_asm(buffer, size);
        if (result != expected) return 0;
    }
    ReadEClock(&end);
    elapsed = eclock_ticks(&end) - eclock_ticks(&start);
    printf("%5u %8lu A %10lu %10lu %10lu\n", size, (unsigned long)count,
           (unsigned long)elapsed, (unsigned long)(elapsed / count),
           (unsigned long)((elapsed * 1000000ULL) / ((uint64_t)frequency * count)));
    return 1;
}

int main(void)
{
    static const uint16_t sizes[] = {16, 64, 256, 512, 1024, 4096};
    static const uint32_t counts[] = {20000, 10000, 4000, 2000, 1000, 250};
    struct MsgPort *port = CreatePort(NULL, 0);
    struct timerequest *request = port == NULL ? NULL :
        (struct timerequest *)CreateExtIO(port, sizeof(*request));
    struct EClockVal eclock;
    uint8_t buffer[4096];
    uint32_t frequency;
    unsigned i;
    int ok = 1;
    if (request == NULL || OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_MICROHZ,
                                      (struct IORequest *)request, 0) != 0) {
        printf("checksumbench: timer.device unavailable\n");
        return 1;
    }
    TimerBase = (struct Device *)request->tr_node.io_Device;
    frequency = ReadEClock(&eclock);
    printf("Checksum benchmark (EClock %lu Hz)\n", (unsigned long)frequency);
    printf(" bytes iterations mode      ticks ticks/iter usec/iter\n");
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
        if (!run_case(sizes[i], counts[i], frequency, buffer)) ok = 0;
    CloseDevice((struct IORequest *)request);
    DeleteExtIO((struct IORequest *)request);
    DeletePort(port);
    return ok ? 0 : 1;
}
