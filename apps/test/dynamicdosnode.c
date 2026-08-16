#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <libraries/expansion.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include <stdio.h>
#include <string.h>

#include "../../../fujinet-nio-driver/amiga/include/fujinet_disk_dos_envec.h"

struct ExpansionBase *ExpansionBase;

static const char dos_name[] = "DY0";
static const char exec_name[] = "fujinet-disk.device";

static void build_envec(fujinet_disk_dos_envec_t *source)
{
    memset(source, 0, sizeof(*source));
    source->de_TableSize = 19; /* complete entries 0..19, not a count */
    source->de_SizeBlock = 128;
    source->de_SecOrg = 0;
    source->de_Surfaces = 2;
    source->de_SectorPerBlock = 1;
    source->de_BlocksPerTrack = 11;
    source->de_LowCyl = 0;
    source->de_HighCyl = 79;
    source->de_Reserved = 2;
    source->de_PreAlloc = 0;
    source->de_Interleave = 0;
    source->de_NumBuffers = 5;
    source->de_BufMemType = 1;
    source->de_MaxTransfer = 0x7fffffffUL;
    source->de_Mask = 0xfffffffeUL;
    source->de_BootPri = 0;
    source->de_DosType = FUJINET_AMIGA_DOS_OFS;
    source->de_Baud = 1200;
    source->de_Control = 0;
    source->de_BootBlocks = 0;
    source->handler_stack_size = 32768;
    source->handler_priority = 5;
    source->handler_glob_vec = -1;
}

static void print_env(const fujinet_disk_dos_envec_t *e)
{
    printf("BUILDER table=%lu sizeBlock=%lu secOrg=%lu surfaces=%lu "
           "sectorPerBlock=%lu blocksPerTrack=%lu reserved=%lu preAlloc=%lu "
           "interleave=%lu lowCyl=%lu highCyl=%lu buffers=%lu bufMemType=%lu "
           "maxTransfer=%08lx mask=%08lx bootPri=%ld dosType=%08lx baud=%lu "
           "control=%lu bootBlocks=%lu stack=%ld priority=%ld globVec=%08lx\n",
           (unsigned long)e->de_TableSize, (unsigned long)e->de_SizeBlock,
           (unsigned long)e->de_SecOrg, (unsigned long)e->de_Surfaces,
           (unsigned long)e->de_SectorPerBlock,
           (unsigned long)e->de_BlocksPerTrack, (unsigned long)e->de_Reserved,
           (unsigned long)e->de_PreAlloc, (unsigned long)e->de_Interleave,
           (unsigned long)e->de_LowCyl, (unsigned long)e->de_HighCyl,
           (unsigned long)e->de_NumBuffers, (unsigned long)e->de_BufMemType,
           (unsigned long)e->de_MaxTransfer, (unsigned long)e->de_Mask,
           (long)e->de_BootPri, (unsigned long)e->de_DosType,
           (unsigned long)e->de_Baud, (unsigned long)e->de_Control,
           (unsigned long)e->de_BootBlocks, (long)e->handler_stack_size,
           (long)e->handler_priority, (unsigned long)e->handler_glob_vec);
}

static void print_bstr(const char *label, BSTR value)
{
    const unsigned char *text = (const unsigned char *)BADDR(value);
    unsigned int length = text != NULL ? text[0] : 0;
    printf("%s=", label);
    while (length-- != 0)
        putchar(*++text);
    putchar('\n');
}

