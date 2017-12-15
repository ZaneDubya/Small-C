/*
** asm.h -- miscellaneous definitions
*/

/* #define DEBUG                   /* include debugging displays */
#define COMMENT    ';'          /* comment delimiter */
#define ANOTHER    '|'          /* another operand option */
#define PREF      0xFF          /* code for prefix in M.I.T. */
#define STACK     2048          /* reserved for stack space */
#define MAXSEG      10          /* maximum segments per module */
#define MAXFN       41          /* max file name space */
#define MAXNAM      31          /* maximum name characters */
#define MAXLINE     81          /* length of source line */
#define LASTLINE    60          /* last line of listing page */
#define SRCEXT   ".ASM"         /* source file extension */
#define OBJEXT   ".OBJ"         /* object file extension */

/*
** symbol table
** 
**             ÚÄÄÄÂÄÄÄ¿
**   STSTR     ³      ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ> name
**             ÃÄÄÄÅÄÄÄÁÄÄÄÂÄÄÄ¿
**   STVAL2    ³               ³
**             ÃÄÄÄÅÄÄÄÅÄÄÄÁÄÄÄÙ
**   STVAL1    ³       ³
**             ÃÄÄÄÅÄÄÄÙ
**   STNDX     ³   ³
**             ÃÄÄÄ´
**   STSIZE    ³   ³
**             ÃÄÄÄÅÄÄÄ¿
**   STFLAGS   ³       ³
**             ÀÄÄÄÁÄÄÄÙ
**   STENTRY
**
*/
#define STSTR        0          /* offset to string pointer */
#define STVAL2  (STSTR   + 2)   /* offset to 2 word value */
#define STVAL1  (STVAL2  + 4)   /* offset to 1 word value */
#define STNDX   (STVAL1  + 2)   /* offset to object file index */
#define STSIZE  (STNDX   + 1)   /* offset to data definition size */
#define STFLAGS (STSIZE  + 1)   /* offset to flag word */
#define STENTRY (STFLAGS + 2)   /* st entry size */
#define STMAX     1000          /* maximum lables allowed */
#define STRAVG      10          /* average symbol string size */

/*
** symbol flags
*/
#define FGRP   0x0001  /* group */
#define FSEG   0x0002  /* segment */
#define FPRO   0x0004  /* proc */
#define FFAR   0x0008  /* far proc */
#define FCLS   0x0010  /* class name */
#define FEQU   0x0020  /* equate symbol */
#define FSET   0x0040  /* set symbol */
#define FEXT   0x0080  /* external */
#define FPUB   0x0100  /* public */
#define FCOD   0x0200  /* code */
#define FDAT   0x0400  /* data */
#define FRED   0x1000  /* report redefinition error on pass 2 */

/*
** macro table
*/
#define MTNXT        0          /* pointer to next macro */
#define MTNAM        2          /* macro name */

/*
** macro p-codes
*/
#define MACRO        1          /* begin a macro */
#define ENDM         2          /* end a macro */
#define CALL         3          /* call a macro */

/*
** Machine Instruction Directives
*/
#define HEX    1  /* hex byte follows */
#define M01    2  /* (mm000r/m) <- opnd 1 */
#define M11    3  /* (mm001r/m) <- opnd 1 */
#define M21    4  /* (mm010r/m) <- opnd 1 */
#define M31    5  /* (mm011r/m) <- opnd 1 */
#define M41    6  /* (mm100r/m) <- opnd 1 */
#define M51    7  /* (mm101r/m) <- opnd 1 */
#define M61    8  /* (mm110r/m) <- opnd 1 */
#define M71    9  /* (mm111r/m) <- opnd 1 */
#define MR1   10  /* (mmregr/m) <- opnd 1 */
#define MR12  11  /* (..reg...) <- opnd 1, (mm...r/m) <- opnd 2 */
#define MR21  12  /* (mm...r/m) <- opnd 1, (..reg...) <- opnd 2 */
#define MI12  13  /* (..iii...) <- opnd 1, (mm...r/m) <- opnd 2 */
#define MS12  14  /* (..sss...) <- opnd 1, (mm...r/m) <- opnd 2 */
#define MS21  15  /* (mm...r/m) <- opnd 1, (..sss...) <- opnd 2 */
#define PF    16  /* add 8 to prior byte if in far proc */
#define PI1   17  /* add opnd 1 i (bits 5-4-3) to prior byte */
#define PR1   18  /* add opnd 1 reg code to prior byte */
#define PR2   19  /* add opnd 2 reg code to prior byte */
#define I1    20  /* 8, 16, 32 bit value of i from opnd 1 */
#define I2    21  /* 8, 16, 32 bit value of i from opnd 2 */
#define I3    22  /* 8, 16, 32 bit value of i from opnd 3 */
#define O1    23  /* 16, 32 bit addr offset from opnd 1 */
#define O2    24  /* 16, 32 bit addr offset from opnd 2 */
#define P1    25  /* 32, 48 bit procedure pointer from opnd 1 */
#define P2    26  /* 32, 48 bit procedure pointer from opnd 2 */
#define S1    27  /* 8, 16, 32 bit self-rel displ from opnd 1 */
#define W6    28  /* generate WAIT (9B) if .8086 */

