#include "nio.h"
#include "serial.h"

#include <stdio.h>
#include <stdlib.h>

static uint8_t sector[1024];

static void print_usage(void)
{
  puts("Usage: NIOREAD [slot] [lba] [bytes] [com]");
  puts("  slot defaults to 1");
  puts("  lba defaults to 0");
  puts("  bytes defaults to 512");
  puts("  com defaults to 1");
}

static void hexdump(const uint8_t *buf, uint16_t len)
{
  uint16_t row;
  uint16_t col;

  for (row = 0; row < len; row += 16) {
    printf("%04X  ", row);
    for (col = 0; col < 16; col++) {
      if (row + col < len)
        printf("%02X ", buf[row + col]);
      else
        printf("   ");
    }
    printf(" ");
    for (col = 0; col < 16 && row + col < len; col++) {
      uint8_t ch = buf[row + col];
      putchar((ch >= 32 && ch <= 126) ? ch : '.');
    }
    puts("");
  }
}

int main(int argc, char **argv)
{
  uint8_t slot = 1;
  uint32_t lba = 0;
  unsigned bytes = 512;
  unsigned com = 1;
  uint16_t base;
  uint16_t bytes_read = 0;
  nio_response_t response;

  if (argc > 5 || (argc > 1 && argv[1][0] == '?')) {
    print_usage();
    return 1;
  }

  if (argc > 1)
    slot = (uint8_t) atoi(argv[1]);
  if (argc > 2)
    lba = strtoul(argv[2], 0, 0);
  if (argc > 3)
    bytes = (unsigned) atoi(argv[3]);
  if (argc > 4)
    com = (unsigned) atoi(argv[4]);

  if (bytes == 0 || bytes > sizeof(sector)) {
    print_usage();
    return 1;
  }

  base = serial_com_base(com);
  if (!base) {
    print_usage();
    return 1;
  }

  printf("NIOREAD slot %u lba %lu bytes %u COM%u\r\n",
         (unsigned) slot, lba, bytes, com);
  serial_init(base, 1);

  if (!nio_disk_read_sector(slot, lba, sector, (uint16_t) bytes, &bytes_read, &response)) {
    printf("Read failed");
    if (response.status != NIO_STATUS_INTERNAL_ERROR)
      printf(": status %u (%s)", response.status, nio_status_name(response.status));
    puts("");
    return 2;
  }

  printf("status: %u (%s)\r\n", response.status, nio_status_name(response.status));
  printf("bytes read: %u\r\n", bytes_read);
  hexdump(sector, bytes_read);

  return 0;
}
