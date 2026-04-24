// ovltest.c -- Overlay demo: resident module.
//
// Demonstrates ylink overlay support.  Three video-mode drawing phases
// (CGA/EGA/VGA) and three OPL2 notes (C4/E4/G4) each live in their own
// overlay module.  Only one drawing module and one sound module are loaded
// into memory at a time; the runtime library loads them on demand.
//
// Build: see ovltest.bat

#include "..\..\smalllib\ovllib\ovllib.h"
#include "ovltest.h"

// Three independent LCG seeds for x, y, colour streams.
unsigned int seed_x, seed_y, seed_c;

void init_rand() {
    int lo, hi;
#asm
    mov  ah, 00h
    int  1ah
    mov  [bp-2], dx
    mov  [bp-4], cx
#endasm
    seed_x = (lo ^ hi)   | 1;
    seed_y = (lo + 4660) | 1;
    seed_c = (hi + 9029) | 1;
}

unsigned int rand_x() {
    unsigned int r;
    seed_x = seed_x * 25173 + 13849;
    r = seed_x;
#asm
    mov  ax, [bp-2]
    mov  cx, 320
    mul  cx
    mov  ax, dx
#endasm
}

unsigned int rand_y() {
    unsigned int r;
    seed_y = seed_y * 20561 + 17239;
    r = seed_y;
#asm
    mov  ax, [bp-2]
    mov  cx, 200
    mul  cx
    mov  ax, dx
#endasm
}

unsigned int rand_c() {
    unsigned int r;
    seed_c = seed_c * 29441 + 15139;
    r = seed_c;
#asm
    mov  ax, [bp-2]
    mov  cx, 4
    mul  cx
    mov  ax, dx
#endasm
}

unsigned int rand_ex() {
    unsigned int r;
    seed_x = seed_x * 25173 + 13849;
    r = seed_x;
#asm
    mov  ax, [bp-2]
    mov  cx, 320
    mul  cx
    mov  ax, dx
#endasm
}

unsigned int rand_ey() {
    unsigned int r;
    seed_y = seed_y * 20561 + 17239;
    r = seed_y;
#asm
    mov  ax, [bp-2]
    mov  cx, 200
    mul  cx
    mov  ax, dx
#endasm
}

unsigned int rand_ec() {
    unsigned int r;
    seed_c = seed_c * 29441 + 15139;
    r = seed_c;
#asm
    mov  ax, [bp-2]
    mov  cx, 16
    mul  cx
    mov  ax, dx
#endasm
}

unsigned int rand_vc() {
    seed_c = seed_c * 29441 + 15139;
    return seed_c >> 8;
}

unsigned int rand_range_c(unsigned int range) {
    unsigned int r;
    seed_c = seed_c * 29441 + 15139;
    r = seed_c;
#asm
    mov  ax, [bp-2]
    mov  cx, [bp+4]
    mul  cx
    mov  ax, dx
#endasm
}

int kbhit() {
#asm
    mov  ah, 1
    int  16h
    jnz  __kbh1
    xor  ax, ax
    jmp  __kbh2
__kbh1:
    mov  ax, 1
__kbh2:
#endasm
}

void consume_key() {
#asm
    mov  ah, 0
    int  16h
#endasm
}

void setmode(int mode) {
#asm
    mov  ax, [bp+4]
    int  10h
#endasm
}

int sb_detect() {
#asm
    mov  dx, 226h
    mov  al, 1
    out  dx, al
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    xor  al, al
    out  dx, al
    mov  dx, 22Eh
    mov  cx, 2000h
__sbd_poll:
    in   al, dx
    test al, 80h
    jnz  __sbd_avail
    dec  cx
    jnz  __sbd_poll
    xor  ax, ax
    jmp  __sbd_done
__sbd_avail:
    mov  dx, 22Ah
    in   al, dx
    cmp  al, 0AAh
    jne  __sbd_no
    mov  ax, 1
    jmp  __sbd_done
__sbd_no:
    xor  ax, ax
__sbd_done:
#endasm
}

// opl_write -- resident OPL2 register write (called from sound overlays).
void opl_write(int reg, int val) {
#asm
    mov  dx, 388h
    mov  al, [bp+4]
    out  dx, al
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    inc  dx
    mov  al, [bp+6]
    out  dx, al
    dec  dx
    mov  cx, 35
__oplw_wait:
    in   al, dx
    dec  cx
    jnz  __oplw_wait
#endasm
}

// opl_init -- reset and configure OPL2 channel 0 as sustained organ tone.
// Resident so that every sound overlay can call it without a stub.
void opl_init() {
    opl_write(0x01, 0x20);
    opl_write(0x20, 0x21);
    opl_write(0x40, 0x28);
    opl_write(0x60, 0xF0);
    opl_write(0x80, 0x00);
    opl_write(0xE0, 0x00);
    opl_write(0x23, 0x21);
    opl_write(0x43, 0x00);
    opl_write(0x63, 0xF0);
    opl_write(0x83, 0x00);
    opl_write(0xE3, 0x00);
    opl_write(0xC0, 0x00);
}

// opl_note -- key-on for OPL2 channel 0.
// Resident so that every sound overlay can call it without a stub.
void opl_note(int flo, int fhi) {
    opl_write(0xA0, flo);
    opl_write(0xB0, fhi);
}

// opl_off -- key-off channel 0.
void opl_off() {
    opl_write(0xB0, 0x00);
}

int main(int argc, char **argv) {
    int sb_ok;

    // Initialise the overlay runtime (must be first).
    ovl_init(argv[0]);

    init_rand();
    sb_ok = sb_detect();

    // Phase 1: CGA mode 4 (320x200, 4-colour) + OPL2 C4
    setmode(4);
    if (sb_ok) play_c4();   // load sound overlay: snd_c4
    cga_phase();            // load draw overlay:  draw_cga
    if (sb_ok) opl_off();
    consume_key();

    // Phase 2: EGA mode 0Dh (320x200, 16-colour) + OPL2 E4
    if (sb_ok) play_e4();   // load sound overlay: snd_e4
    ega_phase();            // load draw overlay:  draw_ega (calls ega_init internally)
    if (sb_ok) opl_off();
    consume_key();

    // Phase 3: VGA mode 13h (320x200, 256-colour) + OPL2 G4
    setmode(19);
    if (sb_ok) play_g4();   // load sound overlay: snd_g4
    vga_phase();            // load draw overlay:  draw_vga
    if (sb_ok) opl_off();
    consume_key();

    setmode(3);
    return 0;
}
