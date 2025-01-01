#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// Allocate 512 MB of memory
#define TO_ALLOCATE (1024 * 1024 * 512)
#define TEST_FILE "test"

int main(int argc, char *argv[])
{
  printf("Hello world!\n");
  // Allocate a HUGE buffer. But this should not crash because of lazy allocation
  char *buffer = sbrk(TO_ALLOCATE);
  if (buffer == (char *)0xffffffffffffffffL)
  {
    printf("Buffer is NULL!\n");
    exit(1);
  }
  printf("Allocated a huge buffer!\n");
  // Fill some data in some pages to test if the pages get allocated
  for (int i = 0; i < 4096 * 4; i++)
    buffer[i] = (char)i;
  // Read the buffer and verify
  for (int i = 0; i < 4096 * 4; i++)
    if (buffer[i] != (char)i)
    {
      printf("Wrong data in index %d: expected %d got %d\n", i, (char)i, buffer[i]);
      exit(1);
    }
  printf("Lazily allocated data!\n");
  // Now try with a syscall
  unlink(TEST_FILE);
  int fd = open(TEST_FILE, O_CREATE | O_RDWR);
  if (fd < 0)
  {
    printf("Cannot open the file (write)\n");
    exit(1);
  }
  int written_size = write(fd, buffer, 4096);
  if (written_size != 4096)
  {
    printf("Short write: %d\n", written_size);
    exit(1);
  }
  close(fd);
  // Read the file in un-allocated memory space
  printf("Reading the test file...\n");
  fd = open(TEST_FILE, O_RDONLY);
  if (fd < 0)
  {
    printf("Cannot open the file (read)\n");
    exit(1);
  }
  int read_size = read(fd, buffer + 1024 * 1024, 4096);
  if (read_size != 4096)
  {
    printf("Short read: %d\n", read_size);
    exit(1);
  }
  close(fd);
  unlink(TEST_FILE);
  // Did we read it correctly?
  for (int i = 0; i < 4096; i++)
    if (buffer[1024 * 1024 + i] != (char)i)
    {
      printf("Wrong data in index %d: expected %d got %d\n", i, (char)i, buffer[1024 * 1024 + i]);
      exit(1);
    }
  // Does negative sbrk work?
  sbrk(-TO_ALLOCATE);
  if (sbrk(0) != buffer)
  {
    printf("Expected sbrk to return to initial state\n");
    exit(1);
  }
  // Done!
  printf("gg\n");
  exit(0);
}