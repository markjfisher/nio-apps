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
extern uint8_t fn_calc_checksum_asm_branch(const uint8_t *, uint16_t);

struct Device *TimerBase;
static uint8_t benchmark_buffer[4096];

#ifdef __KICK13__
typedef uint32_t benchmark_ticks_t;
#else
typedef uint64_t benchmark_ticks_t;
#endif

static int benchmark_clock(struct timerequest *request, benchmark_ticks_t *ticks)
{
#ifdef __KICK13__
    /*
     * timer.device V34 has the TR_GETSYSTIME command, but not the later
     * ReadEClock() library-vector entry.  Calling that absent vector enters
     * arbitrary code and leaves the CLI task held.  The command returns the
     * same monotonic system time with microsecond resolution.
     */
    request->tr_node.io_Command = TR_GETSYSTIME;
    if (DoIO((struct IORequest *)request) != 0)
        return 0;
    *ticks = (request->tr_time.tv_secs * 1000000UL) +
             request->tr_time.tv_micro;
    return 1;
#else
    struct EClockVal value;

    if (TimerBase->dd_Library.lib_Version < 36) {
        request->tr_node.io_Command = TR_GETSYSTIME;
        if (DoIO((struct IORequest *)request) != 0)
            return 0;
        *ticks = (request->tr_time.tv_secs * 1000000UL) +
                 request->tr_time.tv_micro;
        return 1;
    }

    *ticks = ((uint64_t)ReadEClock(&value) << 32) | value.ev_lo;
    return 1;
#endif
}

static uint32_t microseconds_per_iteration(benchmark_ticks_t elapsed,
                                           uint32_t count, uint32_t frequency)
{
#ifdef __KICK13__
    (void)frequency;
    return elapsed / count;
#else
    return (uint32_t)(((uint64_t)elapsed * 1000000ULL) /
                      ((uint64_t)frequency * count));
#endif
}

static int run_case(uint16_t size, uint32_t count, uint32_t frequency,
                    uint8_t *buffer, struct timerequest *timer_request)
{
    uint8_t expected, result;
    benchmark_ticks_t start, end, elapsed;
    uint32_t i;

    for (i = 0; i < size; ++i)
        buffer[i] = (uint8_t)(i * 37U + size);

    expected = fn_calc_checksum(buffer, size);

    /*
     * C implementation.
     */
    if (!benchmark_clock(timer_request, &start))
        return 0;

    for (i = 0; i < count; ++i) {
        result = fn_calc_checksum(buffer, size);
        if (result != expected)
            return 0;
    }

    if (!benchmark_clock(timer_request, &end))
        return 0;

    elapsed = end - start;

    printf("%5u %8lu C %10lu %10lu %10lu\n",
           size,
           (unsigned long)count,
           (unsigned long)elapsed,
           (unsigned long)(elapsed / count),
           (unsigned long)microseconds_per_iteration(elapsed, count, frequency));

    /*
     * ADDX assembler implementation.
     */
    if (!benchmark_clock(timer_request, &start))
        return 0;

    for (i = 0; i < count; ++i) {
        result = fn_calc_checksum_asm(buffer, size);

        if (result != expected) {
            printf("ASM mismatch size=%u expected=%u got=%u\n",
                   (unsigned)size,
                   (unsigned)expected,
                   (unsigned)result);
            return 0;
        }
    }

    if (!benchmark_clock(timer_request, &end))
        return 0;

    elapsed = end - start;

    printf("%5u %8lu A %10lu %10lu %10lu\n",
           size,
           (unsigned long)count,
           (unsigned long)elapsed,
           (unsigned long)(elapsed / count),
           (unsigned long)microseconds_per_iteration(elapsed, count, frequency));

    /*
     * Branch-on-carry assembler implementation.
     */
    if (!benchmark_clock(timer_request, &start))
        return 0;

    for (i = 0; i < count; ++i) {
        result = fn_calc_checksum_asm_branch(buffer, size);

        if (result != expected) {
            printf("BRANCH mismatch size=%u expected=%u got=%u\n",
                   (unsigned)size,
                   (unsigned)expected,
                   (unsigned)result);
            return 0;
        }
    }

    if (!benchmark_clock(timer_request, &end))
        return 0;

    elapsed = end - start;

    printf("%5u %8lu B %10lu %10lu %10lu\n",
           size,
           (unsigned long)count,
           (unsigned long)elapsed,
           (unsigned long)(elapsed / count),
           (unsigned long)microseconds_per_iteration(elapsed, count, frequency));

    return 1;
}

int main(void)
{
    static const uint16_t sizes[] = {
        16, 64, 256, 512, 1024, 4096
    };

#ifdef __KICK13__
    /* A real 7 MHz A500 executes the same coverage in a practical E2E time. */
    static const uint32_t counts[] = {
        2000, 1000, 400, 200, 100, 25
    };
#else
    static const uint32_t counts[] = {
        20000, 10000, 4000, 2000, 1000, 250
    };
#endif

    struct MsgPort *port = NULL;
    struct timerequest *request = NULL;
    uint32_t frequency;
    unsigned i;
    int ok = 1;
    BOOL timer_open = FALSE;

    port = CreatePort(NULL, 0);

    if (port != NULL)
        request = (struct timerequest *)CreateExtIO(port, sizeof(*request));

    if (request == NULL ||
        OpenDevice((CONST_STRPTR)TIMERNAME,
                   UNIT_MICROHZ,
                   (struct IORequest *)request,
                   0) != 0) {
        printf("checksumbench: timer.device unavailable\n");
        goto cleanup;
    }

    timer_open = TRUE;
#ifdef __KICK13__
    frequency = 1000000UL;
#else
    {
        struct EClockVal eclock;

        TimerBase = (struct Device *)request->tr_node.io_Device;
        frequency = TimerBase->dd_Library.lib_Version >= 36
                  ? ReadEClock(&eclock)
                  : 1000000UL;
    }
#endif

#ifdef __KICK13__
    printf("Checksum benchmark (timer.device system clock %lu Hz)\n",
           (unsigned long)frequency);
#else
    printf("Checksum benchmark (EClock %lu Hz)\n",
           (unsigned long)frequency);
#endif

    printf(" bytes iterations mode      ticks ticks/iter usec/iter\n");

    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        if (!run_case(sizes[i],
                      counts[i],
                      frequency,
                      benchmark_buffer,
                      request)) {
            ok = 0;
        }
    }

cleanup:
    if (timer_open)
        CloseDevice((struct IORequest *)request);

    if (request != NULL)
        DeleteExtIO((struct IORequest *)request);

    if (port != NULL)
        DeletePort(port);

    TimerBase = NULL;

    return ok ? 0 : 1;
}