static void print_node(const char *phase, const struct DeviceNode *node)
{
    const struct FileSysStartupMsg *startup;
    const struct DosEnvec *environment;

    printf("%s DEVICE dn_Name=%08lx dn_Startup=%08lx dn_Handler=%08lx "
           "dn_StackSize=%lu dn_Priority=%ld dn_GlobVec=%08lx\n",
           phase, (unsigned long)node->dn_Name,
           (unsigned long)node->dn_Startup,
           (unsigned long)node->dn_Handler,
           (unsigned long)node->dn_StackSize, (long)node->dn_Priority,
           (unsigned long)node->dn_GlobalVec);
    fflush(stdout);
    print_bstr("dn_Name_decoded", node->dn_Name);
    fflush(stdout);
    print_bstr("dn_Handler_decoded", node->dn_Handler);
    startup = (const struct FileSysStartupMsg *)BADDR(node->dn_Startup);
    if (startup == NULL) {
        puts("STARTUP absent=1");
        return;
    }
    printf("%s STARTUP fssm_Unit=%lu fssm_Device=%08lx fssm_Environ=%08lx "
           "fssm_Flags=%08lx\n", phase, (unsigned long)startup->fssm_Unit,
           (unsigned long)startup->fssm_Device,
           (unsigned long)startup->fssm_Environ,
           (unsigned long)startup->fssm_Flags);
    fflush(stdout);
    print_bstr("fssm_Device_decoded", startup->fssm_Device);
    environment = (const struct DosEnvec *)BADDR(startup->fssm_Environ);
    if (environment == NULL) {
        puts("ENV absent=1");
        return;
    }
    printf("%s ENV table=%lu sizeBlock=%lu secOrg=%lu surfaces=%lu "
           "sectorPerBlock=%lu blocksPerTrack=%lu reserved=%lu preAlloc=%lu "
           "interleave=%lu lowCyl=%lu highCyl=%lu buffers=%lu bufMemType=%lu "
           "maxTransfer=%08lx mask=%08lx bootPri=%ld dosType=%08lx baud=%lu "
           "control=%lu bootBlocks=%lu\n", phase,
           (unsigned long)environment->de_TableSize,
           (unsigned long)environment->de_SizeBlock,
           (unsigned long)environment->de_SecOrg,
           (unsigned long)environment->de_Surfaces,
           (unsigned long)environment->de_SectorPerBlock,
           (unsigned long)environment->de_BlocksPerTrack,
           (unsigned long)environment->de_Reserved,
           (unsigned long)environment->de_PreAlloc,
           (unsigned long)environment->de_Interleave,
           (unsigned long)environment->de_LowCyl,
           (unsigned long)environment->de_HighCyl,
           (unsigned long)environment->de_NumBuffers,
           (unsigned long)environment->de_BufMemType,
           (unsigned long)environment->de_MaxTransfer,
           (unsigned long)environment->de_Mask,
           (long)environment->de_BootPri,
           (unsigned long)environment->de_DosType,
           (unsigned long)environment->de_Baud,
           (unsigned long)environment->de_Control,
           (unsigned long)environment->de_BootBlocks);
}

static int action_die_probe(void)
{
    struct DosList *list;
    struct DosList *entry;
    struct MsgPort *task;
    LONG result;
    LONG ioerr;
    int attempt;

    list = LockDosList(LDF_READ | LDF_DEVICES);
    if (list == NULL) {
        puts("DIE pre_lock=failed");
        return 20;
    }
    entry = FindDosEntry(list, dos_name, LDF_DEVICES);
    task = entry != NULL ? entry->dol_Task : NULL;
    printf("DIE pre_present=%d pre_task=%08lx\n", entry != NULL,
           (unsigned long)task);
    UnLockDosList(LDF_READ | LDF_DEVICES);
    if (task == NULL)
        return 20;

    result = DoPkt(task, ACTION_DIE, 0, 0, 0, 0, 0);
    ioerr = IoErr();
    printf("DIE action_result=%ld ioerr=%ld\n", (long)result, (long)ioerr);

    for (attempt = 0; attempt < 20; ++attempt) {
        struct MsgPort *current_task = NULL;
        int present = 0;

        list = LockDosList(LDF_READ | LDF_DEVICES);
        if (list == NULL) {
            puts("DIE poll_lock=failed");
            return 20;
        }
        entry = FindDosEntry(list, dos_name, LDF_DEVICES);
        if (entry != NULL) {
            present = 1;
            current_task = entry->dol_Task;
        }
        printf("DIE poll=%d present=%d task=%08lx\n", attempt, present,
               (unsigned long)current_task);
        UnLockDosList(LDF_READ | LDF_DEVICES);
        if (!present || current_task == NULL)
            break;
        Delay(1);
    }
    return 0;
}

