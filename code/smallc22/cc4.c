// ============================================================================
// Small-C Compiler -- Part 4 -- Back End.
// Copyright 1982, 1983, 1985, 1988 J. E. Hendrix.
// All rights reserved.
// ----------------------------------------------------------------------------
// Notice of Public Domain Status
// The source code for the Small-C Compiler and runtime libraries (CP/M & DOS),
// Small-Mac Assembler (CP/M), Small-Assembler (DOS), Small-Tools programs and
// Small-Windows library to which I hold copyrights are hereby available for
// royalty free use in private or commerical endeavors. The only obligation
// being that the users retain the original copyright notices and credit all
// prior authors (Ron Cain, James Hendrix, etc.) in derivative versions.
// James E. Hendrix Jr.
// ============================================================================

#include "stdio.h"
#include "cc.h"

// #define DISOPT           // display optimizations values

char needLong;

// === externals ==============================================================

extern char
*cptr, *macn, *litq, *symtab, optimize, ssname[NAMESIZE];

extern int
*stage, litlab, litptr, csp, output, oldseg, usexpr,
*snext, *stail, *slast;

int *funcbuf;      // per-function p-code buffer (heap-allocated in header)
int *fnext;        // next free slot in funcbuf
int *fl_labnum;    // label numbers recorded by flushfunc (heap-allocated)
int *fl_labpos;    // corresponding byte offsets

// forward declarations for this file:
void outstr(char ptr[]);
void outname(char ptr[]);
void outlines(char *s);
void outline(char ptr[]);
void outcode(int pcode, int value);
void colon();
void bufout(int pcode, int value);
void dumpstage();
int peep(int *seq);
void outsize(int size, int ident);
int isfree(int reg, int *pp);
int getpop(int *next);


// === optimizer command definitions ==========================================
//              --      p-codes must not overlap these
#define any     0x00FF   // matches any p-code
#define _pop    0x00FE   // matches if corresponding POP2 exists
#define pfree   0x00FD   // matches if pri register free
#define sfree   0x00FC   // matches if sec register free
#define comm    0x00FB   // matches if registers are commutative

//              --      these digits are reserved for n
#define go      0x0100   // go n entries
#define gc      0x0200   // get code from n entries away
#define gv      0x0300   // get value from n entries away
#define sum     0x0400   // add value from nth entry away
#define neg     0x0500   // negate the value
#define ife     0x0600   // if value == n do commands to next 0
#define ifl     0x0700   // if value <  n do commands to next 0
#define swv     0x0800   // swap value with value n entries away
#define topop   0x0900   // moves |code and current value to POP2

#define p1      0x0001   // plus 1
#define p2      0x0002   // plus 2
#define p3      0x0003   // plus 3
#define p4      0x0004   // plus 4
#define m1      0x00FF   // minus 1
#define m2      0x00FE   // minus 2
#define m3      0x00FD   // minus 3
#define m4      0x00FC   // minus 4

#define PRI      0030    // primary register bits
#define SEC      0003    // secondary register bits
#define USES     0011    // use register contents
#define ZAPS     0022    // zap register contents
#define PUSHES   0100    // pushes onto the stack
#define COMMUTES 0200    // commutative p-code

// === optimizer command lists ================================================

#define HIGH_SEQ  129

#ifdef DISOPT
int optcount[HIGH_SEQ + 1];   // per-rule optimization hit counts
#endif

