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
#define STRMEM_SIZE     2
#define STRMEM_OFFSET   4
#define STRMEM_NAME     6   // null-terminated, max 12 chars + null
#define STRMEM_CLASSPTR 19  // ptr to struct def if member is TYPE_STRUCT
#define STRMEM_MAX      22

// struct data and member data definitions
#define STRDAT_NUM      20  // max 254
#define STRMEM_NUM      200
#define STRDAT_START    structdata
#define STRDAT_END      (structdata+STRDAT_NUM*STRDAT_MAX)
#define STRMEM_START    STRDAT_END
#define STRMEM_END      (STRMEM_START+STRMEM_NUM*STRMEM_MAX)
#define STRUCTDATSZ     4800 // STRDAT_NUM*STRDAT_MAX+STRMEM_NUM*STRMEM_MAX
