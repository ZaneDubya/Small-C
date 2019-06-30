/*
** CC.H -- Symbol Definitions for Small-C compiler.
*/

/*
** machine dependent parameters
*/
#define BPW       2   /* bytes per word */
#define LBPW      1   /* log2(BPW) */
#define SBPC      1   /* stack bytes per character */
#define ERRCODE   7   /* op sys return code */

/*
** symbol table format
*/
#define IDENT     0
#define TYPE      1
#define CLASS     2    // unused byte at index 3
#define CLASSPTR  4    // e.g. ptr to struct data in structdat
#define SIZE      6
#define OFFSET    8
#define NAME      10

#define SYMAVG    20 // avg name = 10 chars
#define SYMMAX    24 // max name = (14-1) chars 

/*
** symbol table parameters
*/
#define NUMLOCS   100
#define STARTLOC  symtab
#define ENDLOC    (symtab+NUMLOCS*SYMAVG)
#define NUMGLBS   400
#define STARTGLB  ENDLOC
#define ENDGLB    (ENDLOC+(NUMGLBS-1)*SYMMAX)
// Symbol table size == 9600  /* (NUMLOCS*SYMAVG + NUMGLBS*SYMMAX) */

/*
** system wide name size (for symbols)
*/
#define NAMESIZE 12
#define NAMEMAX  11

/*
** values for "IDENT"
*/
#define IDENT_LABEL    0
#define IDENT_VARIABLE 1
#define IDENT_ARRAY    2
#define IDENT_POINTER  3
#define IDENT_FUNCTION 4

/*
** values for "TYPE"
**    high order 6 bits give length of object
**    low order 2 bits make type unique within length
*/
#define TYPE_LABEL    0
#define IS_UNSIGNED   1
#define TYPE_CHR      (   1 << 2)         // 1, 0     == 0x04
#define TYPE_INT      ( BPW << 2)         // BPW, 0   == 0x08
#define TYPE_UCHR     ((  1 << 2) + 1)    // 1, 1     == 0x05
#define TYPE_UINT     ((BPW << 2) + 1)    // BPW, 1   == 0x09
#define TYPE_STRUCT   ((BPW << 2) + 2)    // BPW, 2   == 0x0A

/*
** values for "CLASS" integer
*/
#define AUTOMATIC 1   // defined locally
#define GLOBAL    2   // defined globally in this object file
#define EXTERNAL  3   // defined globally in another object file
#define AUTOEXT   4   // function that is not declared but is referenced
#define STATIC    5   // only visible in this file ("internal linkage")

/*
** segment types
*/
#define DATASEG 1
#define CODESEG 2

/*
** "switch" table
*/
#define SWSIZ   (2*BPW)
#define SWTABSZ (90*SWSIZ)

/*
** "while" queue
*/
#define WQTABSZ  30
#define WQSIZ     3
#define WQMAX   (wq+WQTABSZ-WQSIZ)

/*
** field offsets in "while" queue
*/
#define WQSP    0
#define WQLOOP  1
#define WQEXIT  2

/*
** literal pool
*/
#define LITABSZ 2000
#define LITMAX  (LITABSZ-1)

/*
** input line
*/
#define LINEMAX  127
#define LINESIZE 128

/*
** entries in staging buffer
*/
#define STAGESIZE   200

/*
** macro (#define) pool
*/
#define MACNBR   300
#define MACNSIZE (MACNBR*(NAMESIZE+2))
#define MACNEND  (macn+MACNSIZE)
#define MACQSIZE (MACNBR*7)
#define MACMAX   (MACQSIZE-1)

/*
** statement types
*/
#define STIF      1
#define STWHILE   2
#define STRETURN  3
#define STBREAK   4
#define STCONT    5
#define STASM     6
#define STEXPR    7
#define STDO      8
#define STFOR     9
#define STSWITCH 10
#define STCASE   11
#define STDEF    12
#define STGOTO   13
#define STLABEL  14

/*
** p-code symbols
**
** legend:
**  1 = primary register (pr in comments)
**  2 = secondary register (sr in comments)
**  b = byte
**  f = jump on false condition
**  l = current literal pool label number
**  m = memory reference by label
**  n = numeric constant
**  p = indirect reference thru pointer in sr
**  r = repeated r times
**  s = stack frame reference
**  u = unsigned
**  w = word
**  _ (tail) = another p-code completes this one
*/

