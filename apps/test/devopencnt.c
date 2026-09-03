#include <exec/devices.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <stdio.h>

extern struct ExecBase *SysBase;

int main(void)
{
    struct Device *device;
    BOOL found = FALSE;
    UWORD open_cnt = 0;

    Forbid();
    device = (struct Device *)FindName(&SysBase->DeviceList,
                                       (CONST_STRPTR)"fujinet-disk.device");
    if (device != NULL) {
        found = TRUE;
        open_cnt = device->dd_Library.lib_OpenCnt;
    }
    Permit();

    printf("DEVICE name=fujinet-disk.device found=%ld opencnt=%u\n",
           found ? 1L : 0L, (unsigned)open_cnt);
    return found ? 0 : 10;
}