int seqdata[] = {
    // 000 Add Fold
    // Replace ADD12+MOVE21 with ADD21, eliminating redundant copy.
    //     IN:  ADD12   MOVE21
    //          AX+=BX  BX=AX
    //     NET: sr = pr + sr
    //     WHY: ADD21 accumulates into BX directly; pr's incidental copy of the sum is dropped.
    //     OUT: ADD21
    //          BX+=AX
    ADD12,MOVE21,0,go|p1,ADD21,0, 0,

    // 001 Pr Imm-Add to Inc/Dec
    // Replace ADD1n(n) with repeated INC/DEC when |n| is small enough to save bytes.
    //     IN:  ADD1n(n)
    //          AX+=n
    //     NET: AX += n
    //     WHY: INC AX / DEC AX are 1-byte; ADD AX,imm16 is 3 bytes. Only useful for |n|<=2.
    //     OUT (n<0, |n|<=2): rDEC1 x |n|
    //     OUT (0<n<=2):      rINC1 x n
    //     OUT (n=0,|n|>2):   no change
    ADD1n,0,ifl|m2,0,ifl|0,rDEC1,neg,0,ifl|p3,rINC1,0, 0,

    // 002 Sr Imm-Add to Inc/Dec
    // Same as Pr Imm-Add to Inc/Dec for sr: replace ADD2n(n) with repeated INC/DEC on BX.
    //     IN:  ADD2n(n)
    //          BX+=n
    //     NET: BX += n
    //     WHY: INC BX / DEC BX are 1-byte; ADD BX,imm16 is 3 bytes. Only useful for |n|<=2.
    //     OUT (n<0, |n|<=2): rDEC2 x |n|
    //     OUT (0<n<=2):      rINC2 x n
    //     OUT (n=0,|n|>2):   no change
    ADD2n,0,ifl|m2,0,ifl|0,rDEC2,neg,0,ifl|p3,rINC2,0, 0,

    // 003 Pr Imm-Sub to Dec/Inc
    // Replace SUB1n(n) with repeated DEC/INC when |n| is small enough to save bytes.
    //     IN:  SUB1n(n)
    //          AX-=n
    //     NET: AX -= n
    //     WHY: DEC AX / INC AX are 1-byte; SUB AX,imm16 is 3 bytes. Only useful for |n|<=2.
    //     OUT (n>0, n<=2):   rDEC1 x n
    //     OUT (n<0, |n|<=2): rINC1 x |n|
    //     OUT (n=0, |n|>2):  no change
    SUB1n,0,ifl|m2,0,ifl|0,rINC1,neg,0,ifl|p3,rDEC1,0, 0,

    // 004 Post-Dec Byte, Ptr
    // Fold post-decrement of a byte through a pointer into a direct memory DEC/SUB.
    //     IN:  rDEC1(n)  PUTbp1   rINC1(n)
    //          AX-=n     [BX]=AL  AX+=n
    //     NET: [BX]-=n, AX restored to pre-dec value
    //     WHY: 8086 DEC [BX] / SUB [BX],imm modify memory directly; no register round-trip.
    //     OUT (n=1):  DECbp     -> DEC BYTE PTR [BX]
    //     OUT (n!=1): SUBbpn(n) -> SUB BYTE PTR [BX], n
    rDEC1,PUTbp1,rINC1,0,go|p2,ife|p1,DECbp,0,SUBbpn,0, 0,

    // 005 Post-Dec Word, Ptr
    // Same as Post-Dec Byte, Ptr for a word: fold post-decrement through a pointer into DEC/SUB word.
    //     IN:  rDEC1(n)  PUTwp1   rINC1(n)
    //          AX-=n     [BX]=AX  AX+=n
    //     NET: [BX]-=n, AX restored to pre-dec value
    //     WHY: 8086 DEC [BX] / SUB [BX],imm modify memory directly; no register round-trip.
    //     OUT (n=1): DECwp      -> DEC WORD PTR [BX]
    //     OUT (n!=1): SUBwpn(n) -> SUB WORD PTR [BX], n
    rDEC1,PUTwp1,rINC1,0,go|p2,ife|p1,DECwp,0,SUBwpn,0, 0,

    // 006 Post-Dec Byte, Global
    // Fold post-decrement of a global byte variable into a direct memory SUB.
    //     IN:  rDEC1(n)  PUTbm1(m)  rINC1(n)
    //          AX-=n     g=AL        AX+=n
    //     NET: g-=n, AX restored to pre-dec value
    //     WHY: 8086 SUB [mem],imm subtracts from a named memory location without a register.
    //     OUT: SUB_m_(m)  COMMAn(n)   -> SUB BYTE PTR g, n
    rDEC1,PUTbm1,rINC1,0,go|p1,SUB_m_,go|p1,COMMAn,go|m1,0, 0,

    // 007 Post-Dec Word, Global
    // Same as Post-Dec Byte, Global for a word: fold post-decrement of a global int into a direct memory SUB.
    //     IN:  rDEC1(n)  PUTwm1(m)  rINC1(n)
    //          AX-=n     g=AX        AX+=n
    //     NET: g-=n, AX restored to pre-dec value
    //     WHY: 8086 SUB [mem],imm subtracts from a named memory location without a register.
    //     OUT: SUB_m_(m)  COMMAn(n)   -> SUB WORD PTR g, n
    rDEC1,PUTwm1,rINC1,0,go|p1,SUB_m_,go|p1,COMMAn,go|m1,0, 0,

    // 008 Post-Inc Byte, Global
    // Fold post-increment of a global byte variable into a direct memory ADD.
    //     IN:  rINC1(n)  PUTbm1(m)  rDEC1(n)
    //          AX+=n     g=AL        AX-=n
    //     NET: g+=n, AX restored to pre-inc value
    //     WHY: Mirrors Post-Dec Byte, Global; 8086 ADD [mem],imm adds to a named memory location without reg.
    //     OUT: ADDm_(m)  COMMAn(n)  ->  ADD BYTE PTR g, n
    rINC1,PUTbm1,rDEC1,0,go|p1,ADDm_,go|p1,COMMAn,go|m1,0, 0,

    // 009 Post-Inc Word, Global
    // Same as Post-Inc Byte, Global for a word: fold post-increment of a global int into a direct memory ADD.
    //     IN:  rINC1(n)  PUTwm1(m)  rDEC1(n)
    //          AX+=n     g=AX        AX-=n
    //     NET: g+=n, AX restored to pre-inc value
    //     WHY: 8086 ADD [mem],imm adds to a named memory location without a register.
    //     OUT: ADDm_(m)  COMMAn(n)  ->  ADD WORD PTR g, n
    rINC1,PUTwm1,rDEC1,0,go|p1,ADDm_,go|p1,COMMAn,go|m1,0, 0,

    // 010 Post-Inc Byte, Ptr
    // Fold post-increment of a byte through a pointer into a direct memory INC/ADD.
    //     IN:  rINC1(n)  PUTbp1   rDEC1(n)
    //          AX+=n     [BX]=AL  AX-=n
    //     NET: [BX]+=n, AX restored to pre-inc value
    //     WHY: Mirrors Post-Dec Byte, Ptr; 8086 INC/ADD BYTE PTR [BX] modifies memory directly.
    //     OUT (n=1):  INCbp      ->  INC BYTE PTR [BX]
    //     OUT (n!=1): ADDbpn(n)  ->  ADD BYTE PTR [BX], n
    rINC1,PUTbp1,rDEC1,0,go|p2,ife|p1,INCbp,0,ADDbpn,0, 0,

    // 011 Post-Inc Word, Ptr
    // Same as Post-Inc Byte, Ptr for a word: fold post-increment through a pointer into INC/ADD word.
    //     IN:  rINC1(n)  PUTwp1   rDEC1(n)
    //          AX+=n     [BX]=AX  AX-=n
    //     NET: [BX]+=n, AX restored to pre-inc value
    //     WHY: Mirrors Post-Dec Word, Ptr; 8086 INC/ADD WORD PTR [BX] modifies memory directly.
    //     OUT (n=1):  INCwp      ->  INC WORD PTR [BX]
    //     OUT (n!=1): ADDwpn(n)  ->  ADD WORD PTR [BX], n
    rINC1,PUTwp1,rDEC1,0,go|p2,ife|p1,INCwp,0,ADDwpn,0, 0,

    // 012 Global Indexed Byte, Signed
    // Fold global-pointer + constant index into a single base+displacement signed byte load.
    //     IN:  GETw1m(m)  GETw2n(n)  ADD12   MOVE21  GETb1p(0)
    //          AX=*m      BX=n       AX+=BX  BX=AX   AL=s8[BX+0], CBW
    //     NET: AX = s8[*m + n]
    //     WHY: ADD12+MOVE21 put g+n into BX for [BX+0]. The 8086 MOV AL,[BX+disp16] addressing
    //          mode encodes a signed 16-bit displacement directly in instruction.
    //     OUT: GETw2m(m)  GETb1p(n)
    //          BX=*m      AL=s8[BX+n], CBW
    GETw1m,GETw2n,ADD12,MOVE21,GETb1p,0,go|p4,gv|m3,go|m1,GETw2m,gv|m3,0, 0,

    // 013 Global Indexed Byte, Unsigned
    // Same as Global Indexed Byte, Signed for an unsigned byte load.
    //     IN:  GETw1m(m)  GETw2n(n)  ADD12   MOVE21  GETb1pu(0)
    //          AX=*m      BX=n       AX+=BX  BX=AX   AL=u8[BX+0], XOR AH,AH
    //     NET: AX = u8[*m + n]
    //     WHY: Same [BX+disp16] folding as 07; GETb1pu zero-extends instead of sign-extends.
    //     OUT: GETw2m(m)  GETb1pu(n)
    //          BX=*m      AL=u8[BX+n], XOR AH,AH
    GETw1m,GETw2n,ADD12,MOVE21,GETb1pu,0,go|p4,gv|m3,go|m1,GETw2m,gv|m3,0, 0,

    // 014 Global Indexed Word
    // Same as Global Indexed Byte, Signed for a word load.
    //     IN:  GETw1m(m)  GETw2n(n)  ADD12   MOVE21  GETw1p(0)
    //          AX=*m      BX=n       AX+=BX  BX=AX   AX=w[BX+0]
    //     NET: AX = w[*m + n]
    //     WHY: Same [BX+disp16] folding as 07; MOV AX,[BX+disp16] loads a full word.
    //     OUT: GETw2m(m)  GETw1p(n)
    //          BX=*m      AX=w[BX+n]
    GETw1m,GETw2n,ADD12,MOVE21,GETw1p,0,go|p4,gv|m3,go|m1,GETw2m,gv|m3,0, 0,

    // 015 Load Global Pair, Swap-Free
    // Load two globals in reversed register order, eliminating SWAP12.
    //     IN:  GETw1m(m1)  GETw2m(m2)  SWAP12
    //          AX=m1       BX=m2       AX<->BX
    //     NET: AX=m2, BX=m1
    //     WHY: The compiler loaded operands in evaluation order (left->pr, right->sr) then
    //          inserted SWAP12 because the subsequent op needed them reversed. Loading them
    //          in the required order directly eliminates the swap.
    //     OUT: GETw2m(m1)  GETw1m(m2)
    //          BX=m1       AX=m2
    GETw1m,GETw2m,SWAP12,0,go|p2,GETw1m,gv|m1,go|m1,gv|m1,0, 0,

    // 016 Load Global Direct to Sr
    // Load a global directly into sr, skipping detour through pr.
    //     IN:  GETw1m(m)  MOVE21
    //          AX=m        BX=AX
    //     NET: BX=m
    //     WHY: GETw2m loads directly into BX; pr load and copy are unnecessary.
    //     OUT: GETw2m(m)
    //          BX=m
    GETw1m,MOVE21,0,go|p1,GETw2m,gv|m1,0, 0,

    // 017 Push Global from Memory
    // Push a global word directly from memory when pr is not needed afterward.
    //     IN:  GETw1m(m)  PUSH1   (pfree: pr not live after this point)
    //          AX=m        push AX
    //     NET: stack <- m
    //     WHY: 8086 PUSH [mem] pushes from memory without touching a register.
    //     OUT: PUSHm(m)
    //          push m
    GETw1m,PUSH1,pfree,0,go|p1,PUSHm,gv|m1,0, 0,

    // 018 Store Const Byte, Global
    // Store a constant directly to a global byte without loading it into a register.
    //     IN:  GETw1n(n)  PUTbm1(m)  (pfree: pr not live after this point)
    //          AX=n       [m]=AL
    //     NET: m (byte) = n
    //     WHY: 8086 MOV BYTE PTR [mem],imm stores an immediate to memory; no register needed.
    //     OUT: PUT_m_(m)  COMMAn(n)  ->  MOV BYTE PTR m, n
    GETw1n,PUTbm1,pfree,0,PUT_m_,go|p1,COMMAn,go|m1,swv|p1,0, 0,

    // 019 Store Const Word, Global
    // Same as Store Const Byte, Global for a word store.
    //     IN:  GETw1n(n)  PUTwm1(m)  (pfree: pr not live after this point)
    //          AX=n       [m]=AX
    //     NET: m (word) = n
    //     WHY: 8086 MOV WORD PTR [mem],imm stores an immediate to memory; no register needed.
    //     OUT: PUT_m_(m)  COMMAn(n)  ->  MOV WORD PTR m, n
    GETw1n,PUTwm1,pfree,0,PUT_m_,go|p1,COMMAn,go|m1,swv|p1,0, 0,

    // 020 Push Ptr-Indirect Word
    // Same as Push Global from Memory for a pointer-indirect word: push through sr ptr when pr is not needed.
    //     IN:  GETw1p(n)  PUSH1   (pfree: pr not live after this point)
    //          AX=w[BX+n]  push AX
    //     NET: stack <- w[BX+n]
    //     WHY: 8086 PUSH WORD PTR [BX+disp] pushes from memory without touching a register.
    //     OUT: PUSHp(n)  ->  PUSH WORD PTR [BX+n]
    GETw1p,PUSH1,pfree,0,go|p1,PUSHp,gv|m1,0, 0,

    // 021 Stack-Var Add Const Fold
    // Fold stack-variable + constant offset into direct sr load and sr add.
    //     IN:  GETw1s(o)  GETw2n(n)  ADD12  MOVE21
    //          AX=stack[o]  BX=n     AX+=BX  BX=AX
    //     NET: BX = stack[o] + n
    //     WHY: GETw2s loads stack slot directly into sr; ADD2n adds n to sr in place.
    //     OUT: GETw2s(o)  ADD2n(n)
    //          BX=stack[o]  BX+=n
    GETw1s,GETw2n,ADD12,MOVE21,0,go|p3,ADD2n,gv|m2,go|m1,GETw2s,gv|m2,0, 0,

    // 022 Load Stack Pair, Swap-Free
    // Load two stack variables in reversed register order, eliminating SWAP12.
    //     IN:  GETw1s(o1)  GETw2s(o2)  SWAP12
    //          AX=stack[o1]  BX=stack[o2]  AX<->BX
    //     NET: AX=stack[o2], BX=stack[o1]
    //     WHY: Same root cause as Load Global Pair, Swap-Free: compiler loaded operands in evaluation order then
    //          inserted SWAP12; load them in the required order from start.
    //     OUT: GETw2s(o1)  GETw1s(o2)
    //          BX=stack[o1]  AX=stack[o2]
    GETw1s,GETw2s,SWAP12,0,go|p2,GETw1s,gv|m1,go|m1,GETw2s,gv|m1,0, 0,

    // 023 Load Stack Var Direct to Sr
    // Load a stack variable directly into sr, skipping detour through pr.
    //     IN:  GETw1s(o)  MOVE21
    //          AX=stack[o]  BX=AX
    //     NET: BX = stack[o]
    //     WHY: Same as Load Global Direct to Sr but for a stack variable; GETw2s loads directly into BX.
    //     OUT: GETw2s(o)
    //          BX=stack[o]
    GETw1s,MOVE21,0,go|p1,GETw2s,gv|m1,0, 0,

    // 024 Global Sub Const, Swap-Free
    // Eliminate SWAP12 from a global-load + constant subtract by reversing operand order.
    //     IN:  GETw2m(m)  GETw1n(n)  SWAP12  SUB12
    //          BX=mem[m]  AX=n       AX<->BX  AX-=BX
    //     NET: AX = mem[m] - n
    //     WHY: The compiler staged mem[m] in sr and needed SWAP to put it in pr for SUB12.
    //          GETw1m loads straight into pr; SUB1n subtracts constant directly.
    //     OUT: GETw1m(m)  SUB1n(n)
    //          AX=mem[m]  AX-=n
    GETw2m,GETw1n,SWAP12,SUB12,0,go|p3,SUB1n,gv|m2,go|m1,GETw1m,gv|m2,0, 0,

    // 025 Add12 Fold to Add1n
    // Replace GETw2n+ADD12 with ADD1n: fold constant load into a direct pr add.
    //     IN:  GETw2n(n)  ADD12
    //          BX=n       AX+=BX
    //     NET: AX += n
    //     WHY: ADD1n(n) adds a constant to pr without touching sr.
    //     OUT: ADD1n(n)  ->  ADD AX, n  (if |n|<=2, Pr Imm-Add to Inc/Dec immediately reduces to INC/DEC AX)
    GETw2n,ADD12,0,go|p1,ADD1n,gv|m1,0, 0,

    // 026 Stack Sub Const, Swap-Free
    // Same as Global Sub Const, Swap-Free for a stack variable: eliminate SWAP12 by reversing operand order.
    //     IN:  GETw2s(o)  GETw1n(n)  SWAP12  SUB12
    //          BX=stack[o]  AX=n     AX<->BX  AX-=BX
    //     NET: AX = stack[o] - n
    //     WHY: Same as Global Sub Const, Swap-Free; GETw1s loads straight into pr, SUB1n subtracts directly.
    //     OUT: GETw1s(o)  SUB1n(n)
    //          AX=stack[o]  AX-=n
    GETw2s,GETw1n,SWAP12,SUB12,0,go|p3,SUB1n,gv|m2,go|m1,GETw1s,gv|m2,0, 0,

    // 027 Sub Fold to Sub1n
    // Collapse MOVE21+GETw1n+SWAP12+SUB12 into a single constant subtract from pr.
    //     IN:  MOVE21  GETw1n(n)  SWAP12  SUB12
    //          BX=AX   AX=n       AX<->BX  AX-=BX
    //     NET: AX -= n
    //     WHY: The four-p-code sequence just computes AX - n; SUB1n does it directly.
    //          May then trigger Pr Imm-Sub to Dec/Inc for small n (INC/DEC reduction).
    //     OUT: SUB1n(n)  ->  SUB AX, n
    MOVE21,GETw1n,SWAP12,SUB12,0,go|p3,SUB1n,gv|m2,0, 0,

    // 028 Commutative N-Load Fold
    // Eliminate MOVE21+GETw1n when the following op is commutative.
    //     IN:  MOVE21  GETw1n(n)  [comm: next op is commutative]
    //          BX=AX   AX=n
    //     NET: AX=n, BX=old AX
    //     WHY: The commutative op does not care which operand is in which register.
    //          GETw2n loads n into sr directly, leaving pr unchanged.
    //     OUT: GETw2n(n)  ->  MOV BX, n
    MOVE21,GETw1n,comm,0,go|p1,GETw2n,0, 0,

    // 029 Point-Mem Offset Fold
    // Fold address-of-global + constant offset into a two-part sr-pointer-with-displacement.
    //     IN:  POINT1m(m)  GETw2n(n)  ADD12  MOVE21
    //          AX=&m        BX=n      AX+=BX  BX=AX
    //     NET: BX = &m + n
    //     WHY: POINT2m_ + PLUSn encode LEA BX,[m+n] as a two-part p-code; no register add needed.
    //     OUT: POINT2m_(m)  PLUSn(n)  ->  MOV BX, OFFSET m+n
    POINT1m,GETw2n,ADD12,MOVE21,0,go|p3,PLUSn,gv|m2,go|m1,POINT2m_,gv|m2,0, 0,

    // 030 Point-Mem Direct to Sr
    // Load address of a global directly into sr when pr is not needed afterward.
    //     IN:  POINT1m(m)  MOVE21  (pfree: pr not live after this point)
    //          AX=&m        BX=AX
    //     NET: BX = &m
    //     WHY: POINT2m loads &m directly into BX; pr load and copy are unnecessary.
    //     OUT: POINT2m(m)  ->  MOV BX, OFFSET m
    POINT1m,MOVE21,pfree,0,go|p1,POINT2m,gv|m1,0, 0,


    // 031 Stack Address Offset Fold
    // Fold POINT1s + constant offset + ADD12 + MOVE21 into POINT2s with the combined displacement.
    //     IN:  POINT1s(o)  GETw2n(n)  ADD12   MOVE21
    //          AX=&stack[o]  BX=n     AX+=BX  BX=AX
    //     NET: BX = &stack[o] + n
    //     WHY: POINT2s encodes a stack-frame offset directly; no register add is needed.
    //     OUT: POINT2s(o+n)
    POINT1s,GETw2n,ADD12,MOVE21,0,sum|p1,go|p3,POINT2s,gv|m3,0, 0,

    // 032 Stack Point, Push and Copy
    // Replace POINT1s+PUSH1+MOVE21 with POINT2s+PUSH2, loading the stack address into sr and pushing from there.
    //     IN:  POINT1s(o)  PUSH1        MOVE21
    //          AX=&stack[o]  push AX   BX=AX
    //     NET: stack <- &stack[o], BX = &stack[o]
    //     WHY: POINT2s loads directly into BX; PUSH2 pushes BX, combining the push and
    //          copy without using pr.
    //     OUT: POINT2s(o)  PUSH2
    POINT1s,PUSH1,MOVE21,0,go|p1,POINT2s,gv|m1,go|p1,PUSH2,go|m1,0, 0,


    // 033 Stack Address Direct to Sr
    // Same as Point-Mem Direct to Sr for a stack address: replace POINT1s+MOVE21 with POINT2s.
    //     IN:  POINT1s(o)  MOVE21
    //          AX=&stack[o]  BX=AX
    //     NET: BX = &stack[o]
    //     WHY: POINT2s loads &stack[o] directly into BX; the pr load and copy are unnecessary.
    //     OUT: POINT2s(o)
    POINT1s,MOVE21,0,go|p1,POINT2s,gv|m1,0, 0,

    // 034 Fold Global Ptr to Signed Byte Load
    // Fold POINT2m+GETb1p into a direct global signed byte load, eliminating the pointer register.
    //     IN:  POINT2m(m)  GETb1p(0)  (sfree: sr not live after this point)
    //          BX=&m       AX=s8[BX+0], CBW
    //     NET: AX = s8[m]
    //     WHY: GETb1m encodes the symbol name directly; MOV AL,[m] CBW needs no BX.
    //     OUT: GETb1m(m)  ->  MOV AL,[m]  CBW
    POINT2m,GETb1p,sfree,0,go|p1,GETb1m,gv|m1,0, 0,

    // 035 Fold Global Ptr to Unsigned Byte Load
    // Same as Fold Global Ptr to Signed Byte Load for an unsigned byte: fold POINT2m+GETb1pu into GETb1mu.
    //     IN:  POINT2m(m)  GETb1pu(0)  (sfree: sr not live after this point)
    //          BX=&m       AL=u8[BX+0], XOR AH,AH
    //     NET: AX = u8[m]
    //     WHY: GETb1mu encodes the symbol name directly; MOV AL,[m] XOR AH,AH needs no BX.
    //     OUT: GETb1mu(m)  ->  MOV AL,[m]  XOR AH,AH
    POINT2m,GETb1pu,sfree,0,go|p1,GETb1mu,gv|m1,0, 0,

    // 036 Fold Global Ptr to Word Load
    // Same as Fold Global Ptr to Signed Byte Load for a word: fold POINT2m+GETw1p into GETw1m.
    //     IN:  POINT2m(m)  GETw1p(0)  (sfree: sr not live after this point)
    //          BX=&m       AX=w[BX+0]
    //     NET: AX = w[m]
    //     WHY: GETw1m encodes the symbol name directly; MOV AX,[m] needs no BX.
    //     OUT: GETw1m(m)  ->  MOV AX,[m]
    POINT2m,GETw1p,sfree,0,go|p1,GETw1m,gv|m1,0, 0,

    // 037 Fold Displaced Global Ptr to Word Load
    // Fold into GETw1m_+PLUSn: collapse pointer-plus-offset + load into a single displaced global word load.
    //     IN:  POINT2m_(m)  PLUSn(n)  GETw1p(0)  (sfree: sr not live after this point)
    //          BX=&m         BX+=n    AX=w[BX+0]
    //     NET: AX = w[m+n]
    //     WHY: GETw1m_+PLUSn encodes the base symbol and displacement directly;
    //          MOV AX,[m+n] needs no register arithmetic.
    //     OUT: GETw1m_(m)  PLUSn(n)  ->  MOV AX,[m+n]
    POINT2m_,PLUSn,GETw1p,sfree,0,go|p2,gc|m1,gv|m1,go|m1,GETw1m_,gv|m1,0, 0,

    // 038 Fold Stack Ptr to Signed Byte Load
    // Fold POINT2s+GETb1p into a direct stack signed byte load, eliminating the pointer register.
    //     IN:  POINT2s(o)    GETb1p(0)  (sfree: sr not live after this point)
    //          BX=&stack[o]  AX=s8[BX+0], CBW
    //     NET: AX = s8[stack+o]
    //     WHY: GETb1s encodes the stack offset directly; MOV AL,[BP+o] CBW needs no BX.
    //     OUT: GETb1s(o)  ->  MOV AL,[BP+o]  CBW
    POINT2s,GETb1p,sfree,0,sum|p1,go|p1,GETb1s,gv|m1,0, 0,

    // 039 Fold Stack Ptr to Unsigned Byte Load
    // Same as Fold Stack Ptr to Signed Byte Load for an unsigned byte: fold POINT2s+GETb1pu into GETb1su.
    //     IN:  POINT2s(o)    GETb1pu(0)  (sfree: sr not live after this point)
    //          BX=&stack[o]  AL=u8[BX+0], XOR AH,AH
    //     NET: AX = u8[stack+o]
    //     WHY: GETb1su encodes the stack offset directly; MOV AL,[BP+o] XOR AH,AH needs no BX.
    //     OUT: GETb1su(o)  ->  MOV AL,[BP+o]  XOR AH,AH
    POINT2s,GETb1pu,sfree,0,sum|p1,go|p1,GETb1su,gv|m1,0, 0,

    // 040 Push Stack Word via Ptr
    // Fold POINT2s+GETw1p+PUSH1 into PUSHs, pushing a stack variable directly without loading it into pr.
    //     IN:  POINT2s(o)    GETw1p(0)  PUSH1   (pfree: pr not live after this point)
    //          BX=&stack[o]  AX=w[BX]   push AX
    //     NET: stack <- w[stack+o]
    //     WHY: PUSHs encodes the stack offset directly; 8086 PUSH [BP+disp] pushes from
    //          memory without a register round-trip.
    //     OUT: PUSHs(o)  ->  PUSH WORD PTR [BP+o]
    POINT2s,GETw1p,PUSH1,pfree,0,sum|p1,go|p2,PUSHs,gv|m2,0, 0,

    // 041 Fold Stack Ptr to Word Load
    // Fold POINT2s+GETw1p into a direct stack word load, eliminating the pointer register.
    //     IN:  POINT2s(o)    GETw1p(0)  (sfree: sr not live after this point)
    //          BX=&stack[o]  AX=w[BX]
    //     NET: AX = w[stack+o]
    //     WHY: GETw1s encodes the stack offset directly; MOV AX,[BP+o] needs no BX.
    //     OUT: GETw1s(o)  ->  MOV AX,[BP+o]
    POINT2s,GETw1p,sfree,0,sum|p1,go|p1,GETw1s,gv|m1,0, 0,

    // 042 Store Word via Stack Pointer to Direct Stack Store
    // Fold POINT2s+PUTwp1 into PUTws1: computed stack address + indirect word store -> direct BP-relative store.
    //     IN:  POINT2s(o)       PUTwp1     (sfree: sr not live after this point)
    //          BX=&stack[o]     [BX]=AX
    //     NET: stack[o] = AX
    //     WHY: POINT2s computes BP+o into BX only to be immediately dereferenced; PUTws1 encodes
    //          the slot offset directly and emits MOV [BP+o],AX without any BX computation.
    //     OUT: PUTws1(o)  ->  MOV <o>[BP],AX
    POINT2s,PUTwp1,sfree,0,sum|p1,go|p1,PUTws1,gv|m1,0, 0,

    // 043 Store Byte via Stack Pointer to Direct Stack Store
    // Same as Store Word via Stack Pointer for a byte: fold POINT2s+PUTbp1 into PUTbs1.
    //     IN:  POINT2s(o)       PUTbp1     (sfree: sr not live after this point)
    //          BX=&stack[o]     [BX]=AL
    //     NET: stack[o] (byte) = AL
    //     WHY: Same as Store Word via Stack Pointer; PUTbs1 emits MOV BYTE PTR [BP+o],AL directly.
    //     OUT: PUTbs1(o)  ->  MOV BYTE PTR <o>[BP],AL
    POINT2s,PUTbp1,sfree,0,sum|p1,go|p1,PUTbs1,gv|m1,0, 0,

    // 044 Push/Pop Pair to Move21
    // Replace PUSH1+any+POP2 with MOVE21+any when any does not clobber sr, eliminating the stack round-trip.
    //     IN:  PUSH1    any       POP2
    //          push AX  <any op>  BX=popped
    //     NET: BX = AX (before any), any executes with BX holding old AX
    //     WHY: If any does not write BX, MOVE21 before any achieves the same result;
    //          MOV BX,AX is shorter than a push/pop pair.
    //     OUT: MOVE21  any  ->  MOV BX,AX  <any op>
    PUSH1,any,POP2,0,go|p2,gc|m1,gv|m1,go|m1,MOVE21,0, 0,

    // 045 Push-Through-Ptr/Pop Pair to Reload
    // Replace PUSHp+any+POP2 with any+GETw2p when any does not clobber sr, eliminating the stack round-trip.
    //     IN:  PUSHp       any       POP2
    //          push w[BX]  <any op>  BX=popped
    //     NET: BX = w[BX] (before any), any executes, then w[BX] is back in BX
    //     WHY: If any does not change BX, the original w[BX] is still reachable;
    //          GETw2p reloads it after any instead of saving/restoring through the stack.
    //     OUT: any  GETw2p(0)  ->  <any op>  MOV BX,[BX]
    PUSHp,any,POP2,0,go|p2,gc|m1,gv|m1,go|m1,GETw2p,gv|m1,0, 0,


    // 046 Constant-1 Left Shift to Single-Bit Shift
    // Replace GETw1n(1)+ASL12 with ASL1_1 when the shift count is exactly 1.
    //     IN:  GETw1n(1)  ASL12
    //          AX=1       AX = BX << AX  (MOV CX,AX / MOV AX,BX / SAL AX,CL)
    //     NET: AX = BX << 1
    //     WHY: ASL12 copies count from AX to CX, moves BX to AX, then shifts by CL (6 bytes).
    //          ASL1_1 uses shorter 8086 one-bit form MOV AX,BX / SHL AX,1 (4 bytes).
    //          Only fires when the shift count is exactly 1.
    //     OUT: ASL1_1  ->  MOV AX,BX / SHL AX,1
    GETw1n,ASL12,0,ife|p1,go|p1,ASL1_1,0, 0,
    
    // 047 Constant-1 Right Shift to Single-Bit Shift
    // Same as Constant-1 Left Shift for arithmetic right shift: replace GETw1n(1)+ASR12 with ASR1_1.
    //     IN:  GETw1n(1)  ASR12
    //          AX=1       AX = BX >> AX  (signed; MOV CX,AX / MOV AX,BX / SAR AX,CL)
    //     NET: AX = BX >> 1  (arithmetic)
    //     WHY: Same as Constant-1 Left Shift; ASR1_1 emits MOV AX,BX / SAR AX,1 (4 bytes)
    //          vs ASR12's MOV CX,AX / MOV AX,BX / SAR AX,CL (6 bytes).
    //     OUT: ASR1_1  ->  MOV AX,BX / SAR AX,1
    GETw1n,ASR12,0,ife|p1,go|p1,ASR1_1,0, 0,

    // 048 Logical Negate Before False-Branch to True-Branch
    // Fold LNEG1+NE10f into EQ10f: eliminate boolean inversion before a conditional branch.
    //     IN:  LNEG1         NE10f(t)
    //          AX=!AX        branch-to-t if AX==0
    //     NET: branch to t if original AX != 0
    //     WHY: LNEG1 emits CALL __lneg (maps nonzero->0, zero->1); NE10f (OR AX,AX / JE) then
    //          branches when the negated value is zero, i.e., when original pr was nonzero.
    //          EQ10f (OR AX,AX / JNE) tests the original pr directly, saving the CALL.
    //     OUT: EQ10f(t)  ->  OR AX,AX / JNE $+5 / JMP t
    LNEG1,NE10f,0,go|p1,EQ10f,0, 0,

    // 049 Logical Negate Before True-Branch to False-Branch
    // Fold LNEG1+EQ10f into NE10f: eliminate boolean inversion before a conditional branch.
    //     IN:  LNEG1         EQ10f(t)
    //          AX=!AX        branch-to-t if AX!=0
    //     NET: branch to t if original AX == 0
    //     WHY: Same as Logical Negate Before False-Branch; EQ10f branches when !pr is nonzero,
    //          i.e., when original pr was zero. NE10f (OR AX,AX / JE) tests original pr directly.
    //     OUT: NE10f(t)  ->  OR AX,AX / JE $+5 / JMP t
    LNEG1,EQ10f,0,go|p1,NE10f,0, 0,
    // ========================================================================
    // CMP-Immediate Rules (70-89)
    // These match GETw2n + XX12 + test-zero when BX is dead (sfree), and
    // rewrite the 3 slots to CMP1n + NOP_ + JccXX, saving 2 bytes per
    // constant comparison by eliminating the MOV BX,n load.
    // Uses write-forward-then-back: CMP1n at pos0, NOP_ at pos1, JccXX at
    // pos2, then go|m2 returns cursor to pos0 for sequential emission.
    // ========================================================================

    // A-group: XX12 + NE10f (branch when condition is FALSE)

    // 050 CMP-Imm Equal, Branch-on-False
    //     IN:  GETw2n(c) EQ12  NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccE(t)    ->  CMP AX,c / JE $+5 / JMP t
    GETw2n,EQ12,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccE,go|m2,0, 0,

    // 051 CMP-Imm Not-Equal, Branch-on-False
    //     IN:  GETw2n(c) NE12  NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccNE(t)   ->  CMP AX,c / JNE $+5 / JMP t
    GETw2n,NE12,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccNE,go|m2,0, 0,

    // 052 CMP-Imm Signed Less-Than, Branch-on-False
    //     IN:  GETw2n(c) LT12  NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccG(t)    ->  CMP AX,c / JG $+5 / JMP t
    GETw2n,LT12,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccG,go|m2,0, 0,

    // 053 CMP-Imm Signed Greater-Than, Branch-on-False
    //     IN:  GETw2n(c) GT12  NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccL(t)    ->  CMP AX,c / JL $+5 / JMP t
    GETw2n,GT12,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccL,go|m2,0, 0,

    // 054 CMP-Imm Signed Less-or-Equal, Branch-on-False
    //     IN:  GETw2n(c) LE12  NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccGE(t)   ->  CMP AX,c / JGE $+5 / JMP t
    GETw2n,LE12,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccGE,go|m2,0, 0,

    // 055 CMP-Imm Signed Greater-or-Equal, Branch-on-False
    //     IN:  GETw2n(c) GE12  NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccLE(t)   ->  CMP AX,c / JLE $+5 / JMP t
    GETw2n,GE12,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccLE,go|m2,0, 0,

    // 056 CMP-Imm Unsigned Less-Than, Branch-on-False
    //     IN:  GETw2n(c) LT12u NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccA(t)    ->  CMP AX,c / JA $+5 / JMP t
    GETw2n,LT12u,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccA,go|m2,0, 0,

    // 057 CMP-Imm Unsigned Greater-Than, Branch-on-False
    //     IN:  GETw2n(c) GT12u NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccB(t)    ->  CMP AX,c / JB $+5 / JMP t
    GETw2n,GT12u,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccB,go|m2,0, 0,

    // 058 CMP-Imm Unsigned Less-or-Equal, Branch-on-False
    //     IN:  GETw2n(c) LE12u NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccAE(t)   ->  CMP AX,c / JAE $+5 / JMP t
    GETw2n,LE12u,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccAE,go|m2,0, 0,

    // 059 CMP-Imm Unsigned Greater-or-Equal, Branch-on-False
    //     IN:  GETw2n(c) GE12u NE10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccBE(t)   ->  CMP AX,c / JBE $+5 / JMP t
    GETw2n,GE12u,NE10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccBE,go|m2,0, 0,

    // B-group: XX12 + EQ10f (branch when condition is TRUE)

    // 060 CMP-Imm Equal, Branch-on-True
    //     IN:  GETw2n(c) EQ12  EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccNE(t)   ->  CMP AX,c / JNE $+5 / JMP t
    GETw2n,EQ12,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccNE,go|m2,0, 0,

    // 061 CMP-Imm Not-Equal, Branch-on-True
    //     IN:  GETw2n(c) NE12  EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccE(t)    ->  CMP AX,c / JE $+5 / JMP t
    GETw2n,NE12,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccE,go|m2,0, 0,

    // 062 CMP-Imm Signed Less-Than, Branch-on-True
    //     IN:  GETw2n(c) LT12  EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccLE(t)   ->  CMP AX,c / JLE $+5 / JMP t
    GETw2n,LT12,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccLE,go|m2,0, 0,

    // 063 CMP-Imm Signed Greater-Than, Branch-on-True
    //     IN:  GETw2n(c) GT12  EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccGE(t)   ->  CMP AX,c / JGE $+5 / JMP t
    GETw2n,GT12,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccGE,go|m2,0, 0,

    // 064 CMP-Imm Signed Less-or-Equal, Branch-on-True
    //     IN:  GETw2n(c) LE12  EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccL(t)    ->  CMP AX,c / JL $+5 / JMP t
    GETw2n,LE12,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccL,go|m2,0, 0,

    // 065 CMP-Imm Signed Greater-or-Equal, Branch-on-True
    //     IN:  GETw2n(c) GE12  EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccG(t)    ->  CMP AX,c / JG $+5 / JMP t
    GETw2n,GE12,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccG,go|m2,0, 0,

    // 066 CMP-Imm Unsigned Less-Than, Branch-on-True
    //     IN:  GETw2n(c) LT12u EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccBE(t)   ->  CMP AX,c / JBE $+5 / JMP t
    GETw2n,LT12u,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccBE,go|m2,0, 0,

    // 067 CMP-Imm Unsigned Greater-Than, Branch-on-True
    //     IN:  GETw2n(c) GT12u EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccAE(t)   ->  CMP AX,c / JAE $+5 / JMP t
    GETw2n,GT12u,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccAE,go|m2,0, 0,

    // 068 CMP-Imm Unsigned Less-or-Equal, Branch-on-True
    //     IN:  GETw2n(c) LE12u EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccB(t)    ->  CMP AX,c / JB $+5 / JMP t
    GETw2n,LE12u,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccB,go|m2,0, 0,

    // 069 CMP-Imm Unsigned Greater-or-Equal, Branch-on-True
    //     IN:  GETw2n(c) GE12u EQ10f(t)  (sfree)
    //     OUT: CMP1n(c)  NOP_  JccA(t)    ->  CMP AX,c / JA $+5 / JMP t
    GETw2n,GE12u,EQ10f,sfree,0,CMP1n,go|p1,NOP_,go|p1,JccA,go|m2,0, 0,

    // Fold Equal Comparison and Branch into CMP/JNE
    // ========================================================================
    // Comparison Fold Rules
    // Match XX12 + NE10f/EQ10f (library comparison call + zero-test branch)
    // and replace with CMP12 + JccXX (inline CMP AX,BX + factored branch).
    // Saves 3 bytes per comparison by eliminating the CALL __xx overhead.
    // Uses write-forward-then-back: CMP12 at pos0, JccXX at pos1, go|m1
    // returns cursor to pos0 for sequential emission.
    // ========================================================================

    // A-group: XX12 + NE10f (branch when condition is FALSE)

    // 070 EQ12+NE10f -> CMP12+JccE (branch when sr != pr)
    EQ12,NE10f,0,CMP12,go|p1,JccE,go|m1,0, 0,
    // 071 NE12+NE10f -> CMP12+JccNE (branch when sr == pr)
    NE12,NE10f,0,CMP12,go|p1,JccNE,go|m1,0, 0,
    // 072 LT12+NE10f -> CMP12+JccG (branch when sr >= pr)
    LT12,NE10f,0,CMP12,go|p1,JccG,go|m1,0, 0,
    // 072 GT12+NE10f -> CMP12+JccL (branch when sr <= pr)
    GT12,NE10f,0,CMP12,go|p1,JccL,go|m1,0, 0,
    // 073 LE12+NE10f -> CMP12+JccGE (branch when sr > pr)
    LE12,NE10f,0,CMP12,go|p1,JccGE,go|m1,0, 0,
    // 074 GE12+NE10f -> CMP12+JccLE (branch when sr < pr)
    GE12,NE10f,0,CMP12,go|p1,JccLE,go|m1,0, 0,
    // 075 LT12u+NE10f -> CMP12+JccA (branch when sr >= pr unsigned)
    LT12u,NE10f,0,CMP12,go|p1,JccA,go|m1,0, 0,
    // 076 GT12u+NE10f -> CMP12+JccB (branch when sr <= pr unsigned)
    GT12u,NE10f,0,CMP12,go|p1,JccB,go|m1,0, 0,
    // 077 LE12u+NE10f -> CMP12+JccAE (branch when sr > pr unsigned)
    LE12u,NE10f,0,CMP12,go|p1,JccAE,go|m1,0, 0,
    // 078 GE12u+NE10f -> CMP12+JccBE (branch when sr < pr unsigned)
    GE12u,NE10f,0,CMP12,go|p1,JccBE,go|m1,0, 0,

    // B-group: XX12 + EQ10f (branch when condition is TRUE — inverted Jcc)

    // 079 EQ12+EQ10f -> CMP12+JccNE (branch when sr == pr)
    EQ12,EQ10f,0,CMP12,go|p1,JccNE,go|m1,0, 0,
    // 080 NE12+EQ10f -> CMP12+JccE (branch when sr != pr)
    NE12,EQ10f,0,CMP12,go|p1,JccE,go|m1,0, 0,
    // 081 LT12+EQ10f -> CMP12+JccLE (branch when sr < pr)
    LT12,EQ10f,0,CMP12,go|p1,JccLE,go|m1,0, 0,
    // 082 GT12+EQ10f -> CMP12+JccGE (branch when sr > pr)
    GT12,EQ10f,0,CMP12,go|p1,JccGE,go|m1,0, 0,
    // 083 LE12+EQ10f -> CMP12+JccL (branch when sr <= pr)
    LE12,EQ10f,0,CMP12,go|p1,JccL,go|m1,0, 0,
    // 084 GE12+EQ10f -> CMP12+JccG (branch when sr >= pr)
    GE12,EQ10f,0,CMP12,go|p1,JccG,go|m1,0, 0,
    // 085 LT12u+EQ10f -> CMP12+JccBE (branch when sr < pr unsigned)
    LT12u,EQ10f,0,CMP12,go|p1,JccBE,go|m1,0, 0,
    // 086 GT12u+EQ10f -> CMP12+JccAE (branch when sr > pr unsigned)
    GT12u,EQ10f,0,CMP12,go|p1,JccAE,go|m1,0, 0,
    // 087 LE12u+EQ10f -> CMP12+JccB (branch when sr <= pr unsigned)
    LE12u,EQ10f,0,CMP12,go|p1,JccB,go|m1,0, 0,
    // 088 GE12u+EQ10f -> CMP12+JccA (branch when sr >= pr unsigned)
    GE12u,EQ10f,0,CMP12,go|p1,JccA,go|m1,0, 0,

    // These four _pop/topop rules eliminate PUSH/POP round-trips by reloading
    // at the pop site.  Disabled because the compiler crashes when they are
    // included in seqdata.
    //
    // Point-Mem Push/Pop to Reload
    //  0,POINT1m,PUSH1,pfree,_pop,0,topop|POINT2m,go|p2,0,
    //
    // Stack-Addr Push/Pop to Reload
    //  0,POINT1s,PUSH1,pfree,_pop,0,topop|POINT2s,go|p2,0,
    //
    // Global-Mem Push/Pop to Reload
    //  0,PUSHm,_pop,0,topop|GETw2m,go|p1,0,
    //
    // Stack-Var Push/Pop to Reload
    //  0,PUSHs,_pop,0,topop|GETw2s,go|p1,0,
    0  // end sentinel
};

