#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int parent = getpid();
    for (int i = 0; i < 5; i++)
    {
        int id = fork();
        if (id > 0)
        {
            nice(5);
            sleep(10);
        }
    }
    if (getpid() == parent)
    {
        char *args[] = {"tree", 0};
        exec("tree", args);
    }

    return 0;
}