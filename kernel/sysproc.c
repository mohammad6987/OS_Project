#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "file.h"
#include "fs.h"
#include "stat.h"
extern int append_to_file(char *path, char *data, int len);
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_nice(void)
{
  struct proc *p = myproc();
  int increment = (int)p->trapframe->a0;
  if (increment < 0)
    return -1;
  int finalNice = p->niceness + increment;
  if (finalNice > 19 || finalNice < -20)
    return -1;

  p->niceness = finalNice;

  return 0;
}

uint64
sys_deadline(void)
{
  struct proc *p = myproc();
  uint newDealine = (int)p->trapframe->a0;
  if (newDealine <= ticks)
    return -1;
  p->ticks = newDealine;
  return 0;
}

uint64 sys_append(void)
{
  char path[EXT2_NAME_LEN], data[EXT2_NAME_LEN];
  int len;

  if (argstr(0, path, EXT2_NAME_LEN) < 0 || // Fetch the path
      argstr(1, data, EXT2_NAME_LEN) < 0)   // Fetch the length
  {
    return -1; // Error, invalid arguments
  }
  argint(2, &len);
  return append_to_file(path, data, len);
}

uint64 sys_readdir(void)
{
  char path[EXT2_NAME_LEN];
  struct inode *dp;
  struct dirent de;


  if (argstr(0, path, EXT2_NAME_LEN) < 0)
    return -1;

  dp = namei(path);
  if (dp == 0 || dp->type != T_DIR)
    return -1;

  ilock(dp);

  int off = 0;
  while (readi(dp, 0, (uint64)&de, off, sizeof(de)) == sizeof(de))
  {
    if (de.inum == 0)
      continue;
    printf("%s\n", de.name);
    off += sizeof(de);
  }

  iunlock(dp);
  return 0;
}
