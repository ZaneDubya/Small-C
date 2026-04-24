#define uint unsigned int
#define byte unsigned char
#define size_t unsigned int

#define DICT_NAME_LENGTH 10
#define DICT_DATA_LENGTH 3
#define DICT_BLOCK_CNT 37

#define DEPEND_DATA_LENGTH 3

#define THEADR 0x80
#define COMMNT 0x88
#define MODEND 0x8A
#define EXTDEF 0x8C
#define PUBDEF 0x90
#define LNAMES 0x96
#define SEGDEF 0x98
#define FIXUPP 0x9C
#define LEDATA 0xA0
#define LIDATA 0xA2
#define LIBHDR 0xF0
#define LIBEND 0xF1
#define LIBDEP 0xF2

#define TWOTABS "\n    "

// --- Linker options ---------------------------------------------------------
#define SEG_ALIGNMENT 2    // align code/data segments to this many bytes
#define LIB_MODEND_ALIGN 16

// --- EXE output -------------------------------------------------------------
// Header is 64 bytes: 30 bytes fixed + up to 8 reloc entries (4 bytes each).
#define EXE_HDR_LEN 64
#define RELOC_COUNT 8
#define RELOC_PER 1

// --- Line buffer ------------------------------------------------------------
#define LINEMAX  127       // max chars read (not including null)
#define LINESIZE 128       // buffer size (LINEMAX + 1)

// --- Input file table -------------------------------------------------------
#define FILE_MAX 96        // max number of input OBJ/LIB files

// --- Module data (modData[]) ------------------------------------------------
// Each module occupies MDAT_PER uint slots.
// MDAT_CSO / MDAT_DSO hold the raw segment size before Pass3,
// and the segment origin (offset from start of segment) after Pass3.
#define MOD_MAX 128
#define MDAT_NAM 0         // ptr to module name string
#define MDAT_CSO 1         // code segment size / origin
#define MDAT_DSO 2         // data segment size / origin
#define MDAT_THD 3         // file offset of THEADR record
#define MDAT_FLG 4         // lo byte: file index; hi byte: flags
#define MDAT_PER 5
#define FlgInLib  0x0100   // module came from a library
#define FlgStart  0x0200   // module contains the program entry point
#define FlgStack  0x0400   // module contains the stack segment
#define FlgInclude 0x0800  // module is included in the output

// --- LNAMES (local segment names, per module) -------------------------------
#define LNAMES_CNT 4
#define LNAME_NULL  0xFF
#define LNAME_CODE  0x00
#define LNAME_DATA  0x01
#define LNAME_STACK 0x02

// --- Segment table (segLengths[], locSegs[]) --------------------------------
#define SEGS_CNT 3
#define SEG_CODE  0
#define SEG_DATA  1
#define SEG_STACK 2
#define SEG_NOTPRESENT 0xFF

// --- Public definitions (pbdfData[]) ----------------------------------------
// Each pubdef occupies PBDF_PER int slots.
#define PBDF_CNT 640
#define PBDF_NAME  0       // ptr to name string
#define PBDF_WHERE 1       // hi byte: module index; lo byte: segment index
#define PBDF_ADDR  2       // offset within segment
#define PBDF_PER   3

// --- External definition buffer ---------------------------------------------
#define EXTBUF_LEN 1536
#define NOT_INCLUDED -2    // pubdef exists but owning module not included
#define NOT_DEFINED  -1    // no matching pubdef found