/*
**    Expression Token Codes
*/
#define OR     0x01  /* |  */
#define XOR    0x02  /* ^  */
#define AND    0x03  /* &  */
#define EQ     0x04  /* == */
#define NE     0x05  /* != */
#define LE     0x06  /* <= */
#define GE     0x07  /* >= */
#define LT     0x08  /* <  */
#define GT     0x09  /* >  */
#define RSH    0x0A  /* >> */
#define LSH    0x0B  /* << */
#define PLUS   0x0C  /* +  */
#define MINUS  0x0D  /* -  */
#define MULT   0x0E  /* *  */
#define DIV    0x0F  /* /  */
#define MOD    0x10  /* %  */
#define CPL    0x11  /* ~  */
#define NOT    0x12  /* !  */
#define LPN    0x13  /* (  */
#define RPN    0x14  /* )  */
#define LOC    0x15  /* $  */
#define SYM    0x16  /* symbol */
#define NUM    0x17  /* number */
#define EOE    0x18  /* end of expr */
#define LBR    0x19  /* [  */
#define RBR    0x1A  /* ]  */
#define PRD    0x1B  /* .  */
#define COL    0x1C  /* :  */
#define REG    0x1D  /* register */
#define SEG    0x1E  /* SEGMENT */
#define OFF    0x1F  /* OFFSET */
#define SHORT  0x20  /* SHORT */

/*
**    Expression Types
*/
#define Ix     0x30  /* i?? -  immed opnd w/o size bits */
#define I6     0x3E  /* i6  -  6 bit immed opnd */
#define I8     0x31  /* i8  -  8 bit immed opnd */
#define I16    0x32  /* i16 - 16 bit immed opnd */
#define I32    0x34  /* i32 - 32 bit immed opnd */
#define ONE    0x35  /* 1   - literal '1' */
#define THREE  0x37  /* 3   - literal '3' */
#define I1632  0x38  /* i16 or i32 depending on OSA */

#define  R8    0x40  /* r8  -  8 bit register */
#define  AL    0x41
#define  BL    0x42
#define  CL    0x43
#define  DL    0x44
#define  AH    0x45
#define  BH    0x46
#define  CH    0x47
#define  DH    0x48

#define  R16   0x50  /* r16 - 16 bit register */
#define  AX    0x51
#define  BX    0x52
#define  CX    0x53
#define  DX    0x54
#define  SP    0x55
#define  BP    0x56
#define  SI    0x57
#define  DI    0x58

#define R32    0x60  /* r32 - 32 bit register */
#define EAX    0x61
#define EBX    0x62
#define ECX    0x63
#define EDX    0x64
#define ESP    0x65
#define EBP    0x66
#define ESI    0x67
#define EDI    0x68
#define eAX    0x69  /*  AX or EAX depending on OSA */
#define R1632  0x6A  /* r16 or r32 depending on OSA */
 
#define xS     0x70  /* xS  - segment register */
#define CS     0x71
#define SS     0x72
#define DS     0x73
#define ES     0x74
#define FS     0x75
#define GS     0x76

#define STx    0x77  /* STx - ST(0),...,ST(7) */
#define ST0    0x78
#define ST1    0x79
#define ST2    0x7A
#define ST3    0x7B
#define ST4    0x7C
#define ST5    0x7D
#define ST6    0x7E
#define ST7    0x7F

#define CRx    0x49  /* CRx - CR0, CR2, CR3 */
#define CR0    0x4A
#define CR2    0x4B
#define CR3    0x4C

