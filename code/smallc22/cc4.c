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

#define HIGH_SEQ  73
#define SEQLEN    13   // max elements in any sequence

int seqdata[HIGH_SEQ + 1][SEQLEN] = {
    { 0,ADD12,MOVE21,0,                      go|p1,ADD21,0 },                             // 00: ADD21
    { 0,ADD1n,0,                             ifl|m2,0,ifl|0,rDEC1,neg,0,ifl|p3,rINC1,0,0 }, // 01: rINC1 or rDEC1 ?
    { 0,ADD2n,0,                             ifl|m2,0,ifl|0,rDEC2,neg,0,ifl|p3,rINC2,0,0 }, // 02: rINC2 or rDEC2 ?
    { 0,rDEC1,PUTbp1,rINC1,0,               go|p2,ife|p1,DECbp,0,SUBbpn,0 },            // 03: SUBbpn or DECbp
    { 0,rDEC1,PUTwp1,rINC1,0,               go|p2,ife|p1,DECwp,0,SUBwpn,0 },            // 04: SUBwpn or DECwp
    { 0,rDEC1,PUTbm1,rINC1,0,               go|p1,SUB_m_,go|p1,COMMAn,go|m1,0 },       // 05: SUB_m_ COMMAn
    { 0,rDEC1,PUTwm1,rINC1,0,               go|p1,SUB_m_,go|p1,COMMAn,go|m1,0 },       // 06: SUB_m_ COMMAn
    { 0,GETw1m,GETw2n,ADD12,MOVE21,GETb1p,0,  go|p4,gv|m3,go|m1,GETw2m,gv|m3,0 },     // 07: GETw2m GETb1p
    { 0,GETw1m,GETw2n,ADD12,MOVE21,GETb1pu,0, go|p4,gv|m3,go|m1,GETw2m,gv|m3,0 },     // 08: GETw2m GETb1pu
    { 0,GETw1m,GETw2n,ADD12,MOVE21,GETw1p,0,  go|p4,gv|m3,go|m1,GETw2m,gv|m3,0 },     // 09: GETw2m GETw1p
    { 0,GETw1m,GETw2m,SWAP12,0,             go|p2,GETw1m,gv|m1,go|m1,gv|m1,0 },        // 10: GETw2m GETw1m
    { 0,GETw1m,MOVE21,0,                    go|p1,GETw2m,gv|m1,0 },                     // 11: GETw2m
    { 0,GETw1m,PUSH1,pfree,0,              go|p1,PUSHm,gv|m1,0 },                      // 12: PUSHm
    { 0,GETw1n,PUTbm1,pfree,0,             PUT_m_,go|p1,COMMAn,go|m1,swv|p1,0 },       // 13: PUT_m_ COMMAn
    { 0,GETw1n,PUTwm1,pfree,0,             PUT_m_,go|p1,COMMAn,go|m1,swv|p1,0 },       // 14: PUT_m_ COMMAn
    { 0,GETw1p,PUSH1,pfree,0,              go|p1,PUSHp,gv|m1,0 },                      // 15: PUSHp
    { 0,GETw1s,GETw2n,ADD12,MOVE21,0,      go|p3,ADD2n,gv|m2,go|m1,GETw2s,gv|m2,0 },  // 16: GETw2s ADD2n
    { 0,GETw1s,GETw2s,SWAP12,0,            go|p2,GETw1s,gv|m1,go|m1,GETw2s,gv|m1,0 }, // 17: GETw2s GETw1s
    { 0,GETw1s,MOVE21,0,                   go|p1,GETw2s,gv|m1,0 },                     // 18: GETw2s
    { 0,GETw2m,GETw1n,SWAP12,SUB12,0,      go|p3,SUB1n,gv|m2,go|m1,GETw1m,gv|m2,0 },  // 19: GETw1m SUB1n
    { 0,GETw2n,ADD12,0,                    go|p1,ADD1n,gv|m1,0 },                      // 20: ADD1n
    { 0,GETw2s,GETw1n,SWAP12,SUB12,0,      go|p3,SUB1n,gv|m2,go|m1,GETw1s,gv|m2,0 },  // 21: GETw1s SUB1n
    { 0,rINC1,PUTbm1,rDEC1,0,              go|p1,ADDm_,go|p1,COMMAn,go|m1,0 },         // 22: ADDm_ COMMAn
    { 0,rINC1,PUTwm1,rDEC1,0,              go|p1,ADDm_,go|p1,COMMAn,go|m1,0 },         // 23: ADDm_ COMMAn
    { 0,rINC1,PUTbp1,rDEC1,0,              go|p2,ife|p1,INCbp,0,ADDbpn,0 },            // 24: ADDbpn or INCbp
    { 0,rINC1,PUTwp1,rDEC1,0,              go|p2,ife|p1,INCwp,0,ADDwpn,0 },            // 25: ADDwpn or INCwp
    { 0,MOVE21,GETw1n,SWAP12,SUB12,0,      go|p3,SUB1n,gv|m2,0 },                      // 26: SUB1n
    { 0,MOVE21,GETw1n,comm,0,              go|p1,GETw2n,0 },                            // 27: GETw2n comm
    { 0,POINT1m,GETw2n,ADD12,MOVE21,0,     go|p3,PLUSn,gv|m2,go|m1,POINT2m_,gv|m2,0 }, // 28: POINT2m_ PLUSn
    { 0,POINT1m,MOVE21,pfree,0,            go|p1,POINT2m,gv|m1,0 },                    // 29: POINT2m
    { 0,POINT1m,PUSH1,pfree,_pop,0,        topop|POINT2m,go|p2,0 },                     // 30: ... POINT2m
    { 0,POINT1s,GETw2n,ADD12,MOVE21,0,     sum|p1,go|p3,POINT2s,gv|m3,0 },             // 31: POINT2s
    { 0,POINT1s,PUSH1,MOVE21,0,            go|p1,POINT2s,gv|m1,go|p1,PUSH2,go|m1,0 },  // 32: POINT2s PUSH2
    { 0,POINT1s,PUSH1,pfree,_pop,0,        topop|POINT2s,go|p2,0 },                     // 33: ... POINT2s
    { 0,POINT1s,MOVE21,0,                  go|p1,POINT2s,gv|m1,0 },                     // 34: POINT2s
    { 0,POINT2m,GETb1p,sfree,0,            go|p1,GETb1m,gv|m1,0 },                     // 35: GETb1m
    { 0,POINT2m,GETb1pu,sfree,0,           go|p1,GETb1mu,gv|m1,0 },                    // 36: GETb1mu
    { 0,POINT2m,GETw1p,sfree,0,            go|p1,GETw1m,gv|m1,0 },                     // 37: GETw1m
    { 0,POINT2m_,PLUSn,GETw1p,sfree,0,     go|p2,gc|m1,gv|m1,go|m1,GETw1m_,gv|m1,0 }, // 38: GETw1m_ PLUSn
    { 0,POINT2s,GETb1p,sfree,0,            sum|p1,go|p1,GETb1s,gv|m1,0 },              // 39: GETb1s
    { 0,POINT2s,GETb1pu,sfree,0,           sum|p1,go|p1,GETb1su,gv|m1,0 },             // 40: GETb1su
    { 0,POINT2s,GETw1p,PUSH1,pfree,0,      sum|p1,go|p2,PUSHs,gv|m2,0 },               // 41: PUSHs
    { 0,POINT2s,GETw1p,sfree,0,            sum|p1,go|p1,GETw1s,gv|m1,0 },              // 42: GETw1s
    { 0,PUSH1,any,POP2,0,                  go|p2,gc|m1,gv|m1,go|m1,MOVE21,0 },         // 43: MOVE21 any
    { 0,PUSHm,_pop,0,                      topop|GETw2m,go|p1,0 },                      // 44: ... GETw2m
    { 0,PUSHp,any,POP2,0,                  go|p2,gc|m1,gv|m1,go|m1,GETw2p,gv|m1,0 },   // 45: GETw2p ...
    { 0,PUSHs,_pop,0,                      topop|GETw2s,go|p1,0 },                      // 46: ... GETw2s
    { 0,SUB1n,0,                           ifl|m2,0,ifl|0,rINC1,neg,0,ifl|p3,rDEC1,0,0 }, // 47: rDEC1 or rINC1 ?
    { 0,LNEG1,NE10f,0,                     go|p1,EQ10f,0 },                              // 48: EQ10f
    { 0,LNEG1,EQ10f,0,                     go|p1,NE10f,0 },                              // 49: NE10f
    { 0,EQ12,NE10f,0,                      go|p1,EQf,0 },                                // 50: EQf
    { 0,NE12,NE10f,0,                      go|p1,NEf,0 },                                // 51: NEf
    { 0,LT12,NE10f,0,                      go|p1,LTf,0 },                                // 52: LTf
    { 0,GT12,NE10f,0,                      go|p1,GTf,0 },                                // 53: GTf
    { 0,LE12,NE10f,0,                      go|p1,LEf,0 },                                // 54: LEf
    { 0,GE12,NE10f,0,                      go|p1,GEf,0 },                                // 55: GEf
    { 0,LT12u,NE10f,0,                     go|p1,LTuf,0 },                               // 56: LTuf
    { 0,GT12u,NE10f,0,                     go|p1,GTuf,0 },                               // 57: GTuf
    { 0,LE12u,NE10f,0,                     go|p1,LEuf,0 },                               // 58: LEuf
    { 0,GE12u,NE10f,0,                     go|p1,GEuf,0 },                               // 59: GEuf
    { 0,POINT2s,PUTwp1,sfree,0,            sum|p1,go|p1,PUTws1,gv|m1,0 },               // 60: PUTws1
    { 0,POINT2s,PUTbp1,sfree,0,            sum|p1,go|p1,PUTbs1,gv|m1,0 },               // 61: PUTbs1
    { 0,GETw1n,ASL12,0,                    ife|p1,go|p1,ASL1_1,0,0 },                   // 62: ASL1_1
    { 0,GETw1n,ASR12,0,                    ife|p1,go|p1,ASR1_1,0,0 },                   // 63: ASR1_1
    // (5) comparison + EQ10f (jump-if-false): mirror of seq50-59 for !(expr)
    // seq50-59 handle  COMPxx, NE10f -> COMPxxf  (jump if expr is TRUE)
    // seq64-73 handle  COMPxx, EQ10f -> COMPxxf  (jump if expr is FALSE)
    // i.e., the NOT-of-condition inverse.  When !(a==b) is tested, seq48
    // converts LNEG1,NE10f -> EQ10f, leaving EQ12,EQ10f in the buffer.
    // These seqs collapse that to a single inline CMP + inverted branch.
    { 0,EQ12,EQ10f,0,                      go | p1,NEf,0 },      // NEf
    { 0,NE12,EQ10f,0,                      go | p1,EQf,0 },      // EQf
    { 0,LT12,EQ10f,0,                      go | p1,GEf,0 },      // GEf
    { 0,GT12,EQ10f,0,                      go | p1,LEf,0 },      // LEf
    { 0,LE12,EQ10f,0,                      go | p1,GTf,0 },        // GTf
    { 0,GE12,EQ10f,0,                      go | p1,LTf,0 },         // LTf
    { 0,LT12u,EQ10f,0,                     go | p1,GEuf,0 },         // GEuf
    { 0,GT12u,EQ10f,0,                     go | p1,LEuf,0 },       // LEuf
    { 0,LE12u,EQ10f,0,                     go | p1,GTuf,0 },        // GTuf
    { 0,GE12u,EQ10f,0,                     go | p1,LTuf,0 }          // LTuf
};

