// snd_e4.c -- OPL2 E4 note overlay (window 2, module "snde4").
// Compiled to snd_e4.obj and assigned to overlay module snde4 in ovltest.ovl.
//
// Exports: play_e4()
// Calls (resident): opl_init, opl_note
//
// E4 (330 Hz): Fnum = 0x1B2, block = 4
//   A0h = 0xB2,  B0h = 0x31

#include "ovltest.h"

void play_e4() {
    opl_init();
    opl_note(0xB2, 0x31);
}