#define DRx    0x59  /* DRx - DR0, DR1, DR2, DR3, DR6, DR7 */
#define DR0    0x5A
#define DR1    0x5B
#define DR2    0x5C
#define DR3    0x5D
#define DR6    0x5E
#define DR7    0x5F

#define TRx    0x6B  /* TRx - TR6, TR7 */
#define TR6    0x6C
#define TR7    0x6D

#define M      0x80  /* m   - mem operand */
#define M8     0x81  /* m8  -  8 bit mem operand */
#define M16    0x82  /* m16 - 16 bit mem operand */
#define M32    0x84  /* m32 - 32 bit mem operand */
#define M48    0x86  /* m48 - 48 bit mem operand */
#define M64    0x88  /* m64 - 64 bit mem operand */
#define M80    0x8A  /* m80 - 80 bit mem operand */
#define M1632  0x8B  /* m16 or m32 depending on OSA */
#define M3248  0x8C  /* m32 or m48 depending on OSA */
#define M3264  0x8D  /* m32 or m64 depending on OSA */

#define POINT  0x90
#define P32    0x94  /* p32 - 32 bit mem pointer to far procedure */
#define P48    0x96  /* p48 - 48 bit mem pointer to far procedure */
#define P3248  0x97  /* p32 or p48 depending on OSA */

#define SELF   0xA0
#define S8     0xA1  /* $8  -  8 bit self-rel addr */
#define S1632  0xA5  /* $16 or $32 depending on OSA */

#define _x     0xB0  /* /x  -  any size reg/mem opnd */
#define _8     0xB1  /* /8  -  8 bit reg/mem opnd */
#define _16    0xB2  /* /16 - 16 bit reg/mem opnd */
#define _32    0xB4  /* /32 - 32 bit reg/mem opnd */
#define _1632  0xB5  /* /16 or /32 depending on OSA */
#define _48    0xB6  /* m48 - 48 bit indir mem opnd */
#define _64    0xB8  /* m64 - 64 bit indir mem opnd */
#define _80    0xBA  /* m80 - 80 bit indir mem opnd */

#define N8     0xC1  /* n8  -  8 bit named mem opnd */
#define N16    0xC2  /* n16 - 16 bit named mem opnd */
#define N32    0xC4  /* n32 - 32 bit named mem opnd */
#define N1632  0xC5  /* n16 or n32 named mem opnd depending on OSA */

#define TYPES  0xC6    /* number of type codes */

/*
** Expression Type Bits
** 
** ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
** ³T T T T T TIND ³    operand    ³
** ³S F R M C (asa)³ type    size  ³  0
** ³H W E E O ÄÄÄÄÄ³ÄÄÄÄÄÄÄ ÄÄÄÄÄÄÄ³
** ³O D G M N a a a³x x x x s s s s³
** ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
** ³               ³  T T T   T    ³
** ³               ³  B S D   B    ³  1 <-- 8086 indirect registers
** ³               ³  P I I   X    ³
** ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
** ³               ³T T T T T T T T³
** ³               ³S B S D A B C D³  2 <-- 80386 indirect E registers
** ³               ³P P I I X X X X³
** ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
** ³ index scaleÄ¿ ³  T T T T T T T³
** ³            ÄÄÄ³  B S D A B C D³  3 <-- 80386 indirect E index registers
** ³            s s³  P I I X X X X³
** ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
*/
#define TIND   0x0700  /* indirect address-size-attribute */
#define TCON   0x0800  /* constant */
#define TMEM   0x1000  /* memory ref */
#define TREG   0x2000  /* register */
#define TFWD   0x4000  /* forward memory ref */
#define TSHO   0x8000  /* short ref */
#define TDX    0x0001  /* indirect via DX or EDX */
#define TCX    0x0002  /* indirect via CX or ECX */
#define TBX    0x0004  /* indirect via BX or EBX */
#define TAX    0x0008  /* indirect via AX or EAX */
#define TDI    0x0010  /* indirect via DI or EDI */
#define TSI    0x0020  /* indirect via SI or ESI */
#define TBP    0x0040  /* indirect via BP or EBP */
#define TSP    0x0080  /* indirect via SP or ESP */

/*
** Segment Register Prefix Indexes
**        cpu code + 1
*/
#define P_ES   1
#define P_CS   2
#define P_SS   3
#define P_DS   4
#define P_FS   5
#define P_GS   6

