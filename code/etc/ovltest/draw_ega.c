// draw_ega.c -- EGA drawing overlay (window 1, module "ega").
// Compiled to draw_ega.obj and assigned to overlay module ega in ovltest.ovl.
//
// Exports: ega_phase()
// Calls (resident): setmode, rand_ex, rand_ey, rand_ec, rand_range_c, kbhit
//
// palette[] and deck[] are globals whose DATA lives in the resident segment;
// only the code for this module is swapped in and out of the draw window.

#include "ovltest.h"

int palette[16];
int deck[64];

static void pick_palette() {
    int i, j, tmp;
    for (i = 0; i < 64; i++) deck[i] = i;
    for (i = 0; i < 16; i++) {
        j = i + rand_range_c(64 - i);
        tmp = deck[j];
        deck[j] = deck[i];
        deck[i] = tmp;
        palette[i] = tmp;
    }
}

static void set_palette() {
#asm
    mov  dx, 3DAh
    in   al, dx
    mov  dx, 3C0h
    mov  si, OFFSET _palette
    xor  cx, cx
__setp_loop:
    mov  al, cl
    out  dx, al
    mov  al, [si]
    out  dx, al
    add  si, 2
    inc  cx
    cmp  cx, 16
    jl   __setp_loop
    mov  al, 20h
    out  dx, al
#endasm
}

static void ega_putpixel(int x, int y, int color) {
#asm
    push  bx
    push  si
    mov   bx, [bp+6]
    mov   ax, bx
    shl   ax, 1
    shl   ax, 1
    shl   ax, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    add   ax, bx
    mov   si, ax
    mov   bx, [bp+4]
    mov   ax, bx
    shr   ax, 1
    shr   ax, 1
    shr   ax, 1
    add   si, ax
    and   bx, 7
    mov   cl, bl
    mov   al, 80h
    shr   al, cl
    mov   ah, al
    mov   al, 8
    mov   dx, 3CEh
    out   dx, ax
    mov   ax, 0A000h
    mov   es, ax
    mov   al, es:[si]
    mov   al, [bp+8]
    and   al, 0Fh
    mov   es:[si], al
    pop   si
    pop   bx
#endasm
}

// ega_phase -- EGA mode 0Dh pixel scatter; returns when a key is pressed.
// Configures the mode and random palette before drawing.
void ega_phase() {
    int x, y, c;
    // INT 10h AL=0Dh: 320x200 EGA 16-colour
    setmode(13);
    pick_palette();
    set_palette();
#asm
    mov  dx, 3CEh
    mov  ax, 0205h
    out  dx, ax
    mov  dx, 3C4h
    mov  ax, 0F02h
    out  dx, ax
#endasm
    while (!kbhit()) {
        x = rand_ex();
        y = rand_ey();
        c = rand_ec();
        ega_putpixel(x, y, c);
    }
}
