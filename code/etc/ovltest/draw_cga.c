// draw_cga.c -- CGA drawing overlay (window 1, module "cga").
// Compiled to draw_cga.obj and assigned to overlay module cga in ovltest.ovl.
//
// Exports: cga_phase()
// Calls (resident): rand_x, rand_y, rand_c, kbhit

#include "ovltest.h"

// putpixel -- plot one pixel in CGA mode 4 (320x200, 4-colour)
static void putpixel(int x, int y, int color) {
#asm
    push  bx
    push  si
    mov   ax, 0B800h
    mov   es, ax
    xor   si, si
    mov   bx, [bp+6]
    test  bx, 1
    jz    __cga_pp1
    mov   si, 2000h
__cga_pp1:
    shr   bx, 1
    mov   ax, bx
    shl   ax, 1
    shl   ax, 1
    shl   ax, 1
    shl   ax, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    add   ax, bx
    add   si, ax
    mov   bx, [bp+4]
    mov   ax, bx
    shr   ax, 1
    shr   ax, 1
    add   si, ax
    and   bx, 3
    xor   bx, 3
    shl   bx, 1
    mov   cl, bl
    mov   al, es:[si]
    mov   bl, 3
    shl   bl, cl
    not   bl
    and   al, bl
    mov   bl, [bp+8]
    and   bl, 3
    shl   bl, cl
    or    al, bl
    mov   es:[si], al
    pop   si
    pop   bx
#endasm
}

// cga_phase -- CGA pixel scatter; returns when a key is pressed.
void cga_phase() {
    int x, y, c;
    while (!kbhit()) {
        x = rand_x();
        y = rand_y();
        c = rand_c();
        putpixel(x, y, c);
    }
}
