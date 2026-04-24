// draw_vga.c -- VGA drawing overlay (window 1, module "vga").
// Compiled to draw_vga.obj and assigned to overlay module vga in ovltest.ovl.
//
// Exports: vga_phase()
// Calls (resident): rand_ex, rand_ey, rand_vc, kbhit

#include "ovltest.h"

// vga_putpixel -- plot one pixel in VGA mode 13h (320x200, 256-colour).
// Mode 13h is a linear byte-per-pixel framebuffer at A000:0000.
static void vga_putpixel(int x, int y, int color) {
#asm
    push  bx
    push  si
    mov   bx, [bp+6]
    mov   si, bx
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    add   si, bx
    add   si, [bp+4]
    mov   ax, 0A000h
    mov   es, ax
    mov   al, [bp+8]
    mov   es:[si], al
    pop   si
    pop   bx
#endasm
}

// vga_phase -- VGA pixel scatter; returns when a key is pressed.
void vga_phase() {
    int x, y, c;
    while (!kbhit()) {
        x = rand_ex();
        y = rand_ey();
        c = rand_vc();
        vga_putpixel(x, y, c);
    }
}
