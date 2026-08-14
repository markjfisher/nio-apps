#include "../../../nio-core-apps/include/platform/amiga/fujinet_disk_iface.h"

#include <dos/dos.h>
#include <devices/trackdisk.h>
#include <exec/io.h>
#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BOOL active_handler(int unit)
{
    char name[4];
    struct DosList *list;
    struct DosList *entry;
    BOOL active = FALSE;

    sprintf(name, "DN%d", unit);
    list = LockDosList(LDF_DEVICES);
    if (list != NULL) {
        entry = FindDosEntry(list, name, LDF_DEVICES);
        active = entry != NULL;
        UnLockDosList(LDF_DEVICES);
    }
    return active;
}

static LONG replace_media(int unit, UBYTE slot)
{
    struct MsgPort *port;
    struct IOExtTD *request;
    struct fujinet_disk_catalog_mount catalog;
    LONG result;

    port = CreatePort(NULL, 0);
    if (port == NULL) return -100;
    request = (struct IOExtTD *)CreateExtIO(port, sizeof(*request));
    if (request == NULL) {
        DeletePort(port);
        return -101;
    }
    if (OpenDevice(FUJINET_DISK_DEVICE_NAME, unit,
                   (struct IORequest *)request, 0) != 0) {
        DeleteExtIO((struct IORequest *)request);
        DeletePort(port);
        return -102;
    }
    catalog.catalog_slot = slot;
    catalog.writable = 0;
    request->iotd_Req.io_Command = FUJINET_DISK_CMD_MOUNT_CATALOG;
    request->iotd_Req.io_Data = &catalog;
    request->iotd_Req.io_Length = sizeof(catalog);
    result = DoIO((struct IORequest *)request);
    CloseDevice((struct IORequest *)request);
    DeleteExtIO((struct IORequest *)request);
    DeletePort(port);
    return result;
}

int main(int argc, char **argv)
{
    LONG result;
    BOOL inhibited;
    BPTR output;

    if (argc == 3 && strcmp(argv[1], "false") == 0) {
        output = Open(argv[2], MODE_NEWFILE);
        if (!output) return 21;
        inhibited = Inhibit("DN0:", DOSFALSE);
        FPrintf(output, "INHIBIT=FALSE RETURN=%ld IOERR=%ld\n",
                (long)inhibited, (long)IoErr());
        Close(output);
        return inhibited ? 0 : 31;
    }
    if (argc != 3) return 20;
    output = Open(argv[1], MODE_NEWFILE);
    if (!output) return 21;
    inhibited = Inhibit("DN0:", DOSTRUE);
    FPrintf(output, "INHIBIT=TRUE RETURN=%ld IOERR=%ld\n",
            (long)inhibited, (long)IoErr());
    if (!inhibited) {
        Close(output);
        return 30;
    }
    result = replace_media(0, (UBYTE)atoi(argv[2]));
    FPrintf(output, "REPLACEMENT RC=%ld\n", (long)result);
    Close(output);
    if (inhibited) {
        Execute("C:fmount_inhibit_exp_a false DH0:exp-a-false.result", 0, 0);
    }
    return result == 0 ? 0 : 10;
}