// === assembly-code strings ==================================================

// First byte contains flag bits indicating:
//    value in ax is needed (010) or zapped (020)
//    value in bx is needed (001) or zapped (002)
char* code[PCODES] = {
    /* 0   (unused)   */ 0,
    /* 1   ADD12      */ "\211ADD AX,BX\n",
    /* 2   ADDSP      */ "\000?ADD SP,<n>\n??",
    /* 3   AND12      */ "\211AND AX,BX\n",
    /* 4   ANEG1      */ "\010NEG AX\n",
    /* 5   (removed)  */ "",  /* ARGCNTn: slot reserved; no longer emitted (rev 139) */
    /* 6   ASL12      */ "\011MOV CX,AX\nMOV AX,BX\nSAL AX,CL\n",
    /* 7   ASR12      */ "\011MOV CX,AX\nMOV AX,BX\nSAR AX,CL\n",
    /* 8   CALL1      */ "\010CALL AX\n",
    /* 9   CALLm      */ "\020CALL <m>\n",
    /* 10  BYTE_      */ "\000 DB ",
    /* 11  BYTEn      */ "\000 DB <n>\n",
    /* 12  BYTEr0     */ "\000 DB <n> DUP(0)\n",
    /* 13  COM1       */ "\010NOT AX\n",
    /* 14  DBL1       */ "\010SHL AX,1\n",
    /* 15  DBL2       */ "\001SHL BX,1\n",
    /* 16  DIV12      */ "\011CWD\nIDIV BX\n",                 /* see gen() */
    /* 17  DIV12u     */ "\011XOR DX,DX\nDIV BX\n",            /* see gen() */
    /* 18  ENTER      */ "\100PUSH BP\nMOV BP,SP\n",
    /* 19  EQ10f      */ "\010OR AX,AX\nJE $+5\nJMP _<n>\n",
    /* 20  EQ12       */ "\211CALL __eq\n",
    /* 21  GE10f      */ "\010OR AX,AX\nJGE $+5\nJMP _<n>\n",
    /* 22  GE12       */ "\011CALL __ge\n",
    /* 23  GE12u      */ "\011CALL __uge\n",
    /* 24  POINT1l    */ "\020MOV AX,OFFSET _<l>+<n>\n",
    /* 25  POINT1m    */ "\020MOV AX,OFFSET <m>\n",
    /* 26  GETb1m     */ "\020MOV AL,BYTE PTR <m>\nCBW\n",
    /* 27  GETb1mu    */ "\020MOV AL,BYTE PTR <m>\nXOR AH,AH\n",
    /* 28  GETb1p     */ "\021MOV AL,?<n>??[BX]\nCBW\n",       /* see gen() */
    /* 29  GETb1pu    */ "\021MOV AL,?<n>??[BX]\nXOR AH,AH\n", /* see gen() */
    /* 30  GETw1m     */ "\020MOV AX,WORD PTR <m>\n",
    /* 31  GETw1n     */ "\020?MOV AX,<n>?XOR AX,AX?\n",
    /* 32  GETw1p     */ "\021MOV AX,?<n>??[BX]\n",            /* see gen() */
    /* 33  GETw2n     */ "\002?MOV BX,<n>?XOR BX,BX?\n",
    /* 34  GT10f      */ "\010OR AX,AX\nJG $+5\nJMP _<n>\n",
    /* 35  GT12       */ "\010CALL __gt\n",
    /* 36  GT12u      */ "\011CALL __ugt\n",
    /* 37  WORD_      */ "\000 DW ",
    /* 38  WORDn      */ "\000 DW <n>\n",
    /* 39  WORDr0     */ "\000 DW <n> DUP(0)\n",
    /* 40  JMPm       */ "\000JMP _<n>\n",
    /* 41  LABm       */ "\000_<n>:\n",
    /* 42  LE10f      */ "\010OR AX,AX\nJLE $+5\nJMP _<n>\n",
    /* 43  LE12       */ "\011CALL __le\n",
    /* 44  LE12u      */ "\011CALL __ule\n",
    /* 45  LNEG1      */ "\010CALL __lneg\n",
    /* 46  LT10f      */ "\010OR AX,AX\nJL $+5\nJMP _<n>\n",
    /* 47  LT12       */ "\011CALL __lt\n",
    /* 48  LT12u      */ "\011CALL __ult\n",
    /* 49  MOD12      */ "\011CWD\nIDIV BX\nMOV AX,DX\n",      /* see gen() */
    /* 50  MOD12u     */ "\011XOR DX,DX\nDIV BX\nMOV AX,DX\n", /* see gen() */
    /* 51  MOVE21     */ "\012MOV BX,AX\n",
    /* 52  MUL12      */ "\211IMUL BX\n",
    /* 53  MUL12u     */ "\211MUL BX\n",
    /* 54  NE10f      */ "\010OR AX,AX\nJNE $+5\nJMP _<n>\n",
    /* 55  NE12       */ "\211CALL __ne\n",
    /* 56  NEARm      */ "\000 DW _<n>\n",
    /* 57  OR12       */ "\211OR AX,BX\n",
    /* 58  POINT1s    */ "\020LEA AX,<n>[BP]\n",
    /* 59  POP2       */ "\002POP BX\n",
    /* 60  PUSH1      */ "\110PUSH AX\n",
    /* 61  PUTbm1     */ "\010MOV BYTE PTR <m>,AL\n",
    /* 62  PUTbp1     */ "\011MOV [BX],AL\n",
    /* 63  PUTwm1     */ "\010MOV WORD PTR <m>,AX\n",
    /* 64  PUTwp1     */ "\011MOV [BX],AX\n",
    /* 65  rDEC1      */ "\010#DEC AX\n#",
    /* 66  REFm       */ "\000_<n>",
    /* 67  RETURN     */ "\000?MOV SP,BP\n??POP BP\nRET\n",
    /* 68  rINC1      */ "\010#INC AX\n#",
    /* 69  SUB12      */ "\011SUB AX,BX\n",                    /* see gen() */
    /* 70  SWAP12     */ "\011XCHG AX,BX\n",
    /* 71  SWAP1s     */ "\012POP BX\nXCHG AX,BX\nPUSH BX\n",
    /* 72  SWITCH     */ "\012CALL __switch\n",
    /* 73  XOR12      */ "\211XOR AX,BX\n",
    /* optimizer-generated */
    /* 74  ADD1n      */ "\010?ADD AX,<n>\n??",
    /* 75  ADD21      */ "\211ADD BX,AX\n",
    /* 76  ADD2n      */ "\010?ADD BX,<n>\n??",
    /* 77  ADDbpn     */ "\001ADD BYTE PTR [BX],<n>\n",
    /* 78  ADDwpn     */ "\001ADD WORD PTR [BX],<n>\n",
    /* 79  ADDm_      */ "\000ADD <m>",
    /* 80  COMMAn     */ "\000,<n>\n",
    /* 81  DECbp      */ "\001DEC BYTE PTR [BX]\n",
    /* 82  DECwp      */ "\001DEC WORD PTR [BX]\n",
    /* 83  POINT2m    */ "\002MOV BX,OFFSET <m>\n",
    /* 84  POINT2m_   */ "\002MOV BX,OFFSET <m>",
    /* 85  GETb1s     */ "\020MOV AL,<n>[BP]\nCBW\n",
    /* 86  GETb1su    */ "\020MOV AL,<n>[BP]\nXOR AH,AH\n",
    /* 87  GETw1m_    */ "\020MOV AX,WORD PTR <m>",
    /* 88  GETw1s     */ "\020MOV AX,<n>[BP]\n",
    /* 89  GETw2m     */ "\002MOV BX,WORD PTR <m>\n",
    /* 90  GETw2p     */ "\021MOV BX,?<n>??[BX]\n",
    /* 91  GETw2s     */ "\002MOV BX,<n>[BP]\n",
    /* 92  INCbp      */ "\001INC BYTE PTR [BX]\n",
    /* 93  INCwp      */ "\001INC WORD PTR [BX]\n",
    /* 94  PLUSn      */ "\000?+<n>??\n",
    /* 95  POINT2s    */ "\002LEA BX,<n>[BP]\n",
    /* 96  PUSH2      */ "\101PUSH BX\n",
    /* 97  PUSHm      */ "\100PUSH WORD PTR <m>\n",
    /* 98  PUSHp      */ "\100PUSH ?<n>??[BX]\n",
    /* 99  PUSHs      */ "\100PUSH ?<n>??[BP]\n",
    /* 100 PUT_m_     */ "\000MOV <m>",
    /* 101 rDEC2      */ "\010#DEC BX\n#",
    /* 102 rINC2      */ "\010#INC BX\n#",
    /* 103 SUB_m_     */ "\000SUB <m>",
    /* 104 SUB1n      */ "\010?SUB AX,<n>\n??",
    /* 105 SUBbpn     */ "\001SUB BYTE PTR [BX],<n>\n",
    /* 106 SUBwpn     */ "\001SUB WORD PTR [BX],<n>\n",
    /* optimizer-generated: direct stack stores */
    /* 107 PUTws1     */ "\010MOV <n>[BP],AX\n",
    /* 108 PUTbs1     */ "\010MOV BYTE PTR <n>[BP],AL\n",
    /* optimizer-generated: constant shift by 1 */
    /* 109 ASL1_1     */ "\001MOV AX,BX\nSHL AX,1\n",
    /* 110 ASR1_1     */ "\001MOV AX,BX\nSAR AX,1\n",
    /* 32-bit load/store */
    /* 111 GETd1m     */ "\020MOV AX,WORD PTR <m>\nMOV DX,WORD PTR <m>+2\n",
    /* 112 GETd1p     */ "\033MOV AX,[BX]\nMOV DX,2[BX]\n",       /* see gen() */
    /* 113 GETd1s     */ "\020MOV AX,<n>[BP]\nMOV DX,<n>+2[BP]\n",
    /* 114 GETdxn     */ "\020?MOV DX,<n>?XOR DX,DX?\n",
    /* 115 GETcxn     */ "\000?MOV CX,<n>?XOR CX,CX?\n",
    /* 116 GETd2m     */ "\003MOV BX,WORD PTR <m>\nMOV CX,WORD PTR <m>+2\n",
    /* 117 GETd2s     */ "\003MOV BX,<n>[BP]\nMOV CX,<n>+2[BP]\n",
    /* 118 PUTdm1     */ "\030MOV WORD PTR <m>,AX\nMOV WORD PTR <m>+2,DX\n",
    /* 119 PUTdp1     */ "\033PUSH BX\nMOV [BX],AX\nMOV 2[BX],DX\nPOP BX\n",
    /* 120 PUTds1     */ "\030MOV <n>[BP],AX\nMOV <n>+2[BP],DX\n",
    /* 32-bit stack */
    /* 121 PUSHd1     */ "\130PUSH DX\nPUSH AX\n",
    /* 122 POPd2      */ "\003POP BX\nPOP CX\n",
    /* 123 PUSHdm     */ "\100PUSH WORD PTR <m>+2\nPUSH WORD PTR <m>\n",
    /* 124 PUSHds     */ "\100PUSH <n>+2[BP]\nPUSH <n>[BP]\n",
    /* 32-bit move/swap */
    /* 125 MOVEd21    */ "\033MOV BX,AX\nMOV CX,DX\n",
    /* 126 SWAPd12    */ "\033XCHG AX,BX\nXCHG DX,CX\n",
    /* 32-bit widening */
    /* 127 WIDENs     */ "\030CWD\n",
    /* 128 WIDENu     */ "\020XOR DX,DX\n",
    /* 129 WIDENs2    */ "\003XOR CX,CX\nTEST BX,8000h\nJZ $+3\nDEC CX\n",
    /* 130 WIDENu2    */ "\001XOR CX,CX\n",
    /* 32-bit inline arithmetic */
    /* 131 ADDd12     */ "\033ADD AX,BX\nADC DX,CX\n",
    /* 132 SUBd12     */ "\033SUB AX,BX\nSBB DX,CX\n",              /* see gen() */
    /* 133 ANDd12     */ "\033AND AX,BX\nAND DX,CX\n",
    /* 134 ORd12      */ "\033OR AX,BX\nOR DX,CX\n",
    /* 135 XORd12     */ "\033XOR AX,BX\nXOR DX,CX\n",
    /* 136 COMd1      */ "\030NOT AX\nNOT DX\n",
    /* 137 ANEGd1     */ "\030NEG DX\nNEG AX\nSBB DX,0\n",
    /* 138 rINCd1     */ "\030ADD AX,<n>\nADC DX,0\n",
    /* 139 rDECd1     */ "\030SUB AX,<n>\nSBB DX,0\n",
    /* 140 DBLd1      */ "\030SHL AX,1\nRCL DX,1\n",
    /* 141 DBLd2      */ "\003SHL BX,1\nRCL CX,1\n",
    /* 32-bit library-call arithmetic */
    /* 142 MULd12     */ "\033CALL __lmul\n",
    /* 143 MULd12u    */ "\033CALL __ulmul\n",
    /* 144 DIVd12     */ "\033CALL __ldiv\n",                        /* see gen() */
    /* 145 DIVd12u    */ "\033CALL __uldiv\n",                       /* see gen() */
    /* 146 MODd12     */ "\033CALL __lmod\n",                        /* see gen() */
    /* 147 MODd12u    */ "\033CALL __ulmod\n",                       /* see gen() */
    /* 148 ASLd12     */ "\033CALL __lshl\n",
    /* 149 ASRd12     */ "\033CALL __lsar\n",
    /* 150 ASRd12u    */ "\033CALL __lshr\n",
    /* 32-bit library-call comparisons */
    /* 151 EQd12      */ "\033CALL __leq\n",
    /* 152 NEd12      */ "\033CALL __lne\n",
    /* 153 LTd12      */ "\033CALL __llt\n",
    /* 154 LEd12      */ "\033CALL __lle\n",
    /* 155 GTd12      */ "\033CALL __lgt\n",
    /* 156 GEd12      */ "\033CALL __lge\n",
    /* 157 LTd12u     */ "\033CALL __ullt\n",
    /* 158 LEd12u     */ "\033CALL __ulle\n",
    /* 159 GTd12u     */ "\033CALL __ulgt\n",
    /* 160 GEd12u     */ "\033CALL __ulge\n",
    /* 32-bit unary */
    /* 161 LNEGd1     */ "\030CALL __llneg\n",
    /* 32-bit truthiness */
    /* 162 EQd10f     */ "\030OR AX,DX\nJE $+5\nJMP _<n>\n",
    /* 163 NEd10f     */ "\030OR AX,DX\nJNE $+5\nJMP _<n>\n",
    /* 32-bit switch dispatch */
    /* 164 LSWITCHd   */ "\012CALL __lswitch\n",
    /* stack cleanup via POPs */
    /* 165 POPCX      */ "\000POP CX\n",
    /* 166 POPCX2     */ "\000POP CX\nPOP CX\n",
    /* short-branch variants: invert skip condition to jump directly */
    /* 167 EQ10fs     */ "\010OR AX,AX\nJNE _<n>\n",
    /* 168 NE10fs     */ "\010OR AX,AX\nJE _<n>\n",
    /* 169 GE10fs     */ "\010OR AX,AX\nJL _<n>\n",
    /* 170 GT10fs     */ "\010OR AX,AX\nJLE _<n>\n",
    /* 171 LE10fs     */ "\010OR AX,AX\nJG _<n>\n",
    /* 172 LT10fs     */ "\010OR AX,AX\nJGE _<n>\n",
    /* 173 EQd10fs    */ "\030OR AX,DX\nJNE _<n>\n",
    /* 174 NEd10fs    */ "\030OR AX,DX\nJE _<n>\n",
    /* 175 JMPs       */ "\000JMP SHORT _<n>\n",
    // factored comparison p-codes
    /* 176 NOP_       */ "\000",
    /* 177 CMP1n      */ "\010CMP AX,<n>\n",
    // factored conditional jumps (long: Jcc $+5 / JMP _n)
    /* 178 JccE       */ "\000JE $+5\nJMP _<n>\n",
    /* 179 JccNE      */ "\000JNE $+5\nJMP _<n>\n",
    /* 180 JccG       */ "\000JG $+5\nJMP _<n>\n",
    /* 181 JccL       */ "\000JL $+5\nJMP _<n>\n",
    /* 182 JccGE      */ "\000JGE $+5\nJMP _<n>\n",
    /* 183 JccLE      */ "\000JLE $+5\nJMP _<n>\n",
    /* 184 JccA       */ "\000JA $+5\nJMP _<n>\n",
    /* 185 JccB       */ "\000JB $+5\nJMP _<n>\n",
    /* 186 JccAE      */ "\000JAE $+5\nJMP _<n>\n",
    /* 187 JccBE      */ "\000JBE $+5\nJMP _<n>\n",
    // factored conditional jumps (short: inverted Jcc SHORT _n)
    /* 188 JccEs      */ "\000JNE _<n>\n",
    /* 189 JccNEs     */ "\000JE _<n>\n",
    /* 190 JccGs      */ "\000JLE _<n>\n",
    /* 191 JccLs      */ "\000JGE _<n>\n",
    /* 192 JccGEs     */ "\000JL _<n>\n",
    /* 193 JccLEs     */ "\000JG _<n>\n",
    /* 194 JccAs      */ "\000JBE _<n>\n",
    /* 195 JccBs      */ "\000JAE _<n>\n",
    /* 196 JccAEs     */ "\000JB _<n>\n",
    /* 197 JccBEs     */ "\000JA _<n>\n",
    // factored register comparison
    /* 198 CMP12      */ "\011CMP AX,BX\n",
    // factored memory/stack comparisons
    /* 199 CMP1m      */ "\010CMP AX,WORD PTR <m>\n",
    /* 200 CMP1s      */ "\010CMP AX,<n>[BP]\n"
};

