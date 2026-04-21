// pixels.c -- CGA/EGA/VGA random pixel demo with Sound Blaster OPL2 audio.
//
// Phase 1: CGA mode 4   (320x200,  4-colour)               -- press a key to continue.
// Phase 2: EGA mode 0Dh (320x200, 16-colour, random palette) -- press a key to continue.
// Phase 3: VGA mode 13h (320x200, 256-colour)               -- press a key to exit.
// Sound:   OPL2 FM tone per phase (C4 / E4 / G4) if SB detected at 220h.
//
// Build:
//   cc pixels -a -p
//   asm pixels /p
//   ylink pixels.obj,..\..\smalllib\clib.lib -e=pixels.exe

// CGA mode 4 screen dimensions
#define SCR_W  320
#define SCR_H  200

// EGA mode 0Dh screen dimensions (320x200, 16-colour)
#define EGA_W  320
#define EGA_H  200

// Three independent LCG states.
// Using different (a, c) pairs for each stream ensures that x, y, and
// colour come from unrelated linear maps -- consecutive outputs of a
// single LCG always lie on hyperplanes in 2-D, so a single shared
// stream produces visible diagonal structure regardless of period.
// All pairs satisfy Hull-Dobell for m=2^16: c is odd, (a-1) mod 4=0.
unsigned int seed_x, seed_y, seed_c;

// EGA palette: 16 colours randomly chosen from the 64-colour EGA hardware palette.
int palette[16];
int deck[64];    // scratch space for Fisher-Yates shuffle

// init_rand -- seed three streams from the BIOS tick counter.
// Different mixing constants keep the seeds distinct at startup.
void init_rand() {
    int lo, hi;
#asm
    mov  ah, 00h
    int  1ah          ; CX:DX = ticks since midnight
    mov  [bp-2], dx   ; lo = DX
    mov  [bp-4], cx   ; hi = CX
#endasm
    seed_x = (lo ^ hi)   | 1;
    seed_y = (lo + 4660) | 1;   // 0x1234: distinct starting phase
    seed_c = (hi + 9029) | 1;   // 0x2345: distinct starting phase
}

// rand_x -- x-stream LCG (a=25173, c=13849), scaled to [0..319].
unsigned int rand_x() {
    unsigned int r;
    seed_x = seed_x * 25173 + 13849;
    r = seed_x;
#asm
    mov  ax, [bp-2]   ; r
    mov  cx, 320
    mul  cx           ; DX:AX = r * 320
    mov  ax, dx       ; high word = unbiased result in [0..319]
#endasm
}

// rand_y -- y-stream LCG (a=20561, c=17239), scaled to [0..199].
// Different multiplier => independent sequence from rand_x.
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

// rand_c -- colour-stream LCG (a=29441, c=15139), scaled to [0..3].
// Third distinct linear map; no correlation with x or y streams.
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

// rand_ex, rand_ey, rand_ec -- EGA-range variants of the three RNG
// streams, reusing the same seeds (continuing from the CGA phase).
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

// rand_vc -- colour stream scaled to [0..255] for VGA.
// Range is 2^8, so the high byte of the 16-bit LCG output is unbiased.
unsigned int rand_vc() {
    seed_c = seed_c * 29441 + 15139;
    return seed_c >> 8;
}

// rand_range_c -- advance the colour stream and return a value in [0..range-1].
// Used by pick_palette for unbiased index selection.
unsigned int rand_range_c(unsigned int range) {
    unsigned int r;
    seed_c = seed_c * 29441 + 15139;
    r = seed_c;
#asm
    mov  ax, [bp-2]   ; r
    mov  cx, [bp+4]   ; range
    mul  cx           ; DX:AX = r * range
    mov  ax, dx       ; high word = unbiased result
#endasm
}

