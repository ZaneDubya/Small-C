// struct/union tag data format
#define STRDAT_SIZE   0     // size of data in struct/union
#define STRDAT_MBEG   2     // ptr to beginning of struct/union member data
#define STRDAT_MEND   4     // ptr to end of struct/union member data
#define STRDAT_NAME   6     // name, length equal to NAMEMAX, null terminated
#define STRDAT_KIND   18    // 0 = struct, 1 = union (occupies former pad byte)
#define STRDAT_MAX    20    // NAMEMAX is 12, 6+12=18, 1b kind, 1b pad align

// STRDAT_KIND values
#define KIND_STRUCT  0
#define KIND_UNION   1

// struct member data format
#define STRMEM_IDENT    0   // ident as in cc.h, type as in cc.h. struct
#define STRMEM_TYPE     1   // members do not need class, all are auto.
#define STRMEM_PTRDEPTH 2   // pointer depth of member: 1=T*, 2=T**, etc.; 0 for non-pointers
#define STRMEM_BITFIELD 3   // 1 if member is a bit-field, 0 otherwise.
#define STRMEM_BITWIDTH 4   // total width of bitfield if member is a bitfield. 0 otherwise.
#define STRMEM_BITOFF   5   // bit offset of member within struct if member is a bitfield, 0 otherwise. max 15 bits.
#define STRMEM_SIZE     6   // size of member in bytes, including padding for alignment.
                            // (bitfields: size of underlying type)
#define STRMEM_OFFSET   8   // offset of member within struct in bytes, incl alignment padding.
                            // (bitfields: offset of underlying type)
#define STRMEM_CLASSPTR 10  // ptr to struct def if member is TYPE_STRUCT
#define STRMEM_NAME     12   // null-terminated, max 12 chars + null
#define STRMEM_PADDING  25   // padding to make size of member data 26 bytes
#define STRMEM_MAX      26

// struct data and member data definitions
#define STRDAT_NUM      20  // max 254
#define STRMEM_NUM      200
#define STRDAT_START    structdata
#define STRDAT_END      (structdata+STRDAT_NUM*STRDAT_MAX)
#define STRMEM_START    STRDAT_END
#define STRMEM_END      (STRMEM_START+STRMEM_NUM*STRMEM_MAX)
#define STRUCTDATSZ     (STRDAT_NUM*STRDAT_MAX+STRMEM_NUM*STRMEM_MAX) // 5600 bytes
