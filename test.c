#include <stdlib.h>
#include <stdio.h>
main () {
 printf ("P\n");
 pid_t x = fork();
 if (x != 0) {
 waitpid(x, NULL , 0);
 printf ("Q\n");
} else {
 printf ("R\n");
 for (int i = 0; i < 2; i++) {
 pid_t y = fork();
 if (y == 0) {
 printf ("S\n");
 } else {
 waitpid(y, NULL , 0); printf ("T\n");
 }
 }
 }
 }
