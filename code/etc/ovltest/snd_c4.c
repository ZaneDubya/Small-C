// snd_c4.c -- OPL2 C4 note overlay (window 2, module "sndc4").
// Compiled to snd_c4.obj and assigned to overlay module sndc4 in ovltest.ovl.
//
// Exports: play_c4()
// Calls (resident): opl_init, opl_note
//
// C4 (261 Hz): Fnum = 0x158, block = 4
//   A0h register (Fnum[7:0])  = 0x58
//   B0h register (KeyOn | block<<2 | Fnum[9:8]) = 0x31

#include "ovltest.h"

void play_c4() {
    opl_init();
    opl_note(0x58, 0x31);
}
