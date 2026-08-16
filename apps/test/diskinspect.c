#include <fujinet-amiga-disk/device.h>
#include <fujinet-amiga-disk/support.h>

#include <devices/trackdisk.h>
#include <exec/io.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct MsgPort *port;
    struct IOExtTD *request;
    struct fujinet_disk_catalog_inspection inspection;
    fujinet_disk_media_profile_t profile;
    uint32_t dos_type;
    LONG result;

    if (argc != 3) return 10;
    port = CreatePort(NULL, 0);
    request = port ? (struct IOExtTD *)CreateExtIO(port, sizeof(*request)) : NULL;
    if (request == NULL || OpenDevice((CONST_STRPTR)FUJINET_DISK_DEVICE_NAME, (ULONG)atoi(argv[1]),
                                      (struct IORequest *)request, 0) != 0)
        return 20;
    memset(&inspection, 0, sizeof(inspection));
    inspection.catalog_slot = (UBYTE)atoi(argv[2]);
    request->iotd_Req.io_Command = FUJINET_DISK_CMD_INSPECT_CATALOG;
    request->iotd_Req.io_Data = &inspection;
    request->iotd_Req.io_Length = sizeof(inspection);
    result = DoIO((struct IORequest *)request);
    CloseDevice((struct IORequest *)request);
    DeleteExtIO((struct IORequest *)request);
    DeletePort(port);
    if (result != 0) { printf("INSPECT RC=%ld\n", (long)result); return 10; }
    if (fujinet_disk_classify_media_profile(&inspection.inspection.media, &profile) != FN_OK ||
        fujinet_disk_classify_filesystem(inspection.inspection.boot_bytes,
                                         inspection.inspection.boot_length,
                                         &dos_type) != FN_OK)
        return 11;
    printf("INSPECT RC=0 size=%u count=%lu profile=%u dostype=%08lx\n",
           (unsigned)inspection.inspection.media.sector_size,
           (unsigned long)inspection.inspection.media.sector_count,
           (unsigned)profile.kind, (unsigned long)dos_type);
    return 0;
}
