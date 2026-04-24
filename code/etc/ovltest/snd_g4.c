// snd_g4.c -- OPL2 G4 note overlay (window 2, module "sndg4").
// Compiled to snd_g4.obj and assigned to overlay module sndg4 in ovltest.ovl.
//
// Exports: play_g4()
// Calls (resident): opl_init, opl_note
//
// G4 (392 Hz): Fnum = 0x204, block = 4
//   A0h = 0x04,  B0h = 0x32

#include "ovltest.h"

void play_g4() {
    opl_init();
    opl_note(0x04, 0x32);
}
