#include <dos/dos.h>
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

int main(void)
{
    list_entries("DEVICE", LDF_DEVICES);
    list_entries("VOLUME", LDF_VOLUMES);
    find_entry("DN0", LDF_DEVICES);
    find_entry("DN0:", LDF_DEVICES);
    find_entry("DN0", LDF_ALL);
    find_entry("DN0:", LDF_ALL);
    return 0;
}