// pick_palette -- fill palette[0..15] with 16 distinct values from 0..63
// using a partial Fisher-Yates shuffle on a 64-element deck.
void pick_palette() {
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

// set_palette -- program EGA ATC registers 0-15 with palette[].
// Protocol: read 3DAh to reset the ATC flip-flop, then for each register
// write the index (bit5=0) then the 6-bit colour value to port 3C0h.
// Finally write 20h to re-enable video output (PAS=1).
void set_palette() {
#asm
    mov  dx, 3DAh
    in   al, dx           ; reset ATC address/data flip-flop
    mov  dx, 3C0h
    mov  si, OFFSET _palette
    xor  cx, cx           ; i = 0
__setp_loop:
    mov  al, cl           ; ATC register index (0..15), bit5=0
    out  dx, al           ; select register
    mov  al, [si]         ; palette[i] low byte = colour 0..63
    out  dx, al           ; write colour
    add  si, 2            ; advance to next int
    inc  cx
    cmp  cx, 16
    jl   __setp_loop
    mov  al, 20h          ; re-enable video (PAS=1)
    out  dx, al
#endasm
}

// consume_key -- drain one pending keypress from the BIOS buffer.
// Call after a kbhit loop exits to clear the key before the next
// phase begins; without this, the next loop exits immediately.
void consume_key() {
#asm
    mov  ah, 0
    int  16h
#endasm
}

// opl_write -- send one register write to the OPL2 FM chip (port 388h/389h).
// Waits ~3.3 us after the address byte and ~23 us after the data byte,
// as required by the OPL2 spec, using dummy status-port reads.
void opl_write(int reg, int val) {
#asm
    mov  dx, 388h
    mov  al, [bp+4]       ; reg index -> address port
    out  dx, al
    in   al, dx           ; ~3.3 us delay: 6 reads of status port
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    inc  dx               ; 389h = data port
    mov  al, [bp+6]       ; val -> data port
    out  dx, al
    dec  dx               ; back to 388h for delay reads
    mov  cx, 35           ; ~23 us delay: 35 reads
__oplw_wait:
    in   al, dx
    dec  cx
    jnz  __oplw_wait
#endasm
}

// opl_init -- set up OPL2 channel 0 as a sustained organ-style tone.
void opl_init() {
    opl_write(0x01, 0x20);  // enable waveform select
    opl_write(0x20, 0x21);  // modulator: EGTyp=1 (sustained), mult=1
    opl_write(0x40, 0x28);  // modulator: TL=40 (moderate attenuation)
    opl_write(0x60, 0xF0);  // modulator: attack=15, decay=0
    opl_write(0x80, 0x00);  // modulator: sustain=0 (max), release=0
    opl_write(0xE0, 0x00);  // modulator: waveform=sine
    opl_write(0x23, 0x21);  // carrier:   EGTyp=1 (sustained), mult=1
    opl_write(0x43, 0x00);  // carrier:   full volume
    opl_write(0x63, 0xF0);  // carrier:   attack=15, decay=0
    opl_write(0x83, 0x00);  // carrier:   sustain=0 (max), release=0
    opl_write(0xE3, 0x00);  // carrier:   waveform=sine
    opl_write(0xC0, 0x00);  // channel 0: FM synthesis, no feedback
}

// opl_note -- key-on for OPL2 channel 0.
// flo = A0h value (Fnum bits 7:0)
// fhi = B0h value (key-on=1 | block<<2 | Fnum bits 9:8)
//   C4 (261 Hz): flo=0x58  fhi=0x31
//   E4 (330 Hz): flo=0xB2  fhi=0x31
//   G4 (392 Hz): flo=0x04  fhi=0x32
void opl_note(int flo, int fhi) {
    opl_write(0xA0, flo);
    opl_write(0xB0, fhi);
}

// opl_off -- key-off channel 0.
void opl_off() {
    opl_write(0xB0, 0x00);
}

// sb_detect -- probe for a Sound Blaster DSP at base 220h.
// Resets the DSP (port 226h), polls port 22Eh for data-ready,
// reads port 22Ah and checks for the 0AAh acknowledgement byte.
// Returns 1 if a SB is found, 0 otherwise.
int sb_detect() {
#asm
    mov  dx, 226h         ; DSP reset port (base 220h + 6)
    mov  al, 1
    out  dx, al           ; assert reset
    in   al, dx           ; short delay before clearing
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    in   al, dx
    xor  al, al
    out  dx, al           ; deassert reset
    mov  dx, 22Eh         ; read-buffer status port (base + 0Eh)
    mov  cx, 2000h        ; timeout: ~8192 polls
__sbd_poll:
    in   al, dx
    test al, 80h          ; bit 7 = data available
    jnz  __sbd_avail
    dec  cx
    jnz  __sbd_poll
    xor  ax, ax           ; timed out: no SB found
    jmp  __sbd_done
__sbd_avail:
    mov  dx, 22Ah         ; read data port (base + 0Ah)
    in   al, dx
    cmp  al, 0AAh         ; SB acknowledges reset with 0AAh
    jne  __sbd_no
    mov  ax, 1
    jmp  __sbd_done
__sbd_no:
    xor  ax, ax
__sbd_done:
#endasm
}

// setmode -- set video mode via INT 10h, AH=0
void setmode(int mode) {
#asm
    mov  ax, [bp+4]   ; AH=0 (high byte of small int), AL=mode
    int  10h
#endasm
}

// ega_init -- switch to EGA mode 0Dh (320x200, 16-colour) and configure
// GC write mode 2.  Picks 16 distinct colours from the 64-colour EGA
// hardware palette at random and loads them into ATC registers 0-15.
void ega_init() {
    setmode(13);    // INT 10h AL=0Dh: 320x200 EGA 16-colour
    pick_palette();
    set_palette();
#asm
    ; GC write mode 2 (GC index 5 = mode register, bit 1:0 = 10b)
    ; Word OUT writes AL to the index port and AH to the data port (index+1)
    mov   dx, 3CEh
    mov   ax, 0205h   ; AH=02 (mode 2), AL=05 (GC Mode register index)
    out   dx, ax

    ; Sequencer Map Mask: enable all 4 planes (index 2, value 0Fh)
    mov   dx, 3C4h
    mov   ax, 0F02h   ; AH=0Fh (all planes), AL=02 (Map Mask index)
    out   dx, ax
#endasm
}

// ega_putpixel -- plot one pixel in EGA mode 0Dh (320x200, 16-colour)
//
// Byte offset = y * 40 + (x >> 3)
// Bit  mask   = 0x80 >> (x & 7)
// Read byte -> loads plane latches; write colour index (bits 3:0) -> GC
// write mode 2 expands across all 4 planes; ATC maps index to palette.
void ega_putpixel(int x, int y, int color) {
#asm
    push  bx
    push  si

    ; Byte offset = y * 40 + (x >> 3)
    ; y * 40 = y * 32 + y * 8  (shift decomposition, avoids MUL)
    mov   bx, [bp+6]         ; bx = y
    mov   ax, bx
    shl   ax, 1
    shl   ax, 1
    shl   ax, 1              ; ax = y * 8
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1              ; bx = y * 32
    add   ax, bx             ; ax = y * 40
    mov   si, ax

    mov   bx, [bp+4]         ; bx = x
    mov   ax, bx
    shr   ax, 1
    shr   ax, 1
    shr   ax, 1              ; ax = x >> 3
    add   si, ax             ; si = final byte offset

    ; Bit mask = 0x80 >> (x & 7)
    and   bx, 7
    mov   cl, bl
    mov   al, 80h
    shr   al, cl             ; al = 0x80 >> (x & 7)

    ; Set GC Bit Mask register (index 8) via word OUT
    mov   ah, al             ; AH = bit mask value
    mov   al, 8              ; AL = GC Bit Mask index
    mov   dx, 3CEh
    out   dx, ax

    ; Point ES at EGA video segment A000:0000
    mov   ax, 0A000h
    mov   es, ax

    ; Read byte to load all four plane latches
    mov   al, es:[si]

    ; Write colour -- write mode 2 places bits 3:0 as colour at the
    ; masked bit position across all 4 planes
    mov   al, [bp+8]
    and   al, 0Fh
    mov   es:[si], al

    pop   si
    pop   bx
#endasm
}

// vga_putpixel -- plot one pixel in VGA mode 13h (320x200, 256-colour)
//
// Mode 13h is a linear byte-per-pixel framebuffer at A000:0000.
// offset = y * 320 + x  (y*320 = y*256 + y*64, computed with SHL chains)
// One write sets the colour; no bitplane setup required.
void vga_putpixel(int x, int y, int color) {
#asm
    push  bx
    push  si

    ; si = y * 320 + x
    ; y * 320 = y * 256 + y * 64
    mov   bx, [bp+6]         ; bx = y
    mov   si, bx
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1
    shl   si, 1              ; si = y * 256
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1              ; bx = y * 64
    add   si, bx             ; si = y * 320
    add   si, [bp+4]         ; si += x

    ; Point ES at VGA linear framebuffer A000:0000
    mov   ax, 0A000h
    mov   es, ax

    ; Write colour byte
    mov   al, [bp+8]
    mov   es:[si], al

    pop   si
    pop   bx
#endasm
}

// kbhit -- non-blocking keyboard check
// Returns 1 if a key is waiting in the buffer, 0 otherwise.
// Uses INT 16h, AH=1 (peek): ZF set => no key, ZF clear => key ready.
int kbhit() {
#asm
    mov  ah, 1
    int  16h
    jnz  __kbh1
    xor  ax, ax       ; ZF was set: no key
    jmp  __kbh2
__kbh1:
    mov  ax, 1        ; ZF was clear: key available
__kbh2:
#endasm
}

// putpixel -- plot one pixel in CGA mode 4 (320x200, 4-colour)
//
// CGA video RAM at B800:0000
//   Even scan lines (y=0,2,4,...): B800:0000 + (y>>1)*80 + (x>>2)
//   Odd  scan lines (y=1,3,5,...): B800:2000 + (y>>1)*80 + (x>>2)
//
// 2 bits per pixel; pixel 0 (x=0) is in bits 7:6 of each byte.
//   shift = (3 - (x & 3)) * 2
//   (3 - n) == (n XOR 3) for n in {0,1,2,3}
void putpixel(int x, int y, int color) {
#asm
    push  bx
    push  si

    ; Point ES at the CGA video segment
    mov   ax, 0B800h
    mov   es, ax

    ; Bank select: odd scan lines live at offset 0x2000
    xor   si, si
    mov   bx, [bp+6]         ; bx = y
    test  bx, 1
    jz    __pp1
    mov   si, 2000h
__pp1:
    ; Row offset: (y >> 1) * 80 = (y >> 1) * 64 + (y >> 1) * 16
    shr   bx, 1              ; bx = y >> 1
    mov   ax, bx
    shl   ax, 1
    shl   ax, 1
    shl   ax, 1
    shl   ax, 1              ; ax = bx * 16
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1
    shl   bx, 1              ; bx = bx * 64
    add   ax, bx             ; ax = original * 80
    add   si, ax

    ; Column byte offset: x >> 2
    mov   bx, [bp+4]         ; bx = x
    mov   ax, bx
    shr   ax, 1
    shr   ax, 1              ; ax = x >> 2
    add   si, ax             ; si = final byte offset

    ; Bit shift amount for this pixel: (3 - (x & 3)) * 2
    ; For x & 3 in {0,1,2,3}: 3-(x&3) == (x&3) XOR 3
    and   bx, 3
    xor   bx, 3              ; bx = 3 - (x & 3)
    shl   bx, 1              ; bx = shift (0, 2, 4, or 6)
    mov   cl, bl             ; cl = shift count

    ; Read-modify-write the video byte
    mov   al, es:[si]        ; read existing byte
    mov   bl, 3
    shl   bl, cl             ; bl = 0x03 shifted to pixel position
    not   bl                 ; bl = clear mask
    and   al, bl             ; zero out the target 2 bits

    mov   bl, [bp+8]         ; bl = color (low byte of int)
    and   bl, 3              ; keep only 2 bits
    shl   bl, cl             ; shift color into position
    or    al, bl             ; combine

    mov   es:[si], al        ; write back

    pop   si
    pop   bx
#endasm
}

// main
int main() {
    int x, y, c, sb_ok;

    init_rand();
    sb_ok = sb_detect();
    // opl_init() is called per-phase to reset chip state cleanly

    // Phase 1: CGA mode 4 (320x200, 4-colour)  -- OPL2 plays C4
    setmode(4);
    if (sb_ok) { opl_init(); opl_note(0x58, 0x31); }
    while (!kbhit()) {
        x = rand_x();
        y = rand_y();
        c = rand_c();
        putpixel(x, y, c);
    }
    if (sb_ok) opl_off();
    consume_key();

    // Phase 2: EGA mode 0Dh (320x200, 16-colour, random palette)  -- OPL2 plays E4
    ega_init();
    if (sb_ok) { opl_init(); opl_note(0xB2, 0x31); }
    while (!kbhit()) {
        x = rand_ex();
        y = rand_ey();
        c = rand_ec();
        ega_putpixel(x, y, c);
    }
    if (sb_ok) opl_off();
    consume_key();

    // Phase 3: VGA mode 13h (320x200, 256-colour)  -- OPL2 plays G4
    setmode(19);    // INT 10h AL=13h: 320x200 VGA 256-colour
    if (sb_ok) { opl_init(); opl_note(0x04, 0x32); }
    while (!kbhit()) {
        x = rand_ex();
        y = rand_ey();
        c = rand_vc();
        vga_putpixel(x, y, c);
    }
    if (sb_ok) opl_off();
    consume_key();

    setmode(3);               // restore 80x25 text mode
    return 0;
}
