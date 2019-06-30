#include <stdio.h>

int global = 0;

main() {
  printf("code addr=%x value=%x\n", code, code());
  printf("%data value=x\n", data);
  return 0;
}