#include <stdint.h>
#include <stdio.h>

#ifdef __ATARI__
#include <conio.h>
#include <peekpoke.h>

#define DDEVIC 0x0300
#define DUNIT  0x0301
#define DCOMND 0x0302
#define DSTATS 0x0303
#define DBUFLO 0x0304
#define DBUFHI 0x0305
#define DTIMLO 0x0306
#define DUNUSE 0x0307
#define DBYTLO 0x0308
#define DBYTHI 0x0309
#define DAUX1  0x030A
#define DAUX2  0x030B

#define SIOV ((void (*)(void))0xE459)

static uint8_t write_buf[33] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20
};

static void print_dcb(const char *label)
{
    printf("%s\n", label);
    printf("dev=%02x unit=%u cmd=%c dst=%02x\n",
           PEEK(DDEVIC), PEEK(DUNIT), PEEK(DCOMND), PEEK(DSTATS));
    printf("buf=%02x%02x bytes=%u aux=%u tim=%u\n",
           PEEK(DBUFHI), PEEK(DBUFLO),
           (unsigned)(PEEK(DBYTLO) | (PEEK(DBYTHI) << 8)),
           (unsigned)(PEEK(DAUX1) | (PEEK(DAUX2) << 8)),
           PEEK(DTIMLO));
}

int main(void)
{
    uint16_t addr = (uint16_t)write_buf;
    uint8_t i;

    clrscr();
    puts("NIO RAW SIO WRITE");
    printf("buf addr=%04x len=33\n", addr);
    printf("first=");
    for (i = 0; i < 8; ++i) {
        printf(" %02x", write_buf[i]);
    }
    puts("");

    POKE(DDEVIC, 0x71);
    POKE(DUNIT, 1);
    POKE(DCOMND, 'W');
    POKE(DSTATS, 0x80);
    POKE(DBUFLO, addr & 0xFF);
    POKE(DBUFHI, addr >> 8);
    POKE(DTIMLO, 60);
    POKE(DUNUSE, 0);
    POKE(DBYTLO, 33);
    POKE(DBYTHI, 0);
    POKE(DAUX1, 33);
    POKE(DAUX2, 0);

    print_dcb("before");
    SIOV();
    print_dcb("after");

    puts("press key");
    cgetc();
    return 0;
}
#else
int main(void)
{
    puts("fsioraw is Atari-only");
    return 1;
}
#endif
