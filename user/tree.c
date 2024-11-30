#include "kernel/types.h"
#include "user/user.h"

enum procstate { RUNNING, SLEEPING, ZOMBIE,RUNNABLE , USED , UNUSED  };
struct process_data {
    int pid;
    int parent_pid;
    int heap_size;
    enum procstate state;
    char name[16];
    int niceness;
};
const char* procstate_strings[] = {
    "RUNNING",
    "SLEEPING",
    "ZOMBIE",
    "RUNNABLE",
    "USED",
    "UNUSED"
};


void dfs(int pid, int depth, struct process_data* procs, int n) {
    for (int i = 0; i < n; i++) {
        if (procs[i].parent_pid == pid) {
            for (int j = 0; j < depth; j++)
                printf("|  ");
            printf("|-> %s , PID: %d , State: %s ,Niceness: %d ,  parent_PID: %d\n", procs[i].name, procs[i].pid , procstate_strings[procs[i].state] , procs[i].niceness,procs[i].parent_pid);
	    dfs(procs[i].pid, depth + 1, procs, n); 
        }
    }
}

int main(void) {
    struct process_data procs[64];
    int index = 0;
    int before_pid = 0;

    while (next_process(before_pid, &procs[index]) > 0) {
        before_pid = procs[index].pid;
        index++;
    }

    printf("List of all processes :\n");
    dfs(-1, 0, procs, index);

    return 0;
}
