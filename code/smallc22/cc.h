// CC.H -- Symbol Definitions for Small-C compiler.

// #define DIAG_OUTPUT      // verbose compiler diagnostics
#define ENABLE_ENUMS
#define ENABLE_TYPEDEF
// #define ENABLE_WARNINGS  // enable optional compiler warnings
#ifdef ENABLE_WARNINGS
#define WARN_IMPLICIT    // warn about implicit int and undeclared functions
#define WARN_ARGCOUNT    // warn about wrong number of arguments in function calls
#endif

// machine dependent parameters
#define BPW       2   // bytes per word
#define LBPW      1   // log2(BPW)
#define BPD       4   // bytes per dword (long)
#define LBPD      2   // log2(BPD)
#define SBPC      1   // stack bytes per character
#define ERRCODE   7   // op sys return code

// === The Symbol Table =======================================================
// ============================================================================
// The symbol table is a single flat byte array. As the compiler processes the
// source code, it adds entries for identifiers to this table.
// Each entry in the table is a flat 24-byte record laid out as follows:
//   [0]    IDENT    (1 byte)  — symbol kind (IDENT_VARIABLE, IDENT_ARRAY, ...)
//   [1]    TYPE     (1 byte)  — element data type (TYPE_INT, TYPE_CHR, ...)
//   [2]    CLASS    (1 byte)  — storage class (AUTOMATIC, GLOBAL, ...) + optional CONST_FLAG
//   [3]    NDIM     (1 byte)  — dimension count for multi-dim arrays; else 0
//   [4..5] CLASSPTR (2 bytes) — pointer to auxiliary metadata (struct def or dimdata)
//   [6..7] SIZE     (2 bytes) — total object size in bytes
//   [8..9] OFFSET   (2 bytes) — address offset (meaning depends on CLASS; see below)
//   [10..22] NAME  (13 bytes) — null-terminated identifier (max NAMEMAX=12 chars + NUL)
//   [23]  (1 pad byte)
// IDENT: the kind/category of the symbol.
//   IDENT_LABEL      — a goto label (local table only); TYPE is 1 if defined, 0 if forward ref
//   IDENT_VARIABLE   — scalar variable of any type, or an enum constant
//   IDENT_ARRAY      — array variable (any element type, any number of dimensions)
//   IDENT_POINTER    — pointer variable, or an array parameter that has decayed to a pointer
//   IDENT_FUNCTION   — function whose return value is a non-pointer type
//   IDENT_PTR_ARRAY  — array of pointers (e.g. char *arr[])
//   IDENT_PTR_FUNCTION — function that returns a pointer
// TYPE: the TYPE_xxx constant for the element/base type of the symbol.
//   For scalars and pointers, this is the declared type (TYPE_INT, TYPE_CHR, TYPE_LONG, etc.).
//   For arrays, this is the element type (e.g. int arr[5] -> TYPE_INT).
//   For functions, this is the return type (or the pointed-to type for IDENT_PTR_FUNCTION).
//   For enum constants, always TYPE_INT.
//   For goto labels, TYPE is REUSED: 1 = label is defined, 0 = forward reference only.
//   Encoding: high 6 bits = element size in bytes (TYPE_xxx >> 2); low 2 bits = variant
//     (bit 0 = IS_UNSIGNED flag; bit 1 used to distinguish TYPE_STRUCT and TYPE_VOID).
// CLASS: storage class of the symbol.  High bit (CONST_FLAG = 0x80) may be OR'd in when the
//   symbol was declared with the 'const' qualifier.  Strip it with (ptr[CLASS] & 0x7F).
//   AUTOMATIC  — local variable or function argument; addressed via BP-relative OFFSET
//   GLOBAL     — file-global with external linkage; emits PUBLIC directive
//   EXTERNAL   — extern declaration; emits EXTRN directive
//   AUTOEXT    — auto-declared (undeclared function call); upgraded to GLOBAL on definition
//   PROTOEXT   — prototype declared but not yet called; upgraded to AUTOEXT on first call
//   STATIC     — file-internal linkage (static global) or static local (OFFSET -> global entry)
//   ENUMCONST  — enum constant; integer value is stored in OFFSET
//   0          — used for goto label entries (TYPE_LABEL == 0)
// NDIM: number of dimensions for multi-dimensional arrays (MAX_DIMS = 8 max).
//   0 or 1 — scalar, pointer, or 1D array (normal case; single subscript uses element-size scaling)
//   2..8   — multi-dimensional array; CLASSPTR points to a dimdata[] entry that holds
//            per-dimension stride values.  primary() initialises is[DIM_LEFT]=NDIM and
//            level14() decrements it for each consumed [index].
// CLASSPTR: pointer to auxiliary type metadata; meaning depends on NDIM and TYPE.
//   TYPE_STRUCT, NDIM <= 1 — direct pointer into structdata[] for this struct type; used to
//     look up members (getStructMember) and struct size (getStructSize).
//   NDIM > 1 (any element type) — pointer into dimdata[]; layout of that entry:
//     [0..1] structPtr  — pointer to struct def (0 if element is not a struct)
//     [2..3] stride_0   — byte stride for outermost dimension (e.g. N*elemSize for [M][N])
//     [4..5] stride_1   — byte stride for next dimension (3D+ arrays)
//     ... (one entry per intermediate dimension; final dimension uses element-size scaling)
//   All other symbols — 0.
// SIZE: total allocated size of the object in bytes (what sizeof() returns).
//   Scalar char/uchar: 1.  Scalar int/uint: 2.  Scalar long/ulong: 4.
//   Pointer (any type): BPW (2).  Pointer array *arr[N]: N * BPW.
//   1D array: N * (TYPE >> 2).  Multi-dim array: product of all dimensions * element size.
//   Struct variable or struct array: sum of member sizes (via getStructSize).
//   Function, goto label, K&R placeholder: 0.
// OFFSET: address offset; meaning is determined by CLASS.
//   GLOBAL / EXTERNAL / STATIC (global entry) — always 0; symbol addressed by assembler label.
//   AUTOMATIC (local variable) — negative BP-relative byte offset computed as (csp - declared).
//     First int in a function gets -2, first long gets -4, etc.
//   AUTOMATIC (function argument) — positive BP-relative byte offset after argument layout is
//     finalised.  Arguments are pushed right-to-left, so the first arg has the lowest
//     offset: for f(a,b,c): a=[BP+4], b=[BP+6], c=[BP+8].  Long args occupy 4 bytes (BPD).
//   STATIC (local entry) — raw char* pointer to the corresponding global table entry; primary()
//     follows this indirection to map the local name to the global _L<N> label.
//   ENUMCONST — the signed 16-bit integer value of the enum constant.
//   IDENT_LABEL — the assembler label number (from getlabel()); passed to gen(LABm/JMPm, ...).

