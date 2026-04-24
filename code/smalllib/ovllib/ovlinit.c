// ovlinit.c -- ovl_init: open the EXE and capture CS for the overlay system.
// Compiled as part of ovllib.lib.
#include <stdio.h>
#include "ovllib.h"

extern int _ovl_exe_fd;
extern int _ovl_cs_seg;
extern int _ovl_win_grp[];
extern int _get_cs();

void ovl_init(char *exepath) {
  int i;
  _ovl_exe_fd = fopen(exepath, "r");
  if (!_ovl_exe_fd) {
    fprintf(stderr, "ovl_init: cannot open EXE %s\n", exepath);
    exit(1);
  }
  _ovl_cs_seg = _get_cs();
  for (i = 0; i < 8; i++) {
    _ovl_win_grp[i] = -1;
  }
}
