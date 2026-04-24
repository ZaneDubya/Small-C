// ovlchk.c -- _ovl_check: dispatch stub helper called by overlay stubs.
// The stub sequence is:
//   PUSH encoded          ; encoded = (win_idx << 8) | grp_idx
//   CALL _ovl_check       ; ensure the right module is loaded
//   POP AX                ; discard `encoded` from stack
//   PUSH target_offset    ; push real function's CS offset
//   RET                   ; jump to target function
// Compiled as part of ovllib.lib.
#include <stdio.h>

extern int _ovl_win_grp[];
void _ovl_load(int win_idx, int grp_idx);

void _ovl_check(int encoded) {
  int win_idx, grp_idx;
  win_idx = (encoded >> 8) & 0xFF;
  grp_idx = encoded & 0xFF;
  if (_ovl_win_grp[win_idx] != grp_idx) {
    _ovl_load(win_idx, grp_idx);
    _ovl_win_grp[win_idx] = grp_idx;
  }
}
