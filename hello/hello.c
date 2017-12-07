#include <stdio.h>

main() {
    int i;
    i = 0;
    i = !i;
    if (i == 1) {
      printf("Nope\n");
    }
    printf("hello world\n");
}