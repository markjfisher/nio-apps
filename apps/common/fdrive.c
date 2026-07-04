#include "fnctl.h"
#include "fnsvc.h"

#include <stdio.h>

static fnsvc_mount_t mount_buf;

int main(void)
{
  uint8_t unit;
  uint8_t slot;
  int drive;
  unsigned shown = 0;

  for (unit = 0; unit < FNCTL_MAX_UNITS; unit++) {
    drive = fnctl_find_drive_for_unit(unit);

    if (!drive)
      continue;
    if (!fnctl_get_unit_slot(unit, &slot))
      continue;

    printf("%c: slot %u", 'A' + drive - 1, (unsigned) slot);
    if (fnsvc_get_mount(slot, &mount_buf) && mount_buf.enabled && mount_buf.uri[0])
      printf(" [%s] %s", mount_buf.mode, mount_buf.uri);
    else
      printf(" -- no image selected --");
    puts("");
    shown++;
  }

  if (!shown) {
    puts("No FujiNet drives found");
    return 1;
  }

  return 0;
}