//  === code generation functions =============================================

// Conservative machine-code byte count for each p-code.
// Used by flushfunc() to estimate branch distances.
// 255 = multi-part or data p-code: blocks optimization across it.
// Values are worst-case (largest encoding) to ensure safety.
int code_size[PCODES] = {
    /*  0 unused   */  0,
    /*  1 ADD12    */  2,  // ADD AX,BX
    /*  2 ADDSP    */  4,  // ADD SP,nn (worst case imm16)
    /*  3 AND12    */  2,  // AND AX,BX
    /*  4 ANEG1    */  2,  // NEG AX
    /*  5 (removed)*/  0,  // ARGCNTn: slot reserved; no longer emitted (rev 139)
    /*  6 ASL12    */  6,  // MOV CX,AX / MOV AX,BX / SAL AX,CL
    /*  7 ASR12    */  6,  // MOV CX,AX / MOV AX,BX / SAR AX,CL
    /*  8 CALL1    */  2,  // CALL AX
    /*  9 CALLm    */  3,  // CALL _name (near)
    /* 10 BYTE_    */255,  // data prefix
    /* 11 BYTEn    */255,  // data
    /* 12 BYTEr0   */255,  // data
    /* 13 COM1     */  2,  // NOT AX
    /* 14 DBL1     */  2,  // SHL AX,1
    /* 15 DBL2     */  2,  // SHL BX,1
    /* 16 DIV12    */  3,  // CWD / IDIV BX
    /* 17 DIV12u   */  4,  // XOR DX,DX / DIV BX
    /* 18 ENTER    */  3,  // PUSH BP / MOV BP,SP
    /* 19 EQ10f    */  7,  // OR AX,AX / JE $+5 / JMP near
    /* 20 EQ12     */  3,  // CALL __eq
    /* 21 GE10f    */  7,
    /* 22 GE12     */  3,
    /* 23 GE12u    */  3,
    /* 24 POINT1l  */  3,  // MOV AX,OFFSET _lit+n
    /* 25 POINT1m  */  3,  // MOV AX,OFFSET _name
    /* 26 GETb1m   */  4,  // MOV AL,[mem] / CBW
    /* 27 GETb1mu  */  5,  // MOV AL,[mem] / XOR AH,AH
    /* 28 GETb1p   */  5,  // MOV AL,[BX+disp16] / CBW
    /* 29 GETb1pu  */  5,  // MOV AL,[BX+disp16] / XOR AH,AH
    /* 30 GETw1m   */  3,  // MOV AX,[mem16]
    /* 31 GETw1n   */  3,  // MOV AX,n or XOR AX,AX
    /* 32 GETw1p   */  4,  // MOV AX,[BX+disp16]
    /* 33 GETw2n   */  3,  // MOV BX,n or XOR BX,BX
    /* 34 GT10f    */  7,
    /* 35 GT12     */  3,
    /* 36 GT12u    */  3,
    /* 37 WORD_    */255,  // data prefix
    /* 38 WORDn    */255,  // data
    /* 39 WORDr0   */255,  // data
    /* 40 JMPm     */  3,  // JMP near
    /* 41 LABm     */  0,  // label definition
    /* 42 LE10f    */  7,
    /* 43 LE12     */  3,
    /* 44 LE12u    */  3,
    /* 45 LNEG1    */  3,  // CALL __lneg
    /* 46 LT10f    */  7,
    /* 47 LT12     */  3,
    /* 48 LT12u    */  3,
    /* 49 MOD12    */  5,  // CWD / IDIV BX / MOV AX,DX
    /* 50 MOD12u   */  6,  // XOR DX,DX / DIV BX / MOV AX,DX
    /* 51 MOVE21   */  2,  // MOV BX,AX
    /* 52 MUL12    */  2,  // IMUL BX
    /* 53 MUL12u   */  2,  // MUL BX
    /* 54 NE10f    */  7,
    /* 55 NE12     */  3,
    /* 56 NEARm    */255,  // data DW
    /* 57 OR12     */  2,  // OR AX,BX
    /* 58 POINT1s  */  4,  // LEA AX,disp[BP]
    /* 59 POP2     */  1,  // POP BX
    /* 60 PUSH1    */  1,  // PUSH AX
    /* 61 PUTbm1   */  3,  // MOV [mem],AL
    /* 62 PUTbp1   */  2,  // MOV [BX],AL
    /* 63 PUTwm1   */  3,  // MOV [mem],AX
    /* 64 PUTwp1   */  2,  // MOV [BX],AX
    /* 65 rDEC1    */  2,  // DEC AX repeated (max 2)
    /* 66 REFm     */255,  // partial label reference
    /* 67 RETURN   */  4,  // MOV SP,BP / POP BP / RET
    /* 68 rINC1    */  2,  // INC AX repeated (max 2)
    /* 69 SUB12    */  2,  // SUB AX,BX
    /* 70 SWAP12   */  1,  // XCHG AX,BX
    /* 71 SWAP1s   */  3,  // POP BX / XCHG AX,BX / PUSH BX
    /* 72 SWITCH   */  3,  // CALL __switch
    /* 73 XOR12    */  2,  // XOR AX,BX
    /* 74 ADD1n    */  3,  // ADD AX,n or nothing
    /* 75 ADD21    */  2,  // ADD BX,AX
    /* 76 ADD2n    */  3,  // ADD BX,n
    /* 77 ADDbpn   */  3,  // ADD BYTE PTR [BX],n
    /* 78 ADDwpn   */  4,  // ADD WORD PTR [BX],n (imm16)
    /* 79 ADDm_    */255,  // partial
    /* 80 COMMAn   */255,  // partial
    /* 81 DECbp    */  2,  // DEC BYTE PTR [BX]
    /* 82 DECwp    */  2,  // DEC WORD PTR [BX]
    /* 83 POINT2m  */  3,  // MOV BX,OFFSET _name
    /* 84 POINT2m_ */255,  // partial
    /* 85 GETb1s   */  5,  // MOV AL,disp[BP] / CBW
    /* 86 GETb1su  */  5,  // MOV AL,disp[BP] / XOR AH,AH
    /* 87 GETw1m_  */255,  // partial
    /* 88 GETw1s   */  4,  // MOV AX,disp[BP]
    /* 89 GETw2m   */  4,  // MOV BX,[mem16]
    /* 90 GETw2p   */  4,  // MOV BX,[BX+disp16]
    /* 91 GETw2s   */  4,  // MOV BX,disp[BP]
    /* 92 INCbp    */  2,  // INC BYTE PTR [BX]
    /* 93 INCwp    */  2,  // INC WORD PTR [BX]
    /* 94 PLUSn    */255,  // partial
    /* 95 POINT2s  */  4,  // LEA BX,disp[BP]
    /* 96 PUSH2    */  1,  // PUSH BX
    /* 97 PUSHm    */  4,  // PUSH WORD PTR [mem16]
    /* 98 PUSHp    */  4,  // PUSH [BX+disp16]
    /* 99 PUSHs    */  4,  // PUSH disp[BP]
    /*100 PUT_m_   */255,  // partial
    /*101 rDEC2    */  2,  // DEC BX repeated (max 2)
    /*102 rINC2    */  2,  // INC BX repeated (max 2)
    /*103 SUB_m_   */255,  // partial
    /*104 SUB1n    */  3,  // SUB AX,n
    /*105 SUBbpn   */  3,  // SUB BYTE PTR [BX],n
    /*106 SUBwpn   */  4,  // SUB WORD PTR [BX],n
    /*107 PUTws1   */  4,  // MOV disp[BP],AX
    /*108 PUTbs1   */  4,  // MOV BYTE PTR disp[BP],AL
    /*109 ASL1_1   */  4,  // MOV AX,BX / SHL AX,1
    /*110 ASR1_1   */  4,  // MOV AX,BX / SAR AX,1
    /*111 GETd1m   */  7,  // MOV AX,[mem] / MOV DX,[mem+2]
    /*112 GETd1p   */  5,  // MOV AX,[BX] / MOV DX,2[BX]
    /*113 GETd1s   */  8,  // MOV AX,n[BP] / MOV DX,n+2[BP]
    /*114 GETdxn   */  3,  // MOV DX,n or XOR DX,DX
    /*115 GETcxn   */  3,  // MOV CX,n or XOR CX,CX
    /*116 GETd2m   */  8,  // MOV BX,[mem] / MOV CX,[mem+2]
    /*117 GETd2s   */  8,  // MOV BX,n[BP] / MOV CX,n+2[BP]
    /*118 PUTdm1   */  7,  // MOV [mem],AX / MOV [mem+2],DX
    /*119 PUTdp1   */  7,  // PUSH BX / MOV [BX],AX / MOV 2[BX],DX / POP BX
    /*120 PUTds1   */  8,  // MOV n[BP],AX / MOV n+2[BP],DX
    /*121 PUSHd1   */  2,  // PUSH DX / PUSH AX
    /*122 POPd2    */  2,  // POP BX / POP CX
    /*123 PUSHdm   */  8,  // PUSH [mem+2] / PUSH [mem]
    /*124 PUSHds   */  8,  // PUSH n+2[BP] / PUSH n[BP]
    /*125 MOVEd21  */  4,  // MOV BX,AX / MOV CX,DX
    /*126 SWAPd12  */  3,  // XCHG AX,BX / XCHG DX,CX
    /*127 WIDENs   */  1,  // CWD
    /*128 WIDENu   */  2,  // XOR DX,DX
    /*129 WIDENs2  */  9,  // XOR CX,CX / TEST BX,8000h / JZ $+3 / DEC CX
    /*130 WIDENu2  */  2,  // XOR CX,CX
    /*131 ADDd12   */  4,  // ADD AX,BX / ADC DX,CX
    /*132 SUBd12   */  4,  // SUB AX,BX / SBB DX,CX
    /*133 ANDd12   */  4,  // AND AX,BX / AND DX,CX
    /*134 ORd12    */  4,  // OR AX,BX / OR DX,CX
    /*135 XORd12   */  4,  // XOR AX,BX / XOR DX,CX
    /*136 COMd1    */  4,  // NOT AX / NOT DX
    /*137 ANEGd1   */  7,  // NEG DX / NEG AX / SBB DX,0
    /*138 rINCd1   */  6,  // ADD AX,n / ADC DX,0
    /*139 rDECd1   */  6,  // SUB AX,n / SBB DX,0
    /*140 DBLd1    */  4,  // SHL AX,1 / RCL DX,1
    /*141 DBLd2    */  4,  // SHL BX,1 / RCL CX,1
    /*142 MULd12   */  3,
    /*143 MULd12u  */  3,
    /*144 DIVd12   */  3,
    /*145 DIVd12u  */  3,
    /*146 MODd12   */  3,
    /*147 MODd12u  */  3,
    /*148 ASLd12   */  3,
    /*149 ASRd12   */  3,
    /*150 ASRd12u  */  3,
    /*151 EQd12    */  3,
    /*152 NEd12    */  3,
    /*153 LTd12    */  3,
    /*154 LEd12    */  3,
    /*155 GTd12    */  3,
    /*156 GEd12    */  3,
    /*157 LTd12u   */  3,
    /*158 LEd12u   */  3,
    /*159 GTd12u   */  3,
    /*160 GEd12u   */  3,
    /*161 LNEGd1   */  3,
    /*162 EQd10f   */  7,  // OR AX,DX / JE $+5 / JMP near
    /*163 NEd10f   */  7,
    /*164 LSWITCHd */  3,
    /*165 POPCX    */  1,  // POP CX
    /*166 POPCX2   */  2,  // POP CX / POP CX
    /*167 EQ10fs   */  4,  // OR AX,AX / JNE _n  (short)
    /*168 NE10fs   */  4,
    /*169 GE10fs   */  4,
    /*170 GT10fs   */  4,
    /*171 LE10fs   */  4,
    /*172 LT10fs   */  4,
    /*173 EQd10fs  */  4,  // OR AX,DX / JNE _n  (short)
    /*174 NEd10fs  */  4,
    /*175 JMPs     */  2,  // JMP SHORT _n
    // factored comparison p-codes
    /*176 NOP_     */  0,  // no output
    /*177 CMP1n    */  3,  // CMP AX,imm16
    // factored conditional jumps (long)
    /*178 JccE     */  5,  // JE  $+5 / JMP near
    /*179 JccNE    */  5,  // JNE $+5 / JMP near
    /*180 JccG     */  5,  // JG  $+5 / JMP near
    /*181 JccL     */  5,  // JL  $+5 / JMP near
    /*182 JccGE    */  5,  // JGE $+5 / JMP near
    /*183 JccLE    */  5,  // JLE $+5 / JMP near
    /*184 JccA     */  5,  // JA  $+5 / JMP near
    /*185 JccB     */  5,  // JB  $+5 / JMP near
    /*186 JccAE    */  5,  // JAE $+5 / JMP near
    /*187 JccBE    */  5,  // JBE $+5 / JMP near
    // factored conditional jumps (short)
    /*188 JccEs    */  2,  // JNE SHORT _n
    /*189 JccNEs   */  2,  // JE  SHORT _n
    /*190 JccGs    */  2,  // JLE SHORT _n
    /*191 JccLs    */  2,  // JGE SHORT _n
    /*192 JccGEs   */  2,  // JL  SHORT _n
    /*193 JccLEs   */  2,  // JG  SHORT _n
    /*194 JccAs    */  2,  // JBE SHORT _n
    /*195 JccBs    */  2,  // JAE SHORT _n
    /*196 JccAEs   */  2,  // JB  SHORT _n
    /*197 JccBEs   */  2,  // JA  SHORT _n
    // factored register comparison
    /*198 CMP12    */  2,  // CMP AX,BX
    // factored memory/stack comparisons
    /*199 CMP1m    */  4,  // CMP AX,WORD PTR [mem16]
    /*200 CMP1s    */  4   // CMP AX,n[BP]
};

