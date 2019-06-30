// struct data format
#define STRDAT_SIZE   0     // size of data in struct
#define STRDAT_MBEG   2     // ptr to beginning of struct member data
#define STRDAT_MEND   4     // ptr to end of struct member data
#define STRDAT_NAME   6     // name, length equal to NAMEMAX, null terminated
#define STRDAT_MAX    20    // NAMEMAX is 12, 6+12=18, 1b null, 1b pad align

// struct member data format
#define STRMEM_IDENT    0   // ident as in cc.h, type as in cc.h. struct
#define STRMEM_TYPE     1   // members do not need class, all are auto.
#define STRMEM_SIZE     2
#define STRMEM_OFFSET   4
#define STRMEM_NAME     6   // null-terminated
#define STRMEM_MAX      20

// struct data and member data definitions
#define STRDAT_NUM      20  // max 254
#define STRMEM_NUM      200
#define STRDAT_START    structdata
#define STRDAT_END      (structdata+STRDAT_NUM*STRDAT_MAX)
#define STRMEM_START    STRDAT_END
#define STRMEM_END      (STRMEM_START+STRMEM_NUM*STRMEM_MAX)
#define STRUCTDATSZ     4400 // STRDAT_NUM*STRDAT_MAX+STRMEM_NUM*STRMEM_MAX
