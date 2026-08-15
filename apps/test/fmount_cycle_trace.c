#include "../../../nio-core-apps/include/platform/amiga/fujinet_disk_iface.h"

#include <devices/trackdisk.h>
#include <exec/io.h>
#include <clib/alib_protos.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>

static void print_name(BPTR output, BSTR name)
{
    const unsigned char *text = (const unsigned char *)BADDR(name);
    unsigned int length;
    if (text == NULL) { FPrintf(output, "<none>"); return; }
    length = text[0];
    while (length-- != 0) FPrintf(output, "%lc", (ULONG)*++text);
}

static void dump_dos_list(BPTR output, LONG stage)
{
    struct DosList *list = LockDosList(LDF_READ | LDF_ALL);
    struct DosList *entry;
    if (list == NULL) { FPrintf(output, "STAGE=%ld DOSLIST lock=failed\n", stage); return; }
    entry = NextDosEntry(list, LDF_ALL);
    while (entry != NULL) {
        if (entry->dol_Type == DLT_DEVICE || entry->dol_Type == DLT_VOLUME) {
            FPrintf(output, "STAGE=%ld DOSLIST ", stage);
            print_name(output, entry->dol_Name);
            FPrintf(output, " type=%ld task=%08lx\n", (long)entry->dol_Type,
                    (unsigned long)entry->dol_Task);
        }
        entry = NextDosEntry(entry, LDF_ALL);
    }
    UnLockDosList(LDF_READ | LDF_ALL);
}

static BOOL active_handler(BPTR output)
{
    struct DosList *list = LockDosList(LDF_READ | LDF_DEVICES);
    struct DosList *entry;
    BOOL active;
    if (list == NULL) { FPrintf(output, "ACTIVE lock=failed\n"); return FALSE; }
    entry = FindDosEntry(list, "DN0", LDF_DEVICES);
    active = entry != NULL && entry->dol_Task != NULL;
    FPrintf(output, "ACTIVE=%ld", (long)active);
    if (entry != NULL) FPrintf(output, " name=DN0 type=%ld task=%08lx",
                                (long)entry->dol_Type, (unsigned long)entry->dol_Task);
    FPrintf(output, "\n");
    UnLockDosList(LDF_READ | LDF_DEVICES);
    return active;
}

static LONG replace_media(UBYTE slot)
{
    struct MsgPort *port = CreatePort(NULL, 0);
    struct IOExtTD *request;
    struct fujinet_disk_catalog_mount catalog;
    LONG result;
    if (port == NULL) return -100;
    request = (struct IOExtTD *)CreateExtIO(port, sizeof(*request));
    if (request == NULL) { DeletePort(port); return -101; }
    if (OpenDevice(FUJINET_DISK_DEVICE_NAME, 0, (struct IORequest *)request, 0) != 0) {
        DeleteExtIO((struct IORequest *)request); DeletePort(port); return -102;
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
    BPTR output;
    LONG result;
    LONG ioerr;
    BOOL inhibited;
    if (argc != 3) return 20;
    output = Open(argv[1], MODE_NEWFILE);
    if (output == NULL) return 21;
    FPrintf(output, "TRACE slot=%ld\n", (long)atoi(argv[2]));
    active_handler(output);
    dump_dos_list(output, 1);
    inhibited = Inhibit("DN0:", DOSTRUE);
    ioerr = IoErr();
    FPrintf(output, "INHIBIT=TRUE RETURN=%ld IOERR=%ld\n", (long)inhibited, (long)ioerr);
    if (inhibited) {
        result = replace_media((UBYTE)atoi(argv[2]));
        FPrintf(output, "REPLACEMENT RC=%ld\n", (long)result);
        dump_dos_list(output, 2);
        inhibited = Inhibit("DN0:", DOSFALSE);
        ioerr = IoErr();
        FPrintf(output, "INHIBIT=FALSE RETURN=%ld IOERR=%ld\n", (long)inhibited, (long)ioerr);
        dump_dos_list(output, 3);
    } else result = -103;
    Close(output);
    return result == 0 && inhibited ? 0 : 31;
}
