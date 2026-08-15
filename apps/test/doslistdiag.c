#include <dos/dos.h>
#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

#include <stdio.h>

static void print_bstr(BSTR bstr)
{
    const unsigned char *text = (const unsigned char *)BADDR(bstr);
    unsigned int length;

    if (text == NULL) {
        fputs("<none>", stdout);
        return;
    }
    length = text[0];
    while (length-- != 0)
        putchar(*++text);
}

static void print_entry(const char *group, struct DosList *entry)
{
    fputs(group, stdout);
    fputs(" name=", stdout);
    print_bstr(entry->dol_Name);
    printf(" type=%ld task=%08lx\n", (long)entry->dol_Type,
           (unsigned long)entry->dol_Task);
}

static void list_entries(const char *group, ULONG flags)
{
    struct DosList *entry;
    struct DosList *list = LockDosList(LDF_READ | flags);

    if (list == NULL) {
        printf("%s lock=failed\n", group);
        return;
    }
    entry = NextDosEntry(list, flags);
    while (entry != NULL) {
        print_entry(group, entry);
        entry = NextDosEntry(entry, flags);
    }
    UnLockDosList(LDF_READ | flags);
}

static void find_entry(const char *name, ULONG flags)
{
    struct DosList *entry;
    struct DosList *list = LockDosList(LDF_READ | flags);

    if (list == NULL) {
        printf("FIND name=%s flags=%08lx lock=failed\n", name,
               (unsigned long)flags);
        return;
    }
    entry = FindDosEntry(list, name, flags);
    printf("FIND name=%s flags=%08lx found=%ld", name,
           (unsigned long)flags, entry != NULL ? 1L : 0L);
    if (entry != NULL) {
        fputs(" type=", stdout);
        printf("%ld task=%08lx entry_name=", (long)entry->dol_Type,
               (unsigned long)entry->dol_Task);
        print_bstr(entry->dol_Name);
    }
    putchar('\n');
    UnLockDosList(LDF_READ | flags);
}

static void print_dos_envec(const char *name)
{
    struct DosList *list;
    struct DosList *entry;
    struct FileSysStartupMsg *startup;
    struct DosEnvec *environment;

    list = LockDosList(LDF_READ | LDF_DEVICES);
    if (list == NULL) {
        puts("ENV lock=failed");
        return;
    }
    entry = FindDosEntry(list, name, LDF_DEVICES);
    if (entry == NULL || entry->dol_misc.dol_handler.dol_Startup == 0) {
        puts("ENV found=0");
        UnLockDosList(LDF_READ | LDF_DEVICES);
        return;
    }
    startup = (struct FileSysStartupMsg *)BADDR(
        entry->dol_misc.dol_handler.dol_Startup);
    if (startup == NULL || startup->fssm_Environ == 0) {
        puts("ENV startup=invalid");
        UnLockDosList(LDF_READ | LDF_DEVICES);
        return;
    }
    environment = (struct DosEnvec *)BADDR(startup->fssm_Environ);
    printf("ENV name=%s table=%lu sizeBlock=%lu secOrg=%lu surfaces=%lu "
           "sectorPerBlock=%lu blocksPerTrack=%lu reserved=%lu "
           "preAlloc=%lu interleave=%lu lowCyl=%lu highCyl=%lu "
           "buffers=%lu bufMemType=%lu maxTransfer=%08lx mask=%08lx "
           "bootPri=%ld dosType=%08lx baud=%lu control=%lu bootBlocks=%lu "
           "stack=%ld priority=%ld globVec=%08lx\n",
           name,
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
           (unsigned long)environment->de_BootBlocks,
           (long)entry->dol_misc.dol_handler.dol_StackSize,
           (long)entry->dol_misc.dol_handler.dol_Priority,
           (unsigned long)entry->dol_misc.dol_handler.dol_GlobVec);
    UnLockDosList(LDF_READ | LDF_DEVICES);
}

int main(void)
{
    list_entries("DEVICE", LDF_DEVICES);
    list_entries("VOLUME", LDF_VOLUMES);
    find_entry("DN0", LDF_DEVICES);
    find_entry("DN0:", LDF_DEVICES);
    find_entry("DN0", LDF_ALL);
    find_entry("DN0:", LDF_ALL);
    print_dos_envec("DN0");
    return 0;
}
