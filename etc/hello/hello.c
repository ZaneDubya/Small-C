#include <stdio.h>

int global = 0;

main() {
  if (undefined == 1) {
    puts("Should not happen");
  }
}
