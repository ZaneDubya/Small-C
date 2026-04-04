#include "../../smallc22/stdio.h"

int global = 0;
extern int data;

int main() {
  printf("code addr=%x value=%x\n", code, code());
  printf("code addr=%x value=%x\n", code, code());
  printf("code addr=%x value=%x\n", code, code());
  printf("code addr=%x value=%x\n", code, code());
  printf("code addr=%x value=%x\n", code, code());
  printf("data addr=%x value=%d\n", &data, data);
  return 0;
}