#define IDENT     0
#define TYPE      1
#define CLASS     2
#define NDIM      3     // number of dimensions: 0/1 = scalar/pointer/1D array; 2+ = multi-dim
#define CLASSPTR  4     // ptr to structdata[] (TYPE_STRUCT, NDIM<=1), dimdata[] (NDIM>1), or 0
#define SIZE      6     // for objects, total object size in bytes (sizeof); for functions, this is FNPARAMPTR
#define FNPARAMPTR SIZE // for IDENT_FUNCTION/IDENT_PTR_FUNCTION: index into paramTypes[]
#define OFFSET    8     // GLOBAL=0; AUTO local=neg BP offset; AUTO arg=pos BP offset; ENUMCONST=value;
                        // STATIC local=ptr to global entry; LABEL=label number
#define NAME      10
#define SYMAVG    24    // local entry stride: NAME(10)+NAMEMAX(12)+NUL(1)+1pad = 24
#define SYMMAX    24    // global entry stride: NAME(10)+NAMEMAX(12)+NUL(1)+1pad = 24

// symbol table parameters
#define NUMLOCS   100
#define STARTLOC  symtab
#define ENDLOC    (symtab+NUMLOCS*SYMAVG)
#define NUMGLBS   380

#define STARTGLB  ENDLOC
#define ENDGLB    (ENDLOC+(NUMGLBS-1)*SYMMAX)
// Symbol table size == (NUMLOCS*SYMAVG + NUMGLBS*SYMMAX)

// array initializer limit
#define MAXARRAYDIM 256


// system wide name size (for symbols)
#define NAMESIZE 13
#define NAMEMAX  12

