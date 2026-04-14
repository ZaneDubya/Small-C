// struct/union tag data format
#define STRDAT_SIZE   0     // size of data in struct/union
#define STRDAT_MBEG   2     // ptr to beginning of struct/union member data
#define STRDAT_MEND   4     // ptr to end of struct/union member data
#define STRDAT_NAME   6     // name, length equal to NAMEMAX (13), null terminated
#define STRDAT_KIND   19    // 0 = struct, 1 = union
#define STRDAT_MAX    20    // NAMEMAX is 12, 6+12+1=19, 1b kind

// STRDAT_KIND values
#define KIND_STRUCT  0
#define KIND_UNION   1

// struct member data format
#define STRMEM_SIZE     0   // size of member in bytes, including padding for alignment.
                            // (bitfields: size of underlying type)
#define STRMEM_OFFSET   2   // offset of member within struct in bytes, incl alignment padding.
                            // (bitfields: offset of underlying type)
#define STRMEM_CLASSPTR 4   // ptr to struct def if member is TYPE_STRUCT
#define STRMEM_IDENT    6   // ident as in cc.h, type as in cc.h. struct
#define STRMEM_TYPE     7   // members do not need class, all are auto.
#define STRMEM_PTRDEPTH 8   // pointer depth of member: 1=T*, 2=T**, etc.; 0 for non-pointers
#define STRMEM_BITWIDTH 9   // bit-field width in bits (1..16); 0 = not a bit-field
#define STRMEM_BITOFF   10  // bit offset of bit-field within allocation unit (0..15); 0 if not a bit-field
#define STRMEM_NAME     11   // name, length equal to NAMEMAX (13), null terminated
#define STRMEM_MAX      24

// struct data and member data definitions
#define STRDAT_NUM      20  // max 254
#define STRMEM_NUM      100
#define STRDAT_START    structdata
#define STRDAT_END      (structdata+STRDAT_NUM*STRDAT_MAX)
#define STRMEM_START    STRDAT_END
#define STRMEM_END      (STRMEM_START+STRMEM_NUM*STRMEM_MAX)
#define STRUCTDATSZ     (STRDAT_NUM*STRDAT_MAX+STRMEM_NUM*STRMEM_MAX) // 2800 bytes
