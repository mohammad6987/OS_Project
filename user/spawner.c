#include "user/user.h"

int main(void)
{
	int procnums = 2;
	int parent = getpid();
	for (int i = 0; i < procnums; i++)
	{
		int pid = fork();
		if (pid == 0)
		{
			nice(2);
			pid = fork();
			if (pid > 0)
			{
				char *args[] = {"sleep", "100000000", 0};
				nice(5);
				exec("sleep", args);
			}
		}
	}
	if (getpid() == parent)
	{
		char *args[] = {"tree", 0};
		exec("tree", args);
	}
}