// values for "IDENT"
#define IDENT_LABEL     0
#define IDENT_VARIABLE  1
#define IDENT_ARRAY     2
#define IDENT_POINTER   3
#define IDENT_FUNCTION      4
#define IDENT_PTR_ARRAY     5
#define IDENT_PTR_FUNCTION  6

// values for "TYPE"
//    high order 6 bits give length of object
//    low order 2 bits make type unique within length
#define TYPE_LABEL    0
#define TYPE_VOID     2   // void type for void pointers; size field = 0  (0x02 >> 2 == 0)
#define IS_UNSIGNED   1
#define TYPE_CHR      (   1 << 2)         // 1, 0     == 0x04
#define TYPE_INT      ( BPW << 2)         // BPW, 0   == 0x08
#define TYPE_UCHR     ((  1 << 2) + 1)    // 1, 1     == 0x05
#define TYPE_UINT     ((BPW << 2) + 1)    // BPW, 1   == 0x09
#define TYPE_STRUCT   ((BPW << 2) + 2)    // BPW, 2   == 0x0A
#define TYPE_LONG     ( BPD << 2)         // BPD, 0   == 0x10
#define TYPE_ULONG    ((BPD << 2) + 1)    // BPD, 1   == 0x11

// values for "CLASS" integer
#define AUTOMATIC 1   // defined locally
#define GLOBAL    2   // defined globally in this object file
#define EXTERNAL  3   // defined globally in another object file
#define AUTOEXT   4   // function that is not declared but is referenced
#define STATIC    5   // only visible in this file ("internal linkage")
#define ENUMCONST 6   // enum constant: integer value stored in OFFSET field
#define PROTOEXT  7   // function prototype declared but not yet called or defined
#define TYPEDEF   8   // typedef alias: IDENT=IDENT_VARIABLE or IDENT_POINTER, TYPE=aliased type

// const qualifier flag packed into CLASS byte (high bit; CLASS values are <= 6).
// To test: ptr[CLASS] & CONST_FLAG  (non-zero == const-qualified)
// To strip: ptr[CLASS] & 0x7F      (yields the raw storage class)
#define CONST_FLAG  0x80

// segment types
#define DATASEG 1
#define CODESEG 2

// "switch" table
#define SWSIZ   (2*BPW)
#define SWTABSZ (90*SWSIZ)

// "while" queue
#define WQTABSZ  30
#define WQSIZ     3
#define WQMAX   (wq+WQTABSZ-WQSIZ)

// field offsets in "while" queue
#define WQSP    0
#define WQLOOP  1
#define WQEXIT  2

// literal pool
#define LITABSZ 5000

#define LITMAX  (LITABSZ-1)

// multi-dimensional array metadata buffer
#define DIMDATSZ  500  // dimdata buffer size
#define MAX_DIMS    8  // max dimensions per array
#define FNPARAMTS_SZ 768 // size of paramTypes[] function parameter-type buffer

// argument buffer for two-pass right-to-left argument pushing
// First pass collects each arg's p-codes here; second pass emits them reversed.
// argbufcur is saved/restored per callfunc nesting level, so this only needs
// to hold the peak concurrent usage across all active nesting levels at once.
#define MAX_CALL_ARGS  12   // max arguments to any one function call
#define ARGBUF_SIZE   400   // int slots (peak concurrent p-codes in flight)

// is[] array -- expression state (8 ints)
// Starting with level1(), this code places information about the expression
// under analysis into the is[] and is2[] arrays. These are local arrays of
// seven integers each. The term 'is' in these array names is not an acronym,
// but the word. These arrays tell the analyzer what the entire expression or
// any part of it is. In other words, they carry the properties of whatever has
// been parsed at every point in the process.
// This first instance of the array is for the left leg of the descent through
// all of the levels. Another (is2[]) is declared--for use with the right
// operand--at each level where a binary operator is recognized. Before control
// rises to a higher level, the left array is adjusted to reflect the result of
// the operation, and the right, being local and having served its purpose, is
// discarded. At lower levels, the right array appears as a left array, and
// other right arrays are declared as other binary operators are encountered.

