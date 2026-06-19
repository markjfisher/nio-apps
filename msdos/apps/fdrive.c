#include "fnctl.h"
#include "fnsvc.h"

#include <stdio.h>

int main(void)
{
  uint8_t unit;
  unsigned shown = 0;

  for (unit = 0; unit < FNCTL_MAX_UNITS; unit++) {
    uint8_t slot;
    int drive = fnctl_find_drive_for_unit(unit);
    fnsvc_mount_t mount;

    if (!drive)
      continue;
    if (!fnctl_get_unit_slot(unit, &slot))
      continue;

    printf("%c: slot %u", 'A' + drive - 1, (unsigned) slot);
    if (fnsvc_get_mount(slot, &mount) && mount.enabled && mount.uri[0])
      printf(" [%s] %s", mount.mode, mount.uri);
    else
      printf(" -- no image selected --");
    puts("");
    shown++;
  }

  if (!shown) {
    puts("No FujiNet DOS drives found. Is FUJINET.SYS loaded?");
    return 1;
  }

  return 0;
}