static int switch_to_hd(void)
{
    struct DosList *list = LockDosList(LDF_READ | LDF_DEVICES);
    struct DosList *entry;
    struct FileSysStartupMsg *startup;
    struct DosEnvec *env;

    if (list == NULL)
        return 20;
    entry = FindDosEntry(list, dos_name, LDF_DEVICES);
    if (entry == NULL || entry->dol_Task != NULL ||
        entry->dol_misc.dol_handler.dol_Startup == 0) {
        puts("HD_UPDATE inactive=0");
        UnLockDosList(LDF_READ | LDF_DEVICES);
        return 20;
    }
    startup = (struct FileSysStartupMsg *)BADDR(
        entry->dol_misc.dol_handler.dol_Startup);
    env = (struct DosEnvec *)BADDR(startup->fssm_Environ);
    if (env == NULL) {
        UnLockDosList(LDF_READ | LDF_DEVICES);
        return 20;
    }
    env->de_Surfaces = 2;
    env->de_SectorPerBlock = 1;
    env->de_BlocksPerTrack = 22;
    env->de_LowCyl = 0;
    env->de_HighCyl = 79;
    env->de_DosType = FUJINET_AMIGA_DOS_OFS;
    printf("HD_UPDATE inactive=1 task=00000000 table=%lu sizeBlock=%lu "
           "surfaces=%lu sectorPerBlock=%lu blocksPerTrack=%lu lowCyl=%lu "
           "highCyl=%lu dosType=%08lx\n",
           (unsigned long)env->de_TableSize,
           (unsigned long)env->de_SizeBlock,
           (unsigned long)env->de_Surfaces,
           (unsigned long)env->de_SectorPerBlock,
           (unsigned long)env->de_BlocksPerTrack,
           (unsigned long)env->de_LowCyl,
           (unsigned long)env->de_HighCyl,
           (unsigned long)env->de_DosType);
    UnLockDosList(LDF_READ | LDF_DEVICES);
    return 0;
}

static int switch_to_dd(void)
{
    struct DosList *list = LockDosList(LDF_READ | LDF_DEVICES);
    struct DosList *entry;
    struct FileSysStartupMsg *startup;
    struct DosEnvec *env;

    if (list == NULL)
        return 20;
    entry = FindDosEntry(list, dos_name, LDF_DEVICES);
    if (entry == NULL || entry->dol_Task != NULL ||
        entry->dol_misc.dol_handler.dol_Startup == 0) {
        puts("DD_UPDATE inactive=0");
        UnLockDosList(LDF_READ | LDF_DEVICES);
        return 20;
    }
    startup = (struct FileSysStartupMsg *)BADDR(
        entry->dol_misc.dol_handler.dol_Startup);
    env = (struct DosEnvec *)BADDR(startup->fssm_Environ);
    if (env == NULL) {
        UnLockDosList(LDF_READ | LDF_DEVICES);
        return 20;
    }
    env->de_Surfaces = 2;
    env->de_SectorPerBlock = 1;
    env->de_BlocksPerTrack = 11;
    env->de_LowCyl = 0;
    env->de_HighCyl = 79;
    env->de_DosType = FUJINET_AMIGA_DOS_OFS;
    printf("DD_UPDATE inactive=1 task=00000000 table=%lu sizeBlock=%lu "
           "surfaces=%lu sectorPerBlock=%lu blocksPerTrack=%lu lowCyl=%lu "
           "highCyl=%lu dosType=%08lx\n",
           (unsigned long)env->de_TableSize,
           (unsigned long)env->de_SizeBlock,
           (unsigned long)env->de_Surfaces,
           (unsigned long)env->de_SectorPerBlock,
           (unsigned long)env->de_BlocksPerTrack,
           (unsigned long)env->de_LowCyl,
           (unsigned long)env->de_HighCyl,
           (unsigned long)env->de_DosType);
    UnLockDosList(LDF_READ | LDF_DEVICES);
    return 0;
}

