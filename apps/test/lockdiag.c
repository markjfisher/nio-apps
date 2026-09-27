/*
 * Report public AmigaDOS references to DN0:/DN2: filesystem handlers.
 *
 * This is a diagnostic utility, not an unmount implementation. It samples
 * process current/home directory locks and standard streams, whose handler
 * ports can be compared with the live DosList DeviceNode task ports.
 */
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#define MAX_PROCESSES 64
#define MAX_VOLUME_LOCKS 64

static struct MsgPort *device_task(const char *name)
{
    struct DosList *list;
    struct DosList *entry;
    struct MsgPort *task = NULL;

    list = LockDosList(LDF_READ | LDF_DEVICES);
    if (list == NULL)
        return NULL;
    entry = FindDosEntry(list, name, LDF_DEVICES);
    if (entry != NULL)
        task = entry->dol_Task;
    UnLockDosList(LDF_READ | LDF_DEVICES);
    return task;
}

static unsigned int append_processes(struct Task **tasks, unsigned int count,
                                     unsigned int capacity, struct List *list)
{
    struct Node *node;

    for (node = list->lh_Head; node->ln_Succ != NULL; node = node->ln_Succ) {
        if (node->ln_Type == NT_PROCESS && count < capacity)
            tasks[count++] = (struct Task *)node;
    }
    return count;
}

static unsigned int snapshot_processes(struct Task **tasks, unsigned int capacity)
{
    unsigned int count = 0;
    struct Task *current;

    Forbid();
    current = SysBase->ThisTask;
    if (current->tc_Node.ln_Type == NT_PROCESS && count < capacity)
        tasks[count++] = current;
    count = append_processes(tasks, count, capacity, &SysBase->TaskReady);
    count = append_processes(tasks, count, capacity, &SysBase->TaskWait);
    Permit();
    return count;
}

static const char *match_handler(struct MsgPort *port, struct MsgPort *dn0,
                                 struct MsgPort *dn2)
{
    if (port == dn0 && port != NULL)
        return "DN0";
    if (port == dn2 && port != NULL)
        return "DN2";
    return "other";
}

static void print_lock(const char *field, BPTR bptr, struct MsgPort *dn0,
                       struct MsgPort *dn2)
{
    struct FileLock *lock = (struct FileLock *)BADDR(bptr);

    if (lock == NULL) {
        printf("  LOCK field=%s value=00000000\n", field);
        return;
    }
    printf("  LOCK field=%s value=%08lx handler=%08lx match=%s key=%ld\n",
           field, (unsigned long)lock, (unsigned long)lock->fl_Task,
           match_handler(lock->fl_Task, dn0, dn2), (long)lock->fl_Key);
}

static void print_stream(const char *field, BPTR bptr, struct MsgPort *dn0,
                         struct MsgPort *dn2)
{
    struct FileHandle *handle = (struct FileHandle *)BADDR(bptr);

    if (handle == NULL) {
        printf("  STREAM field=%s value=00000000\n", field);
        return;
    }
    printf("  STREAM field=%s value=%08lx handler=%08lx match=%s\n",
           field, (unsigned long)handle, (unsigned long)handle->fh_Type,
           match_handler(handle->fh_Type, dn0, dn2));
}

/*
 * Volume entries maintain the public chain of outstanding FileLocks.  A lock
 * can outlive the usual process CurrentDir/HomeDir fields, so sample this
 * handler-owned list separately rather than assuming the process scan is
 * complete.
 */
static void volume_name(BSTR bptr, char *output, unsigned int capacity)
{
    const unsigned char *source = (const unsigned char *)BADDR(bptr);
    unsigned int length;

    if (source == NULL || capacity == 0) {
        if (capacity != 0)
            output[0] = '\0';
        return;
    }
    length = source[0];
    if (length >= capacity)
        length = capacity - 1;
    memcpy(output, source + 1, length);
    output[length] = '\0';
}

static void print_volume_locks(struct MsgPort *dn0, struct MsgPort *dn2)
{
    const ULONG flags = LDF_READ | LDF_VOLUMES;
    struct DosList *list;
    struct DosList *entry;

    list = LockDosList(flags);
    if (list == NULL) {
        printf("VOLUMES unavailable\n");
        return;
    }
    for (entry = NextDosEntry(list, LDF_VOLUMES); entry != NULL;
         entry = NextDosEntry(entry, LDF_VOLUMES)) {
        BPTR lock_bptr = entry->dol_misc.dol_volume.dol_LockList;
        char name[64];
        unsigned int index = 0;

        volume_name(entry->dol_Name, name, sizeof(name));
        printf("VOLUME node=%08lx handler=%08lx match=%s name=%s name_bptr=%08lx lock_list=%08lx\n",
               (unsigned long)entry, (unsigned long)entry->dol_Task,
               match_handler(entry->dol_Task, dn0, dn2), name,
               (unsigned long)entry->dol_Name, (unsigned long)lock_bptr);
        while (lock_bptr != 0 && index < MAX_VOLUME_LOCKS) {
            struct FileLock *lock = (struct FileLock *)BADDR(lock_bptr);

            if (lock == NULL)
                break;
            printf("  VOLUME_LOCK index=%u address=%08lx next=%08lx handler=%08lx "
                   "match=%s key=%ld volume=%08lx\n",
                   index, (unsigned long)lock, (unsigned long)lock->fl_Link,
                   (unsigned long)lock->fl_Task,
                   match_handler(lock->fl_Task, dn0, dn2), (long)lock->fl_Key,
                   (unsigned long)lock->fl_Volume);
            lock_bptr = lock->fl_Link;
            ++index;
        }
        if (lock_bptr != 0)
            printf("  VOLUME_LOCK truncated_after=%u\n", MAX_VOLUME_LOCKS);
    }
    UnLockDosList(flags);
}

int main(void)
{
    struct Task *tasks[MAX_PROCESSES];
    struct MsgPort *dn0 = device_task("DN0");
    struct MsgPort *dn2 = device_task("DN2");
    unsigned int count;
    unsigned int index;

    count = snapshot_processes(tasks, MAX_PROCESSES);
    printf("HANDLER DN0=%08lx DN2=%08lx processes=%u\n",
           (unsigned long)dn0, (unsigned long)dn2, count);
    for (index = 0; index < count; ++index) {
        struct Process *process = (struct Process *)tasks[index];
        const char *name = process->pr_Task.tc_Node.ln_Name;

        printf("PROCESS address=%08lx name=%s state=%ld\n",
               (unsigned long)process, name != NULL ? name : "<none>",
               (long)process->pr_Task.tc_State);
        print_lock("CurrentDir", process->pr_CurrentDir, dn0, dn2);
        print_lock("HomeDir", process->pr_HomeDir, dn0, dn2);
        print_stream("CIS", process->pr_CIS, dn0, dn2);
        print_stream("COS", process->pr_COS, dn0, dn2);
        print_stream("CES", process->pr_CES, dn0, dn2);
    }
    print_volume_locks(dn0, dn2);
    return 0;
}