#define SYMTAB_ADR 0    // is[SYMTAB_ADR] contains the address of the symbol
                        // table entry that describes the operand. Information
                        // from the symbol table is used when generating code
                        // that access the operand. Constants are not included
                        // in the symbol table, and thus this is zero. Only the
                        // primary operand has an address in the symbol table,
                        // thus when the primary operand combines into a larger
                        // entity, this value is reset to zero.

#define TYP_OBJ 1       // is[TYP_OBJ] contains the data type of an indirectly
                        // referenced object; these are referenced by way of an
                        // address in a register. This includes (1) function
                        // arguments, (2) local objects, and (3) globally
                        // declared arrays. In the first two cases, the address
                        // is calculated relative to BP, the stack frame ptr.
                        // In the third case an array element's address is
                        // calculated relative to the label which identifies
                        // the array. Static objects other than arrays have a
                        // zero in this element because they are directly
                        // reference by their label. In some cases (array names
                        // without subscripts or leading ampersand), we only
                        // need the address, so this element does not matter.

#define TYP_ADR 2       // is[TYP_ADR] contains the data type of an address
                        // (pointer, array, &variable); otherwise, zero.

#define TYP_CNST 3      // is[TYP_CNST] contains the data type (INT or UINT) if
                        // the (sub)expression is a constant value; otherwise,
                        // zero. The unsigned designation applies only to
                        // values over 32767 that are written without a sign.

#define VAL_CNST 4      // is[VAL_CNST] contains the value produced by a
                        // constant (sub)expression. It is used for other
                        // purposes when the (sub)expression is not constant.

#define LAST_OP 5       // is[LAST_OP] contains the p-code that generates the
                        // highest binary operator in an expression. Example:
                        // for the expression "left oper zero" (where left is
                        // the left subexpression, oper is a binary operator,
                        // and zero is a subexpression which evaluates to zero)
                        // which form of optimized code to generate. This
                        // element is used by test().

#define STG_ADR 6       // is[STG_ADR] contains the staging buffer address
                        // where code that evaluates the oper zero part of an
                        // expression of the form "left oper zero" is stored.
                        // If not zero, it tells test() that better code can be
                        // generated and how much of the end of the staging
                        // buffer to replace.

#define DIM_LEFT 7      // is[DIM_LEFT] remaining dimensions to subscript.
                        // Set in primary() for multi-dim arrays (NDIM > 1).
                        // Decremented in level14() for each [index].
                        // When it reaches 1, the final subscript uses
                        // element-size scaling (normal path).

#define VAL_CNST_HI 8  // is[VAL_CNST_HI] contains the high 16-bit word of a
                        // 32-bit constant value when TYP_CNST is TYPE_LONG or
                        // TYPE_ULONG. Zero for 16-bit constants.

#define TYP_VAL 9       // is[TYP_VAL] holds the TYPE_xxx of the current value
                        // after an explicit cast (applycast), so that a
                        // subsequent widening cast can see whether the value
                        // is signed or unsigned. Zero when no cast has been
                        // applied (e.g. bare function call return values).

#define ISSIZE 10       // number of entries in is[] arrays

// input line
#define LINEMAX  127
#define LINESIZE 128

// entries in staging buffer
#define STAGESIZE   300


// macro (#define) pool
#define MACNBR   500

#define MACNSIZE (MACNBR*(NAMESIZE+2))
#define MACNEND  (macn+MACNSIZE)
#define MACQSIZE (MACNBR*7)
#define MACMAX   (MACQSIZE-1)

// statement types
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

#define FUNCBUFSIZE 2000  // int slots in per-function buffer (1000 p-code pairs, 4000 bytes)