// === assembly-code strings ==================================================

// First byte contains flag bits indicating:
//    the value in ax is needed (010) or zapped (020)
//    the value in bx is needed (001) or zapped (002)
char* code[PCODES] = {
    /* 0   (unused)   */ 0,
    /* 1   ADD12      */ "\211ADD AX,BX\n",
    /* 2   ADDSP      */ "\000?ADD SP,<n>\n??",
    /* 3   AND12      */ "\211AND AX,BX\n",
    /* 4   ANEG1      */ "\010NEG AX\n",
    /* 5   ARGCNTn    */ "\000?MOV CL,<n>?XOR CL,CL?\n",
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
    /* optimizer-generated: inline comparison + conditional jump */
    /* 107 EQf        */ "\011CMP AX,BX\nJE $+5\nJMP _<n>\n",
    /* 108 NEf        */ "\011CMP AX,BX\nJNE $+5\nJMP _<n>\n",
    /* 109 LTf        */ "\011CMP AX,BX\nJG $+5\nJMP _<n>\n",
    /* 110 GTf        */ "\011CMP AX,BX\nJL $+5\nJMP _<n>\n",
    /* 111 LEf        */ "\011CMP AX,BX\nJGE $+5\nJMP _<n>\n",
    /* 112 GEf        */ "\011CMP AX,BX\nJLE $+5\nJMP _<n>\n",
    /* 113 LTuf       */ "\011CMP AX,BX\nJA $+5\nJMP _<n>\n",
    /* 114 GTuf       */ "\011CMP AX,BX\nJB $+5\nJMP _<n>\n",
    /* 115 LEuf       */ "\011CMP AX,BX\nJAE $+5\nJMP _<n>\n",
    /* 116 GEuf       */ "\011CMP AX,BX\nJBE $+5\nJMP _<n>\n",
    /* optimizer-generated: direct stack stores */
    /* 117 PUTws1     */ "\010MOV <n>[BP],AX\n",
    /* 118 PUTbs1     */ "\010MOV BYTE PTR <n>[BP],AL\n",
    /* optimizer-generated: constant shift by 1 */
    /* 119 ASL1_1     */ "\001MOV AX,BX\nSHL AX,1\n",
    /* 120 ASR1_1     */ "\001MOV AX,BX\nSAR AX,1\n",
    /* 32-bit load/store */
    /* 121 GETd1m     */ "\020MOV AX,WORD PTR <m>\nMOV DX,WORD PTR <m>+2\n",
    /* 122 GETd1p     */ "\033MOV AX,[BX]\nMOV DX,2[BX]\n",       /* see gen() */
    /* 123 GETd1s     */ "\020MOV AX,<n>[BP]\nMOV DX,<n>+2[BP]\n",
    /* 124 GETdxn     */ "\020?MOV DX,<n>?XOR DX,DX?\n",
    /* 125 GETcxn     */ "\000?MOV CX,<n>?XOR CX,CX?\n",
    /* 126 GETd2m     */ "\003MOV BX,WORD PTR <m>\nMOV CX,WORD PTR <m>+2\n",
    /* 127 GETd2s     */ "\003MOV BX,<n>[BP]\nMOV CX,<n>+2[BP]\n",
    /* 128 PUTdm1     */ "\030MOV WORD PTR <m>,AX\nMOV WORD PTR <m>+2,DX\n",
    /* 129 PUTdp1     */ "\033PUSH BX\nMOV [BX],AX\nMOV 2[BX],DX\nPOP BX\n",
    /* 130 PUTds1     */ "\030MOV <n>[BP],AX\nMOV <n>+2[BP],DX\n",
    /* 32-bit stack */
    /* 131 PUSHd1     */ "\130PUSH DX\nPUSH AX\n",
    /* 132 POPd2      */ "\003POP BX\nPOP CX\n",
    /* 133 PUSHdm     */ "\100PUSH WORD PTR <m>+2\nPUSH WORD PTR <m>\n",
    /* 134 PUSHds     */ "\100PUSH <n>+2[BP]\nPUSH <n>[BP]\n",
    /* 32-bit move/swap */
    /* 135 MOVEd21    */ "\033MOV BX,AX\nMOV CX,DX\n",
    /* 136 SWAPd12    */ "\033XCHG AX,BX\nXCHG DX,CX\n",
    /* 32-bit widening */
    /* 137 WIDENs     */ "\030CWD\n",
    /* 138 WIDENu     */ "\020XOR DX,DX\n",
    /* 139 WIDENs2    */ "\003XOR CX,CX\nTEST BX,8000h\nJZ $+3\nDEC CX\n",
    /* 140 WIDENu2    */ "\001XOR CX,CX\n",
    /* 32-bit inline arithmetic */
    /* 141 ADDd12     */ "\033ADD AX,BX\nADC DX,CX\n",
    /* 142 SUBd12     */ "\033SUB AX,BX\nSBB DX,CX\n",              /* see gen() */
    /* 143 ANDd12     */ "\033AND AX,BX\nAND DX,CX\n",
    /* 144 ORd12      */ "\033OR AX,BX\nOR DX,CX\n",
    /* 145 XORd12     */ "\033XOR AX,BX\nXOR DX,CX\n",
    /* 146 COMd1      */ "\030NOT AX\nNOT DX\n",
    /* 147 ANEGd1     */ "\030NEG DX\nNEG AX\nSBB DX,0\n",
    /* 148 rINCd1     */ "\030ADD AX,<n>\nADC DX,0\n",
    /* 149 rDECd1     */ "\030SUB AX,<n>\nSBB DX,0\n",
    /* 150 DBLd1      */ "\030SHL AX,1\nRCL DX,1\n",
    /* 151 DBLd2      */ "\003SHL BX,1\nRCL CX,1\n",
    /* 32-bit library-call arithmetic */
    /* 152 MULd12     */ "\033CALL __lmul\n",
    /* 153 MULd12u    */ "\033CALL __ulmul\n",
    /* 154 DIVd12     */ "\033CALL __ldiv\n",                        /* see gen() */
    /* 155 DIVd12u    */ "\033CALL __uldiv\n",                       /* see gen() */
    /* 156 MODd12     */ "\033CALL __lmod\n",                        /* see gen() */
    /* 157 MODd12u    */ "\033CALL __ulmod\n",                       /* see gen() */
    /* 158 ASLd12     */ "\033CALL __lshl\n",
    /* 159 ASRd12     */ "\033CALL __lsar\n",
    /* 160 ASRd12u    */ "\033CALL __lshr\n",
    /* 32-bit library-call comparisons */
    /* 161 EQd12      */ "\033CALL __leq\n",
    /* 162 NEd12      */ "\033CALL __lne\n",
    /* 163 LTd12      */ "\033CALL __llt\n",
    /* 164 LEd12      */ "\033CALL __lle\n",
    /* 165 GTd12      */ "\033CALL __lgt\n",
    /* 166 GEd12      */ "\033CALL __lge\n",
    /* 167 LTd12u     */ "\033CALL __ullt\n",
    /* 168 LEd12u     */ "\033CALL __ulle\n",
    /* 169 GTd12u     */ "\033CALL __ulgt\n",
    /* 170 GEd12u     */ "\033CALL __ulge\n",
    /* 32-bit unary */
    /* 171 LNEGd1     */ "\030CALL __llneg\n",
    /* 32-bit truthiness */
    /* 172 EQd10f     */ "\030OR AX,DX\nJE $+5\nJMP _<n>\n",
    /* 173 NEd10f     */ "\030OR AX,DX\nJNE $+5\nJMP _<n>\n",
    /* 32-bit switch dispatch */
    /* 174 LSWITCHd   */ "\012CALL __lswitch\n"
};