int main(int argc, char **argv)
{
    fujinet_disk_dos_envec_t source;
    ULONG packet[4 + 20];
    struct DeviceNode *node;

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
    if (ExpansionBase == NULL) {
        puts("EXPANSION open=failed");
        return 20;
    }

    (void)argc;
    if (argc > 1 && strcmp(argv[1], "--die") == 0) {
        int result = action_die_probe();
        CloseLibrary((struct Library *)ExpansionBase);
        return result;
    }
    if (argc > 1 && strcmp(argv[1], "--switch-hd") == 0) {
        int result = switch_to_hd();
        CloseLibrary((struct Library *)ExpansionBase);
        return result;
    }
    if (argc > 1 && strcmp(argv[1], "--switch-dd") == 0) {
        int result = switch_to_dd();
        CloseLibrary((struct Library *)ExpansionBase);
        return result;
    }
    (void)argv;

    build_envec(&source);
    print_env(&source);
    packet[0] = (ULONG)dos_name;
    packet[1] = (ULONG)exec_name;
    packet[2] = 0;
    packet[3] = 0;
    packet[4 + DE_TABLESIZE] = source.de_TableSize;
    packet[4 + DE_SIZEBLOCK] = source.de_SizeBlock;
    packet[4 + DE_SECORG] = source.de_SecOrg;
    packet[4 + DE_NUMHEADS] = source.de_Surfaces;
    packet[4 + DE_SECSPERBLK] = source.de_SectorPerBlock;
    packet[4 + DE_BLKSPERTRACK] = source.de_BlocksPerTrack;
    packet[4 + DE_RESERVEDBLKS] = source.de_Reserved;
    packet[4 + DE_PREFAC] = source.de_PreAlloc;
    packet[4 + DE_INTERLEAVE] = source.de_Interleave;
    packet[4 + DE_LOWCYL] = source.de_LowCyl;
    packet[4 + DE_UPPERCYL] = source.de_HighCyl;
    packet[4 + DE_NUMBUFFERS] = source.de_NumBuffers;
    packet[4 + DE_BUFMEMTYPE] = source.de_BufMemType;
    packet[4 + DE_MAXTRANSFER] = source.de_MaxTransfer;
    packet[4 + DE_MASK] = source.de_Mask;
    packet[4 + DE_BOOTPRI] = (ULONG)source.de_BootPri;
    packet[4 + DE_DOSTYPE] = source.de_DosType;
    packet[4 + DE_BAUD] = source.de_Baud;
    packet[4 + DE_CONTROL] = source.de_Control;
    packet[4 + DE_BOOTBLOCKS] = source.de_BootBlocks;

    node = MakeDosNode(packet);
    if (node == NULL) {
        puts("MAKE failed=1");
        CloseLibrary((struct Library *)ExpansionBase);
        return 20;
    }
    print_node("BEFORE_ADD", node);
    node->dn_StackSize = source.handler_stack_size;
    node->dn_Priority = source.handler_priority;
    node->dn_GlobalVec = (BPTR)source.handler_glob_vec;
    puts("HANDLER_FIELDS_APPLIED stack=32768 priority=5 globVec=ffffffff handler_preserved=0");
    print_node("CONFIGURED", node);
    if (!AddDosNode(source.de_BootPri, 0, node)) {
        puts("ADD failed=1 cleanup=required");
        CloseLibrary((struct Library *)ExpansionBase);
        return 20;
    }
    puts("ADD name=DY0 unit=0 handler=fujinet-disk.device");
    print_node("AFTER_ADD", node);
    CloseLibrary((struct Library *)ExpansionBase);
    return 0;
}