// === P-code definitions =====================================================
// ============================================================================
// The Small-C compiler generates code for a simple stack-based virtual machine
// with the instruction set defined by the symbolic names below.
// legend:
//  1 = primary register (pr in comments)
//  2 = secondary register (sr in comments)
//  b = byte
//  f = jump on false condition
//  l = current literal pool label number
//  m = memory reference by label
//  n = numeric constant
//  p = indirect reference thru pointer in sr
//  r = repeated r times
//  s = stack frame reference
//  u = unsigned
//  w = word
//  _ (tail) = another p-code completes this one
// compiler-generated
#define ADD12     1   // add sr to pr
#define ADDSP     2   // add to stack pointer
#define AND12     3   // AND sr to pr
#define ANEG1     4   // arith negate pr
/* removed (rev 139): was pass arg count in CL; */
#define ASL12     6   // arith shift left sr by pr into pr
#define ASR12     7   // arith shift right sr by pr into pr
#define CALL1     8   // call function thru pr
#define CALLm     9   // call function directly
#define BYTE_    10   // define bytes (part 1)
#define BYTEn    11   // define byte of value n
#define BYTEr0   12   // define r bytes of value 0
#define COM1     13   // ones complement pr
#define DBL1     14   // double pr
#define DBL2     15   // double sr
#define DIV12    16   // div pr by sr
#define DIV12u   17   // div pr by sr unsigned
#define ENTER    18   // set stack frame on function entry
#define EQ10f    19   // jump if (pr == 0) is false
#define EQ12     20   // set pr TRUE if (sr == pr)
#define GE10f    21   // jump if (pr >= 0) is false
#define GE12     22   // set pr TRUE if (sr >= pr)
#define GE12u    23   // set pr TRUE if (sr >= pr) unsigned
#define POINT1l  24   // point pr to function's literal pool
#define POINT1m  25   // point pr to mem item thru label
#define GETb1m   26   // get byte into pr from mem thru label
#define GETb1mu  27   // get unsigned byte into pr from mem thru label
#define GETb1p   28   // get byte into pr from mem thru sr ptr
#define GETb1pu  29   // get unsigned byte into pr from mem thru sr ptr
#define GETw1m   30   // get word into pr from mem thru label
#define GETw1n   31   // get word of value n into pr
#define GETw1p   32   // get word into pr from mem thru sr ptr
#define GETw2n   33   // get word of value n into sr
#define GT10f    34   // jump if (pr > 0) is false
#define GT12     35   // set pr TRUE if (sr > pr)
#define GT12u    36   // set pr TRUE if (sr > pr) unsigned
#define WORD_    37   // define word (part 1)
#define WORDn    38   // define word of value n
#define WORDr0   39   // define r words of value 0
#define JMPm     40   // jump to label
#define LABm     41   // define label m
#define LE10f    42   // jump if (pr <= 0) is false
#define LE12     43   // set pr TRUE if (sr <= pr)
#define LE12u    44   // set pr TRUE if (sr <= pr) unsigned
#define LNEG1    45   // logical negate pr
#define LT10f    46   // jump if (pr < 0) is false
#define LT12     47   // set pr TRUE if (sr < pr)
#define LT12u    48   // set pr TRUE if (sr < pr) unsigned
#define MOD12    49   // modulo pr by sr
#define MOD12u   50   // modulo pr by sr unsigned
#define MOVE21   51   // move pr to sr
#define MUL12    52   // multiply pr by sr
#define MUL12u   53   // multiply pr by sr unsigned
#define NE10f    54   // jump if (pr != 0) is false
#define NE12     55   // set pr TRUE if (sr != pr)
#define NEARm    56   // define near pointer thru label
#define OR12     57   // OR sr onto pr
#define POINT1s  58   // point pr to stack item
#define POP2     59   // pop stack into sr
#define PUSH1    60   // push pr onto stack
#define PUTbm1   61   // put pr byte in mem thru label
#define PUTbp1   62   // put pr byte in mem thru sr ptr
#define PUTwm1   63   // put pr word in mem thru label
#define PUTwp1   64   // put pr word in mem thru sr ptr
#define rDEC1    65   // dec pr (may repeat)
#define REFm     66   // finish instruction with label
#define RETURN   67   // restore stack and return
#define rINC1    68   // inc pr (may repeat)
#define SUB12    69   // sub sr from pr
#define SWAP12   70   // swap pr and sr
#define SWAP1s   71   // swap pr and top of stack
#define SWITCH   72   // find switch case
#define XOR12    73   // XOR pr with sr