// Map a long-branch p-code to its short-branch equivalent, or 0 if not a branch.
int shortbranch(int pc) {
    switch (pc) {
    case EQ10f:  return EQ10fs;
    case NE10f:  return NE10fs;
    case GE10f:  return GE10fs;
    case GT10f:  return GT10fs;
    case LE10f:  return LE10fs;
    case LT10f:  return LT10fs;
    // EQf..GEuf retired (p-codes removed): now using CMP12 + JccXX
    case EQd10f: return EQd10fs;
    case NEd10f: return NEd10fs;
    case JccE:   return JccEs;
    case JccNE:  return JccNEs;
    case JccG:   return JccGs;
    case JccL:   return JccLs;
    case JccGE:  return JccGEs;
    case JccLE:  return JccLEs;
    case JccA:   return JccAs;
    case JccB:   return JccBs;
    case JccAE:  return JccAEs;
    case JccBE:  return JccBEs;
    case JMPm:   return JMPs;
    default:     return 0;
    }
}

// print all assembler info before any code is generated
// and ensure that segments appear in correct order.
void header() {
    needLong = 0;
    funcbuf = calloc(FUNCBUFSIZE, BPW);
    fl_labnum = calloc(256, BPW);
    fl_labpos = calloc(256, BPW);
    fnext = funcbuf;
    toseg(CODESEG);
    outlines("extrn __eq: near\n"
             "extrn __ne: near\n"
             "extrn __le: near\n"
             "extrn __lt: near\n"
             "extrn __ge: near\n"
             "extrn __gt: near\n"
             "extrn __ule: near\n"
             "extrn __ult: near\n"
             "extrn __uge: near\n"
             "extrn __ugt: near\n"
             "extrn __lneg: near\n"
             "extrn __switch: near");
    outline("dw 0"); // force non-zero code pointers, word alignment
    toseg(DATASEG);
    outline("dw 0"); // force non-zero data pointers, word alignment
}

