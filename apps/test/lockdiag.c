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
    return 0;
}