// optimizer-generated
#define ADD1n    74   // add n to pr
#define ADD21    75   // add pr to sr
#define ADD2n    76   // add immediate to sr
#define ADDbpn   77   // add n to mem byte thru sr ptr
#define ADDwpn   78   // add n to mem word thru sr ptr
#define ADDm_    79   // add n to mem byte/word thru label (part 1)
#define COMMAn   80   // finish instruction with ,n
#define DECbp    81   // dec mem byte thru sr ptr
#define DECwp    82   // dec mem word thru sr ptr
#define POINT2m  83   // point sr to mem thru label
#define POINT2m_ 84   // point sr to mem thru label (part 1)
#define GETb1s   85   // get byte into pr from stack
#define GETb1su  86   // get unsigned byte into pr from stack
#define GETw1m_  87   // get word into pr from mem thru label (part 1)
#define GETw1s   88   // get word into pr from stack
#define GETw2m   89   // get word into sr from mem (label)
#define GETw2p   90   // get word into sr thru sr ptr
#define GETw2s   91   // get word into sr from stack
#define INCbp    92   // inc byte in mem thru sr ptr
#define INCwp    93   // inc word in mem thru sr ptr
#define PLUSn    94   // finish instruction with +n
#define POINT2s  95   // point sr to stack
#define PUSH2    96   // push sr
#define PUSHm    97   // push word from mem thru label
#define PUSHp    98   // push word from mem thru sr ptr
#define PUSHs    99   // push word from stack
#define PUT_m_  100   // put byte/word into mem thru label (part 1)
#define rDEC2   101   // dec sr (may repeat)
#define rINC2   102   // inc sr (may repeat)
#define SUB_m_  103   // sub from mem byte/word thru label (part 1)
#define SUB1n   104   // sub n from pr
#define SUBbpn  105   // sub n from mem byte thru sr ptr
#define SUBwpn  106   // sub n from mem word thru sr ptr


// optimizer-generated: direct stack stores
#define PUTws1  107   // put pr word into stack frame
#define PUTbs1  108   // put pr byte into stack frame

// optimizer-generated: constant shift by 1
#define ASL1_1  109   // shift left sr by 1 into pr
#define ASR1_1  110   // shift right sr by 1 into pr

// 32-bit (long) p-codes -- load/store
//  d = dword (32-bit), using DX:AX (primary) / CX:BX (secondary)
#define GETd1m  111   // load dword into DX:AX from global label
#define GETd1p  112   // load dword into DX:AX thru pointer in BX
#define GETd1s  113   // load dword into DX:AX from stack frame
#define GETdxn  114   // load constant into DX (high word of primary)
#define GETcxn  115   // load constant into CX (high word of secondary)
#define GETd2m  116   // load dword into CX:BX from global label
#define GETd2s  117   // load dword into CX:BX from stack frame
#define PUTdm1  118   // store DX:AX dword to global label
#define PUTdp1  119   // store DX:AX dword thru pointer in BX
#define PUTds1  120   // store DX:AX dword to stack frame

// 32-bit stack operations
#define PUSHd1  121   // push DX:AX (4 bytes)
#define POPd2   122   // pop 4 bytes into CX:BX
#define PUSHdm  123   // push dword from global label
#define PUSHds  124   // push dword from stack frame

// 32-bit move/swap
#define MOVEd21 125   // move DX:AX to CX:BX
#define SWAPd12 126   // swap DX:AX with CX:BX

// 32-bit widening
#define WIDENs  127   // sign-extend AX to DX:AX (CWD)
#define WIDENu  128   // zero-extend AX to DX:AX (XOR DX,DX)
#define WIDENs2 129   // sign-extend BX to CX:BX
#define WIDENu2 130   // zero-extend BX to CX:BX

// 32-bit inline arithmetic
#define ADDd12  131   // DX:AX += CX:BX
#define SUBd12  132   // DX:AX -= CX:BX  (auto-swap)
#define ANDd12  133   // DX:AX &= CX:BX
#define ORd12   134   // DX:AX |= CX:BX
#define XORd12  135   // DX:AX ^= CX:BX
#define COMd1   136   // ~DX:AX (ones complement)
#define ANEGd1  137   // -DX:AX (arith negate)
#define rINCd1  138   // DX:AX += n (32-bit increment)
#define rDECd1  139   // DX:AX -= n (32-bit decrement)
#define DBLd1   140   // DX:AX <<= 1
#define DBLd2   141   // CX:BX <<= 1

