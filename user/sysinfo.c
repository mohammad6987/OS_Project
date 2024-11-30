#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

struct sysinfo_data {
    uint32 free_memory;
    uint32 running_processes;
};

int main(int argc, char *argv[])
{
 struct sysinfo_data info;
  printf("this is system info:\n");
  if (sysinfo(&info) < 0) {
        printf("There was an error...\n");
  } else {
	uint32 kb = info.free_memory/1024;
        printf("Free Memory:\n %d Bytes\n %d KB\n %d MB\n", info.free_memory , kb ,kb/1024 );
        printf("Running Processes: %d\n", info.running_processes);
    }
  exit(0);
}
