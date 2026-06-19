#include "nio.h"
#include "serial.h"

#include <stdio.h>
#include <stdlib.h>

static void print_usage(void)
{
  puts("Usage: NIOPROBE [slot] [com]");
  puts("  slot defaults to 1");
  puts("  com defaults to 1");
}

static void print_flags(uint8_t flags)
{
  printf("flags: 0x%02X", flags);
  if (flags & NIO_DISK_INFO_INSERTED)
    printf(" inserted");
  if (flags & NIO_DISK_INFO_READONLY)
    printf(" readonly");
  if (flags & NIO_DISK_INFO_DIRTY)
    printf(" dirty");
  if (flags & NIO_DISK_INFO_CHANGED)
    printf(" changed");
  if (flags & NIO_DISK_INFO_HAS_GEOMETRY)
    printf(" geometry");
  if (flags & NIO_DISK_INFO_HAS_LAST_ERROR)
    printf(" last-error");
  puts("");
}

int main(int argc, char **argv)
{
  uint8_t slot = 1;
  unsigned com = 1;
  uint16_t base;
  nio_disk_info_t info;
  nio_response_t response;

  if (argc > 3 || (argc > 1 && argv[1][0] == '?')) {
    print_usage();
    return 1;
  }

  if (argc > 1)
    slot = (uint8_t) atoi(argv[1]);
  if (argc > 2)
    com = (unsigned) atoi(argv[2]);

  base = serial_com_base(com);
  if (!base) {
    print_usage();
    return 1;
  }

  printf("NIOPROBE slot %u COM%u\r\n", (unsigned) slot, com);
  serial_init(base, 1);

  if (!nio_disk_info(slot, &info, &response)) {
    printf("Disk info failed");
    if (response.status != NIO_STATUS_INTERNAL_ERROR)
      printf(": status %u (%s)", response.status, nio_status_name(response.status));
    puts("");
    return 2;
  }

  printf("status: %u (%s)\r\n", response.status, nio_status_name(response.status));
  printf("reply bytes: %u\r\n", response.payload_length);
  print_flags(info.flags);
  printf("reported slot: %u\r\n", (unsigned) info.slot);
  printf("image type: %u\r\n", (unsigned) info.image_type);
  printf("sector size: %u\r\n", info.sector_size);
  printf("sector count: %lu\r\n", info.sector_count);
  printf("last error: %u\r\n", (unsigned) info.last_error);

  return 0;
}