// 32-bit library-call arithmetic
#define MULd12  142   // DX:AX *= CX:BX (signed)
#define MULd12u 143   // DX:AX *= CX:BX (unsigned)
#define DIVd12  144   // DX:AX /= CX:BX (signed, auto-swap)
#define DIVd12u 145   // DX:AX /= CX:BX (unsigned, auto-swap)
#define MODd12  146   // DX:AX %= CX:BX (signed, auto-swap)
#define MODd12u 147   // DX:AX %= CX:BX (unsigned, auto-swap)
#define ASLd12  148   // DX:AX <<= CL
#define ASRd12  149   // DX:AX >>= CL (signed)
#define ASRd12u 150   // DX:AX >>= CL (unsigned)

// 32-bit library-call comparisons (result: AX = 0 or 1, 16-bit)
#define EQd12   151   // AX = (DX:AX == CX:BX)
#define NEd12   152   // AX = (DX:AX != CX:BX)
#define LTd12   153   // AX = (DX:AX <  CX:BX) signed
#define LEd12   154   // AX = (DX:AX <= CX:BX) signed
#define GTd12   155   // AX = (DX:AX >  CX:BX) signed
#define GEd12   156   // AX = (DX:AX >= CX:BX) signed
#define LTd12u  157   // AX = (DX:AX <  CX:BX) unsigned
#define LEd12u  158   // AX = (DX:AX <= CX:BX) unsigned
#define GTd12u  159   // AX = (DX:AX >  CX:BX) unsigned
#define GEd12u  160   // AX = (DX:AX >= CX:BX) unsigned

// 32-bit unary (library)
#define LNEGd1  161   // AX = !DX:AX (result is 16-bit 0/1)

// 32-bit truthiness (conditional jump)
#define EQd10f  162   // jump if (DX:AX == 0) is false
#define NEd10f  163   // jump if (DX:AX != 0) is false

// 32-bit switch dispatch
#define LSWITCHd 164  // CALL __lswitch
#define POPCX   165   // POP CX           (replaces ADD SP,2)
#define POPCX2  166   // POP CX / POP CX  (replaces ADD SP,4)
// short-branch variants (two-pass optimizer, generated by flushfunc)
#define EQ10fs  167   // OR AX,AX / JNE SHORT _n  (replaces EQ10f)
#define NE10fs  168   // OR AX,AX / JE  SHORT _n  (replaces NE10f)
#define GE10fs  169   // OR AX,AX / JL  SHORT _n  (replaces GE10f)
#define GT10fs  170   // OR AX,AX / JLE SHORT _n  (replaces GT10f)
#define LE10fs  171   // OR AX,AX / JG  SHORT _n  (replaces LE10f)
#define LT10fs  172   // OR AX,AX / JGE SHORT _n  (replaces LT10f)
#define EQd10fs 173   // OR AX,DX  / JNE SHORT _n (replaces EQd10f)
#define NEd10fs 174   // OR AX,DX  / JE  SHORT _n (replaces NEd10f)
#define JMPs    175   // JMP SHORT _n             (replaces JMPm)
// factored comparison p-codes (optimizer-generated)
#define NOP_    176   // no operation (optimizer padding, emits nothing)
#define CMP1n   177   // CMP AX,<n>  (compare AX with immediate)
// factored conditional jumps — named by the SKIP condition in "Jcc $+5 / JMP _n"
#define JccE    178   // JE  $+5 / JMP _n         (skip if equal)
#define JccNE   179   // JNE $+5 / JMP _n         (skip if not equal)
#define JccG    180   // JG  $+5 / JMP _n         (skip if greater, signed)
#define JccL    181   // JL  $+5 / JMP _n         (skip if less, signed)
#define JccGE   182   // JGE $+5 / JMP _n         (skip if greater or equal, signed)
#define JccLE   183   // JLE $+5 / JMP _n         (skip if less or equal, signed)
#define JccA    184   // JA  $+5 / JMP _n         (skip if above, unsigned)
#define JccB    185   // JB  $+5 / JMP _n         (skip if below, unsigned)
#define JccAE   186   // JAE $+5 / JMP _n         (skip if above or equal, unsigned)
#define JccBE   187   // JBE $+5 / JMP _n         (skip if below or equal, unsigned)
// factored short-branch conditional jumps (inverted condition, direct jump)
#define JccEs   188   // JNE SHORT _n             (replaces JccE)
#define JccNEs  189   // JE  SHORT _n             (replaces JccNE)
#define JccGs   190   // JLE SHORT _n             (replaces JccG)
#define JccLs   191   // JGE SHORT _n             (replaces JccL)
#define JccGEs  192   // JL  SHORT _n             (replaces JccGE)
#define JccLEs  193   // JG  SHORT _n             (replaces JccLE)
#define JccAs   194   // JBE SHORT _n             (replaces JccA)
#define JccBs   195   // JAE SHORT _n             (replaces JccB)
#define JccAEs  196   // JB  SHORT _n             (replaces JccAE)
#define JccBEs  197   // JA  SHORT _n             (replaces JccBE)
// factored register comparison
#define CMP12   198   // CMP AX,BX (compare pr with sr, sets flags only)
// factored memory/stack comparisons
#define CMP1m   199   // CMP AX,WORD PTR [mem] (compare pr with global, sets flags only)
#define CMP1s   200   // CMP AX,n[BP] (compare pr with stack var, sets flags only)
// zero-test branch p-codes without redundant OR AX,AX prefix (requires SETSFLG on predecessor)
#define NE10fp  201   // JNE $+5 / JMP _n  (NE10f without OR AX,AX)
#define EQ10fp  202   // JE $+5 / JMP _n   (EQ10f without OR AX,AX)
#define NE10fps 203   // JE SHORT _n       (short form of NE10fp)
#define EQ10fps 204   // JNE SHORT _n      (short form of EQ10fp)
#define RETNOFP 205   // RET only (frame pointer omitted — no BP setup/teardown)

