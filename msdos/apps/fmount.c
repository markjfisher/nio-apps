#include "fnctl.h"
#include "fnsvc.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
  puts("Usage: FMOUNT slot [drive:] [RO|RW]");
}

static fnsvc_mount_t mount;

static int drive_to_unit(const char *s)
{
  int drive;
  int unit;

  if (!s || !isalpha((unsigned char) s[0]))
    return -1;
  drive = toupper((unsigned char) s[0]) - 'A' + 1;
  for (unit = 0; unit < FNCTL_MAX_UNITS; unit++) {
    int found = fnctl_find_drive_for_unit((uint8_t) unit);
    if (found == drive)
      return unit;
  }
  return -1;
}

int main(int argc, char **argv)
{
  uint8_t slot;
  int unit;
  uint8_t readonly = 0;
  int argi;

  if (argc < 2 || argc > 4 || (argc > 1 && argv[1][0] == '?')) {
    usage();
    return 1;
  }

  slot = (uint8_t) atoi(argv[1]);
  if (slot >= FNCTL_MAX_UNITS) {
    puts("Bad slot");
    return 1;
  }

  unit = slot;
  for (argi = 2; argi < argc; argi++) {
    if (isalpha((unsigned char) argv[argi][0]) && argv[argi][1] == ':') {
      unit = drive_to_unit(argv[argi]);
      if (unit < 0) {
        printf("%c: is not a FujiNet drive\n", toupper((unsigned char) argv[argi][0]));
        return 1;
      }
    } else if (stricmp(argv[argi], "RO") == 0) {
      readonly = 1;
    } else if (stricmp(argv[argi], "RW") == 0) {
      readonly = 0;
    } else {
      usage();
      return 1;
    }
  }

  if (!fnsvc_get_mount(slot, &mount) || !mount.enabled || !mount.uri[0]) {
    printf("Slot %u has no image selected\n", (unsigned) slot);
    return 2;
  }

  if (!fnsvc_disk_mount(slot, mount.uri, readonly)) {
    puts("Disk mount failed");
    return 2;
  }

  if (!fnctl_set_unit_slot((uint8_t) unit, slot)) {
    puts("Unable to update FUJINET.SYS drive mapping");
    return 2;
  }

  {
    int drive = fnctl_find_drive_for_unit((uint8_t) unit);
    printf("Mounted slot %u on %c:\n", (unsigned) slot, drive ? ('A' + drive - 1) : '?');
  }
  return 0;
}