//  === code generation functions =============================================

// print all assembler info before any code is generated
// and ensure that the segments appear in the correct order.
void header() {
    needLong = 0;
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

// print any assembler stuff needed at the end
void trailer() {
    char *cp;
    cptr = STARTGLB;
    while (cptr < ENDGLB) {
        if (cptr[IDENT] == IDENT_FUNCTION && cptr[CLASS] == AUTOEXT)
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
        int i, *count;
        printf(";opt   count\n");
        for (i = -1; ++i <= HIGH_SEQ; ) {
            count = seq[i];
            printf("; %2u   %5u\n", i, *count);
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
        outcode(pcode, value);
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

// dump the staging buffer
void dumpstage() {
    int i;
    stail = snext;
    snext = stage;
    while (snext < stail) {
        if (optimize) {
        restart:
            i = -1;
            while (++i <= HIGH_SEQ) if (peep(seqdata[i])) {
#ifdef DISOPT
                if (isatty(output))
                    fprintf(stderr, "                   optimized %2u\n", i);
#endif
                goto restart;
            }
        }
        outcode(snext[0], snext[1]);
        snext += 2;
    }
}

// change to a new segment
// may be called with NULL, CODESEG, or DATASEG
void toseg(int newseg) {
    if (oldseg == newseg)
            return;
    if (oldseg == CODESEG)
        outline("CODE ENDS");
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
    if (ident == IDENT_FUNCTION)
        toseg(CODESEG);
    else
        toseg(DATASEG);
    if (isGlobal) {
        outstr("PUBLIC ");
        outname(ssname);
        newline();
    }
    outname(ssname);
    if (ident == IDENT_FUNCTION) {
        colon();
        newline();
    }
}

// declare external reference
void external(char *name, int size, int ident) {
    if (ident == IDENT_FUNCTION)
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
                  && ident != IDENT_FUNCTION)
        outstr("BYTE");
    else if (size == BPD && ident != IDENT_POINTER && ident != IDENT_PTR_ARRAY
                         && ident != IDENT_FUNCTION)
        outstr("DWORD");
    else if (ident != IDENT_FUNCTION)
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
    seq++;
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
            case ife:   if (snext[1] != n) skip = YES;  break;
            case ifl:   if (snext[1] >= n) skip = YES;  break;
            case go:    snext += (n << 1);               break;
            case gc:    snext[0] = snext[(n << 1)];     goto done;
            case gv:    snext[1] = snext[(n << 1) + 1];   goto done;
            case sum:   snext[1] += snext[(n << 1) + 1];   goto done;
            case neg:   snext[1] = -snext[1];          goto done;
            case topop: pop[0] = n; pop[1] = snext[1]; goto done;
            case swv:   tmp = snext[1];
                snext[1] = snext[(n << 1) + 1];
                snext[(n << 1) + 1] = tmp;
            done:       reply = YES;
                break;
            }
        }
        else snext[0] = *seq;         // set p-code
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
// wasted with an ADDSP.
int getpop(int *next) {
    char *cp;
    int level;  level = 0;
    while (YES) {
        if (next >= stail)                     // compiler error
            return 0;
        if (*next == POP2)
            if (level) --level;
            else return next;                   // have a matching POP2
        else if (*next == ADDSP) {             // after func call
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