#define PCODES  206   // size of code[]

// function prototypes for Small-C *.C files
int AddSymbol(char *sname, char id, char type, int size, int offset, int *lgpp, int class);
int FindSymbol(char *sname, char *buf, int len, char *end, int max, int off, int namelen);
int IsMatch(char *lit);
int IsUnsigned(int is[]);
int alpha(char c);
int amatch(char *lit, int len);
int an(char c);
int callfunc(char *ptr);
int dofunction(int class);
int dotype(int *typeSubPtr);
int endst();
int findglb(char *sname);
int findloc(char *sname);
int gch();
int getClsPtr(char *sym);
int getDimStride(char *sym, int idx);
int getStructMember(char *basestruct, char *sname);
int getStructPtr();
int getStructPtr();
int getStructSize(char *basestruct);
int getint(char *addr, int len);
int getlabel();
int inbyte();
int IsConstExpr(int *val);
int isLongVal(int is[]);
int isreserved(char *name);
int level1(int is[]);
int nextsym(char *entry);
int nextop(char *list);
int parseLocalDeclare(int type, int typeSubPtr, int defArrTyp, int *id, int *sz);
int primary(int *is);
int streq(char str1[], char str2[]);
int string(int *offset);
int symname(char *sname);
int white();
void addwhile(int ptr[]);
void ClearStage(int *before, int *start);
void GenTestAndJmp(int label, int reqParens);
void ParseExpression(int *con, int *val);
void ReqSemicolon();
void Require(char *str);
void blanks();
void bump(int n);
void declglb(int type, int class, int typeSubPtr);
void delwhile();
void doInline();
void error(char msg[]);
void fetch(int is[]);
void flushfunc();
void gen(int pcode, int value);
void illname();
void kill();
void lout(char *line, int fd);
void needlval();
void newline();
void openfile();
void outdec(int number);
void putint(int i, char *addr, int len);
int readwhile(int *ptr);
void setstage(int *before, int *start);
void skipToNextToken();
void stowlit(int value, int size);
#ifdef ENABLE_WARNINGS
void warning(char msg[]);
void warningWithName(char msg[], char *name, char suffix[]);
#endif
int storeDimDat(int sptr, int ndim, int strides[]);
int IsCExpr32(int *val, int *val_hi);
void toseg(int newseg);
void dumpzero(int size, int count);
void decGlobal(int ident, int isGlobal);
void multidef(char *sname);
void external(char *name, int size, int ident);
void initStructs();
void getRunArgs();
void preprocess();
void header();
void trailer();
int dostructblock();
void point();
int storeParamTypes(char *typebuf, int nparams_byte);
#ifdef ENABLE_ENUMS
void initEnums();
int doEnum(int *typeSubPtr);
#endif
#ifdef ENABLE_TYPEDEF
int doTypedef();
#endif