// print assembler stuff needed at the end
void trailer() {
    char *cp;
    cptr = STARTGLB;
    while (cptr < ENDGLB) {
        if ((cptr[IDENT] == IDENT_FUNCTION || cptr[IDENT] == IDENT_PTR_FUNCTION) && cptr[CLASS] == AUTOEXT)
            external(cptr + NAME, 0, IDENT_FUNCTION);
        cptr += SYMMAX;
    }
    if ((cp = findglb("main")) && cp[CLASS] == GLOBAL)
        external("_main", 0, IDENT_FUNCTION);
    if (needLong) {
        toseg(CODESEG);
        outlines("extrn __lmul: near\n"
                 "extrn __ulmul: near\n"
                 "extrn __ldiv: near\n"
                 "extrn __uldiv: near\n"
                 "extrn __lmod: near\n"
                 "extrn __ulmod: near\n"
                 "extrn __lshl: near\n"
                 "extrn __lsar: near\n"
                 "extrn __lshr: near\n"
                 "extrn __leq: near\n"
                 "extrn __lne: near\n"
                 "extrn __llt: near\n"
                 "extrn __lle: near\n"
                 "extrn __lgt: near\n"
                 "extrn __lge: near\n"
                 "extrn __ullt: near\n"
                 "extrn __ulle: near\n"
                 "extrn __ulgt: near\n"
                 "extrn __ulge: near\n"
                 "extrn __llneg: near\n"
                 "extrn __lswitch: near");
    }
    toseg(NULL);
    outline("END");
#ifdef DISOPT
    {
        int i;
        outstr(";opt   count"); newline();
        for (i = 0; i <= HIGH_SEQ; i++) {
            fprintf(output, "; %d   %d\n", i, optcount[i]);
            poll(YES);
        }
    }
#endif 
}