/* compiler-generated */
#define ADD12     1   /* add sr to pr */
#define ADDSP     2   /* add to stack pointer */
#define AND12     3   /* AND sr to pr */
#define ANEG1     4   /* arith negate pr */
#define ARGCNTn   5   /* pass arg count to function */
#define ASL12     6   /* arith shift left sr by pr into pr */
#define ASR12     7   /* arith shift right sr by pr into pr */
#define CALL1     8   /* call function thru pr */
#define CALLm     9   /* call function directly */
#define BYTE_    10   /* define bytes (part 1) */
#define BYTEn    11   /* define byte of value n */
#define BYTEr0   12   /* define r bytes of value 0 */
#define COM1     13   /* ones complement pr */
#define DBL1     14   /* double pr */
#define DBL2     15   /* double sr */
#define DIV12    16   /* div pr by sr */
#define DIV12u   17   /* div pr by sr unsigned */
#define ENTER    18   /* set stack frame on function entry */
#define EQ10f    19   /* jump if (pr == 0) is false */
#define EQ12     20   /* set pr TRUE if (sr == pr) */
#define GE10f    21   /* jump if (pr >= 0) is false */
#define GE12     22   /* set pr TRUE if (sr >= pr) */
#define GE12u    23   /* set pr TRUE if (sr >= pr) unsigned */
#define POINT1l  24   /* point pr to function's literal pool */
#define POINT1m  25   /* point pr to mem item thru label */
#define GETb1m   26   /* get byte into pr from mem thru label */
#define GETb1mu  27   /* get unsigned byte into pr from mem thru label */
#define GETb1p   28   /* get byte into pr from mem thru sr ptr */
#define GETb1pu  29   /* get unsigned byte into pr from mem thru sr ptr */
#define GETw1m   30   /* get word into pr from mem thru label */
#define GETw1n   31   /* get word of value n into pr */
#define GETw1p   32   /* get word into pr from mem thru sr ptr */
#define GETw2n   33   /* get word of value n into sr */
#define GT10f    34   /* jump if (pr > 0) is false */
#define GT12     35   /* set pr TRUE if (sr > pr) */
#define GT12u    36   /* set pr TRUE if (sr > pr) unsigned */
#define WORD_    37   /* define word (part 1) */
#define WORDn    38   /* define word of value n */
#define WORDr0   39   /* define r words of value 0 */
#define JMPm     40   /* jump to label */
#define LABm     41   /* define label m */
#define LE10f    42   /* jump if (pr <= 0) is false */
#define LE12     43   /* set pr TRUE if (sr <= pr) */
#define LE12u    44   /* set pr TRUE if (sr <= pr) unsigned */
#define LNEG1    45   /* logical negate pr */
#define LT10f    46   /* jump if (pr < 0) is false */
#define LT12     47   /* set pr TRUE if (sr < pr) */
#define LT12u    48   /* set pr TRUE if (sr < pr) unsigned */
#define MOD12    49   /* modulo pr by sr */
#define MOD12u   50   /* modulo pr by sr unsigned */
#define MOVE21   51   /* move pr to sr */
#define MUL12    52   /* multiply pr by sr */
#define MUL12u   53   /* multiply pr by sr unsigned */
#define NE10f    54   /* jump if (pr != 0) is false */
#define NE12     55   /* set pr TRUE if (sr != pr) */
#define NEARm    56   /* define near pointer thru label */
#define OR12     57   /* OR sr onto pr */
#define POINT1s  58   /* point pr to stack item */
#define POP2     59   /* pop stack into sr */
#define PUSH1    60   /* push pr onto stack */
#define PUTbm1   61   /* put pr byte in mem thru label */
#define PUTbp1   62   /* put pr byte in mem thru sr ptr */
#define PUTwm1   63   /* put pr word in mem thru label */
#define PUTwp1   64   /* put pr word in mem thru sr ptr */
#define rDEC1    65   /* dec pr (may repeat) */
#define REFm     66   /* finish instruction with label */
#define RETURN   67   /* restore stack and return */
#define rINC1    68   /* inc pr (may repeat) */
#define SUB12    69   /* sub sr from pr */
#define SWAP12   70   /* swap pr and sr */
#define SWAP1s   71   /* swap pr and top of stack */
#define SWITCH   72   /* find switch case */
#define XOR12    73   /* XOR pr with sr */

		/* optimizer-generated */
#define ADD1n    74   /* add n to pr */
#define ADD21    75   /* add pr to sr */
#define ADD2n    76   /* add immediate to sr */
#define ADDbpn   77   /* add n to mem byte thru sr ptr */
#define ADDwpn   78   /* add n to mem word thru sr ptr */
#define ADDm_    79   /* add n to mem byte/word thru label (part 1) */
#define COMMAn   80   /* finish instruction with ,n */
#define DECbp    81   /* dec mem byte thru sr ptr */
#define DECwp    82   /* dec mem word thru sr ptr */
#define POINT2m  83   /* point sr to mem thru label */
#define POINT2m_ 84   /* point sr to mem thru label (part 1) */
#define GETb1s   85   /* get byte into pr from stack */
#define GETb1su  86   /* get unsigned byte into pr from stack */
#define GETw1m_  87   /* get word into pr from mem thru label (part 1) */
#define GETw1s   88   /* get word into pr from stack */
#define GETw2m   89   /* get word into sr from mem (label) */
#define GETw2p   90   /* get word into sr thru sr ptr */
#define GETw2s   91   /* get word into sr from stack */
#define INCbp    92   /* inc byte in mem thru sr ptr */
#define INCwp    93   /* inc word in mem thru sr ptr */
#define PLUSn    94   /* finish instruction with +n */
#define POINT2s  95   /* point sr to stack */
#define PUSH2    96   /* push sr */
#define PUSHm    97   /* push word from mem thru label */
#define PUSHp    98   /* push word from mem thru sr ptr */
#define PUSHs    99   /* push word from stack */
#define PUT_m_  100   /* put byte/word into mem thru label (part 1) */
#define rDEC2   101   /* dec sr (may repeat) */
#define rINC2   102   /* inc sr (may repeat) */
#define SUB_m_  103   /* sub from mem byte/word thru label (part 1) */
#define SUB1n   104   /* sub n from pr */
#define SUBbpn  105   /* sub n from mem byte thru sr ptr */
#define SUBwpn  106   /* sub n from mem word thru sr ptr */

#define PCODES  107   /* size of code[] */

