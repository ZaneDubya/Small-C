/******************************************************************************
* overlays.h -- overlay support for ylink
* Declares all overlay constants, globals, and function prototypes.
* Include this in any file that needs to interact with the overlay system.
******************************************************************************/
#ifndef OVERLAYS_H
#define OVERLAYS_H

// --- Overlay window table ---------------------------------------------------
// A window is a region of the resident code segment that is time-shared by
// one or more overlay modules. Each window entry is OVL_WIN_PER uints wide.
#define OVL_WIN_MAX 8      // max overlay windows
#define OVL_WIN_PER 3      // fields per window entry
#define OVLW_ID   0        // user-assigned window number (1-based in directive)
#define OVLW_BASE 1        // CS offset where window starts (assigned in Pass3)
#define OVLW_SIZE 2        // window size in bytes (max of member module sizes)

// --- Overlay module table ----------------------------------------------------
// A module is a set of OBJ modules that are swapped in/out together.
// Multiple modules can share a window; only one loads at a time.
// Each module entry is OVL_MOD_PER uints wide.
#define OVL_MOD_MAX 32     // max overlay modules
#define OVL_MOD_PER 4      // fields per module entry
#define OVLG_WIN 0         // 0-based index into ovlWinData
#define OVLG_NAM 1         // ptr to null-terminated module name string
#define OVLG_SIZ 2         // total code size of this module (computed in Pass3)
#define OVLG_ORG 3         // code origin = window base (assigned in Pass3)

// --- Overlay stub table -----------------------------------------------------
// A stub is a small trampoline emitted in the resident code segment for each
// unique cross-module call target. Each stub entry is OVL_STB_PER uints wide.
#define OVL_STB_MAX 256    // max overlay stubs (unique overlay entry points)
#define OVL_STB_PER 3      // fields per stub entry
#define OVLS_PBS 0         // pbdf index of the target public definition
#define OVLS_COF 1         // CS offset of this stub in resident code segment
#define OVLS_MOD 2         // overlay module index of the target function

// --- Misc -------------------------------------------------------------------
#define OVL_MOD_MOD_NONE 0xFF  // modOvlMod value: module is resident (not in a module)
// 11 byte stub: PUSH enc(2+1); CALL rel16(2+2); POP AX(1); PUSH tgt(2+2); RET(1);
#define OVL_STUB_BYTES 11

// --- Global variables (defined in overlays.c) -------------------------------
extern char *pathOverlay;   // path to overlay directive file (-od=)
extern uint *ovlWinData;    // window table: OVL_WIN_MAX entries x OVL_WIN_PER uints
extern int ovlWinCount;     // number of windows declared
extern uint *ovlModData;    // module table: OVL_MOD_MAX entries x OVL_MOD_PER uints
extern int ovlModCount;     // number of modules declared
extern byte *modOvlMod;     // per-module module assignment: OVL_MOD_MOD_NONE or module index
extern uint *ovlStubData;   // stub table: OVL_STB_MAX entries x OVL_STB_PER uints
extern int ovlStubCount;    // number of stubs registered
extern uint ovlStubBase;    // CS offset of first stub in resident code segment
extern uint ovlCodeFd;      // temp file fd for overlay code writes; 0xffff = inactive

// --- Function prototypes ----------------------------------------------------
void OvlAllocInit();
char *OvlFileBase(char *path);
void ParseOverlayDef();
void P2_ForceInclude(char *name);
void WarnCrossWindow();
void PassOvl();
void OvlLayoutSegments();
uint OvlGetWriteFd(uint outfd, byte segType);
int OvlLookupStub(int rawPbdfIdx);
void OvlEmitStubs(uint outfd);
int FindStub(int pbdfIdx);
void WriteOvlToc(uint outfd, uint *modExeOffLo, uint *modExeOffHi);
void PatchOvlTocOff(uint outfd);
void WriteOvlBlocks();

#endif