// remember where we are in the queue in case we have to back up.
void setstage(int *before, int *start) {
    if ((*before = snext) == 0)
        snext = stage;
    *start = snext;
}

// generate code in staging buffer.
void gen(int pcode, int value) {
    int newcsp;
    switch (pcode) {
        case GETb1pu:
        case GETb1p:
        case GETw1p:
            gen(MOVE21, 0); 
            break;
        case GETd1p:
            gen(MOVEd21, 0);
            break;
        case SUB12:
        case MOD12:
        case MOD12u:
        case DIV12:
        case DIV12u:
            gen(SWAP12, 0);
            break;
        case SUBd12:
        case MODd12:
        case MODd12u:
        case DIVd12:
        case DIVd12u:
            gen(SWAPd12, 0);
            needLong = 1;
            break;
        case PUSH1:
        case PUSH2:
            csp -= BPW;
            break;
        case PUSHd1:
        case PUSHdm:
        case PUSHds:
            csp -= BPD;
            break;
        case POP2:
            csp += BPW;
            break;
        case POPd2:
            csp += BPD;
            break;
        case ADDSP:
        case RETURN:
            newcsp = value;
            value -= csp;
            csp = newcsp;
            if (pcode == ADDSP) {
                if      (value == BPW) pcode = POPCX;
                else if (value == BPD) pcode = POPCX2;
            }
            break;
        case MULd12:  case MULd12u:
        case ASLd12:  case ASRd12:  case ASRd12u:
        case EQd12:   case NEd12:
        case LTd12:   case LEd12:   case GTd12:   case GEd12:
        case LTd12u:  case LEd12u:  case GTd12u:  case GEd12u:
        case LNEGd1:
        case LSWITCHd:
            needLong = 1;
    }
    if (snext == 0) {
        bufout(pcode, value);
        return;
    }
    if (snext >= slast) {
        error("staging buffer overflow");
        return;
    }
    snext[0] = pcode;
    snext[1] = value;
    snext += 2;
}

// ClearStage() either writes the contents of the buffer to the output file and
// resets snext to zero or merely backs up snext to an earlier point in the
// buffer, thereby discarding the most recently generated code. ClearStage()
// calls dumpstage which calls outcode() to translate the p-codes to ASCII
// strings and write them to the output file. Outcode() in turn calls peep()
// to optimize the p-codes before it translates and writes them.
// If start == 0, throw away contents.
// If before != 0, don't dump queue yet.
void ClearStage(int *before, int *start) {
    if (before) {
        snext = before;
        return;
    }
    if (start)
        dumpstage();
    snext = 0;
}

// Route a p-code into funcbuf when in CODE segment, else emit directly.
// Handles overflow by flushing without optimization.
void bufout(int pcode, int value) {
    int used;
    used = fnext - funcbuf;
    if (oldseg != CODESEG) {
        outcode(pcode, value);
        return;
    }
    if (fnext >= funcbuf + FUNCBUFSIZE) {
        // overflow: emit buffer and new entry directly, skip optimization
        int *p;
        p = funcbuf;
        while (p < fnext) { outcode(p[0], p[1]); p += 2; }
        fnext = funcbuf;
        outcode(pcode, value);
        return;
    }
    fnext[0] = pcode;
    fnext[1] = value;
    fnext += 2;
}

// Two-pass short-branch optimizer over funcbuf.
// Pass 1: record label byte positions.
// Pass 2: substitute short-branch p-codes where target is within +-127 bytes.
// Pass 3: emit via outcode().
void flushfunc() {
    int *p, pc, val, spc, pos, short_end, delta, changed;
    int i, lpos, nflab;
    int pairs;
    pairs = (fnext - funcbuf) / 2;
    if (fnext == funcbuf) {
        return;
    }
    // iterate: each pass may shrink branches, bringing more targets within range
    do {
        // pass 1: record label byte positions using current p-codes
        nflab = 0;
        pos = 0;
        p = funcbuf;
        while (p < fnext) {
            pc = p[0];
            if (pc == LABm && nflab < 256) {
                fl_labnum[nflab] = p[1];
                fl_labpos[nflab] = pos;
                ++nflab;
            }
            pos += code_size[pc];
            p += 2;
        }
        // pass 2: substitute reachable long branches with short variants
        changed = 0;
        pos = 0;
        p = funcbuf;
        while (p < fnext) {
            pc = p[0];
            spc = shortbranch(pc);
            if (spc) {
                val = p[1];
                lpos = -1;
                for (i = 0; i < nflab; ++i) {
                    if (fl_labnum[i] == val) { lpos = fl_labpos[i]; break; }
                }
                if (lpos >= 0) {
                    short_end = pos + code_size[spc];
                    delta = lpos - short_end;
                    if (delta >= -128 && delta <= 127) {
                        if (p[0] != spc) { p[0] = spc; changed = 1; }
                    }
                }
            }
            pos += code_size[pc];
            p += 2;
        }
    } while (changed);
    // pass 3: emit
    p = funcbuf;
    while (p < fnext) { outcode(p[0], p[1]); p += 2; }
    fnext = funcbuf;
}

// dump the staging buffer
void dumpstage() {
    int *seq, rulenum;
    stail = snext;
    snext = stage;
    while (snext < stail) {
        if (optimize) {
        restart:
            seq = seqdata;
            rulenum = 0;
            while (*seq) {
                if (peep(seq)) {
#ifdef DISOPT
                    optcount[rulenum]++;
                    fprintf(output, ";                  optimized %d\n", rulenum);
#endif
                    goto restart;
                }
                // advance past this rule's double-zero terminator
                while (!(*seq == 0 && *(seq + 1) == 0))
                    seq++;
                seq += 2;
                rulenum++;
            }
        }
        bufout(snext[0], snext[1]);
        snext += 2;
    }
}

// change to a new segment
// may be called with NULL, CODESEG, or DATASEG
void toseg(int newseg) {
    if (oldseg == newseg) {
        // function-to-function boundary: flush previous function's body
        // before the next function's labels are emitted directly to file
        if (oldseg == CODESEG && funcbuf && fnext > funcbuf)
            flushfunc();
        return;
    }
    if (oldseg == CODESEG) {
        flushfunc();
        outline("CODE ENDS");
    }
    else if (oldseg == DATASEG)
        outline("DATA ENDS");
    if (newseg == CODESEG) {
        outline("CODE SEGMENT PUBLIC");
        outline("ASSUME CS:CODE, SS:DATA, DS:DATA");
    }
    else if (newseg == DATASEG)
        outline("DATA SEGMENT PUBLIC");
    oldseg = newseg;
}

// declare variable, allowing global scope
void decGlobal(int ident, int isGlobal) {
    if (ident == IDENT_FUNCTION || ident == IDENT_PTR_FUNCTION)
        toseg(CODESEG);
    else
        toseg(DATASEG);
    if (isGlobal) {
        outstr("PUBLIC ");
        outname(ssname);
        newline();
    }
    outname(ssname);
    if (ident == IDENT_FUNCTION || ident == IDENT_PTR_FUNCTION) {
        colon();
        newline();
    }
}

// declare external reference
void external(char *name, int size, int ident) {
    if (ident == IDENT_FUNCTION || ident == IDENT_PTR_FUNCTION)
        toseg(CODESEG);
    else
        toseg(DATASEG);
    outstr("EXTRN ");
    outname(name);
    colon();
    outsize(size, ident);
    newline();
}

// output the size of the object pointed to.
void outsize(int size, int ident) {
    if (size == 1 && ident != IDENT_POINTER && ident != IDENT_PTR_ARRAY
                  && ident != IDENT_FUNCTION && ident != IDENT_PTR_FUNCTION)
        outstr("BYTE");
    else if (size == BPD && ident != IDENT_POINTER && ident != IDENT_PTR_ARRAY
                         && ident != IDENT_FUNCTION && ident != IDENT_PTR_FUNCTION)
        outstr("DWORD");
    else if (ident != IDENT_FUNCTION && ident != IDENT_PTR_FUNCTION)
        outstr("WORD");
    else
        outstr("NEAR");
}

// point to following object(s)
void point() {
    outline(" DW $+2");
}

// dump the literal pool
void dumplits(int size) {
    int j, k;
    k = 0;
    while (k < litptr) {
        poll(1);                     // allow program interruption
        if (size == 1)
            gen(BYTE_, NULL);
        else
            gen(WORD_, NULL);
        j = 10;
        while (j--) {
            if (size == BPD) {
                // emit 32-bit value as two 16-bit words: lo, hi
                outdec(getint(litq + k, BPW));
                fputc(',', output);
                outdec(getint(litq + k + BPW, BPW));
                k += BPD;
                j--;   // used an extra column for hi word
            }
            else {
                outdec(getint(litq + k, size));
                k += size;
            }
            if (j <= 0 || k >= litptr) {
                newline();
                break;
            }
            fputc(',', output);
        }
    }
}

// dump zeroes for default initial values
void dumpzero(int size, int count) {
    if (count > 0) {
        if (size == 1)
            gen(BYTEr0, count);
        else if (size == BPD)
            gen(WORDr0, count * 2);  // two zero words per long
        else
            gen(WORDr0, count);
    }
}

// === optimizer functions ====================================================

// Try to optimize sequence at snext in the staging buffer.
int peep(int *seq) {
    int *next, *pop, n, skip, tmp, reply;
    char c, *cp;
    next = snext;
    while (*seq) {
        switch (*seq) {
        case any:
            if (next < stail)
                break;
            return (NO);
        case pfree:
            if (isfree(PRI, next))
                break;
            return (NO);
        case sfree:
            if (isfree(SEC, next))
                break;
            return (NO);
        case comm: 
            if (*(cp = code[*next]) & COMMUTES)
                break;
            return (NO);
        case _pop:
            if (pop = getpop(next))
                break;
            return (NO);
        default:
            if (next >= stail || *next != *seq)
                return (NO);
        }
        next += 2; ++seq;
    }

    // === have a match, now optimize it ======================================

    reply = skip = NO;
    while (*(++seq) || skip) {
        if (skip) {
            if (*seq == 0)
                skip = NO;
            continue;
        }
        if (*seq >= PCODES) {
            c = *seq & 0xFF;            // get low byte of command
            n = c;                      // and sign extend into n
            switch (*seq & 0xFF00) {
            case ife:
                if (snext[1] != n) {
                    skip = YES;
                }
                break;
            case ifl:
                if (snext[1] >= n) {
                    skip = YES;
                }
                break;
            case go:
                snext += (n << 1);      
                break;
            case gc:
                snext[0] = snext[(n << 1)];
                goto done;
            case gv:
                snext[1] = snext[(n << 1) + 1];
                goto done;
            case sum:
                snext[1] += snext[(n << 1) + 1];
                goto done;
            case neg:
                snext[1] = -snext[1];
                goto done;
            case topop:
                pop[0] = n;
                pop[1] = snext[1];
                goto done;
            case swv:
                tmp = snext[1];
                snext[1] = snext[(n << 1) + 1];
                snext[(n << 1) + 1] = tmp;
            done:
                reply = YES;
                break;
            }
        }
        else { snext[0] = *seq; reply = YES; }  // set p-code
    }
    return (reply);
}

// Is the primary or secondary register free?
// Is it zapped or unused by the p-code at pp
// or a successor?  If the primary register is
// unused by it still may not be free if the
// context uses the value of the expression.
int isfree(int reg, int *pp) {
    char *cp;
    while (pp < stail) {
        cp = code[*pp];
        if (*cp & USES & reg) return (NO);
        if (*cp & ZAPS & reg) return (YES);
        pp += 2;
    }
    if (usexpr) return (reg & 001);   // PRI => NO, SEC => YES at end
    else       return (YES);
}

// Get place where the currently pushed value is popped?
// NOTE: Function arguments are not popped, they are
// wasted with an ADDSP, POPCX, or POPCX2.
int getpop(int *next) {
    char *cp;
    int level;  level = 0;
    while (YES) {
        if (next >= stail)                     // compiler error
            return 0;
        if (*next == POP2)
            if (level) --level;
            else return next;                   // have a matching POP2
        else if (*next == ADDSP || *next == POPCX || *next == POPCX2) { // after func call
            if ((level -= (next[1] >> LBPW)) < 0)
                return 0;
        }
        else {
            cp = code[*next];                   // code string ptr
            if (*cp & PUSHES) ++level;           // must be a push
        }
        next += 2;
    }
}

// === output functions =======================================================

void colon() {
    fputc(':', output);
}

void newline() {
    fputc(NEWLINE, output);
}

// output assembly code.
void outcode(int pcode, int value) {
    int part, skip, count;
    char *cp, *back;
    part = back = 0;
    skip = NO;
    cp = code[pcode] + 1;          // skip 1st byte of code string
    while (*cp) {
        if (*cp == '<') {
            ++cp;                      // skip to action code
            if (skip == NO) switch (*cp) {
            case 'm': outname(value + NAME); break; // mem ref by label
            case 'n': outdec(value);       break; // numeric constant
            case 'l': outdec(litlab);      break; // current literal label
            }
            cp += 2;                   // skip past >
        }
        else if (*cp == '?') {        // ?..if value...?...if not value...?
            switch (++part) {
            case 1: if (value == 0) skip = YES; break;
            case 2: skip = !skip;              break;
            case 3: part = 0; skip = NO;       break;
            }
            ++cp;                      // skip past ?
        }
        else if (*cp == '#') {        // repeat #...# value times
            ++cp;
            if (back == 0) {
                if ((count = value) < 1) {
                    while (*cp && *cp++ != '#');
                    continue;
                }
                back = cp;
                continue;
            }
            if (--count > 0) cp = back;
            else back = 0;
        }
        else if (skip == NO) fputc(*cp++, output);
        else ++cp;
    }
}

void outdec(int number) {
    int k, zs;
    char c, *q, *r;
    zs = 0;
    k = 10000;
    if (number < 0) {
        number = -number;
        fputc('-', output);
    }
    while (k >= 1) {
        q = 0;
        r = number;
        while (r >= k) { ++q; r = r - k; }
        c = q + '0';
        if (c != '0' || k == 1 || zs) {
            zs = 1;
            fputc(c, output);
        }
        number = r;
        k /= 10;
    }
}

void outline(char ptr[]) {
    outstr(ptr);
    newline();
}

// output multiple lines from a single string; segments separated by '\n'
void outlines(char *s) {
    while (*s) {
        outstr(s);
        while (*s >= ' ') s++;
        newline();
        if (*s == '\n') s++;
    }
}

void outname(char ptr[]) {
    outstr("_");
    while (*ptr >= ' ') fputc(*ptr++, output);
}

void outstr(char ptr[]) {
    poll(1);           // allow program interruption
    while (*ptr >= ' ') fputc(*ptr++, output);
}
