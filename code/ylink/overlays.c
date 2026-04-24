/******************************************************************************
* overlays.c -- overlay support for ylink
*
* Overlay model:
*   - A "window" is a region of the resident code segment used as a load area.
*     Multiple modules may share a window; only one is resident at a time.
*   - A "module" is an OBJ that is loaded in a window.
*   - A "stub" is a small trampoline (OVL_STUB_BYTES bytes) emitted in the
*     resident code segment for each cross-module call target. Callers link to
*     the stub; the stub invokes the overlay manager (__ovl_check) to load the
*     correct module before jumping to the target function.
*
* Entry points called from link.c (in order of use):
*   OvlAllocInit()       -- allocate/init all overlay tables (from AllocAll)
*   ParseOverlayDef()    -- read -od= directive file after Pass1
*   WarnCrossWindow()    -- warn about same-window cross-module calls
*   PassOvl()            -- scan EXTDEFs, register stubs
*   P2_ForceInclude(n)   -- force-include the overlay manager module
*   OvlLayoutSegments()  -- assign window/stub/module origins (from Pass3)
*   OvlGetWriteFd(fd,t)  -- redirect CODE writes to temp file when active
*   OvlLookupStub(idx)   -- check if a pubdef needs a stub (from P4_FixExt)
*   OvlEmitStubs(outfd)  -- write stub bytes into the output EXE (from Pass4)
*   WriteOvlBlocks()     -- append overlay module blocks + TOC to the EXE
******************************************************************************/
#include <stdio.h>
#include "notice.h"
#include "link.h"
#include "overlays.h"

// === Globals from link.c ====================================================
// These are defined in link.c and accessed here via extern.
extern char *line;
extern uint fdDebug;
extern char *pathOutput;
extern int *filePaths;
extern int fileCount;
extern uint *modData;
extern int modCount;
extern int *pbdfData;
extern uint *segLengths;
extern int extNext, extCount;

// === Functions from link.c ==================================================
char *AllocMem(int nItems, int itemSize);
int FindPubDef(char *name, int retAllDefined);
void P4_DoMod(uint fd, uint outfd, uint *codeBase, uint *dataBase);
void forward(uint fd, uint offset);
void fatal(char *str);
void fatalf(char *format, char *arg);
void fatalfl(char *msg, long arg);
int safefopen(char *path, char *opts);
void safefclose(uint fd);
uint read_u16(uint fd);
int read_u8(uint fd);
int readstr(char *str, uint size, uint fd);
int readstrpre(char *str, uint fd);
void write_f8(uint fd, char value);
void write_f16(uint fd, uint value);
int toupper(int c);

// === Overlay globals ========================================================
// All overlay state is held here; accessed from link.c via overlays.h externs.
char *pathOverlay;   // path to overlay directive file (-od=)
uint *ovlWinData;    // window table: OVL_WIN_MAX entries x OVL_WIN_PER uints
int ovlWinCount;
uint *ovlModData;    // module table: OVL_MOD_MAX entries x OVL_MOD_PER uints
int ovlModCount;
byte *modOvlMod;     // per-module: OVL_MOD_MOD_NONE or 0-based module index
uint *ovlStubData;   // stub table: OVL_STB_MAX entries x OVL_STB_PER uints
int ovlStubCount;
uint ovlStubBase;    // CS offset of the first stub in the resident code segment
uint ovlCodeFd;      // temp file fd for overlay CODE writes; 0xffff = inactive
uint ovlTocOffLo;    // low word of TOC block file offset (written to __ovl_toc_off)
uint ovlTocOffHi;    // high word of TOC block file offset

// ============================================================================
// OvlAllocInit -- allocate and zero-initialise all overlay data structures.
// Called once from AllocAll() before any linking passes run.
// ============================================================================
void OvlAllocInit() {
  int i;
  ovlWinData  = AllocMem(OVL_WIN_MAX * OVL_WIN_PER, 2);
  ovlModData  = AllocMem(OVL_MOD_MAX * OVL_MOD_PER, 2);
  modOvlMod   = AllocMem(MOD_MAX, 1);
  ovlStubData = AllocMem(OVL_STB_MAX * OVL_STB_PER, 2);
  for (i = 0; i < MOD_MAX; i++) {
    modOvlMod[i] = OVL_MOD_MOD_NONE;
  }
  pathOverlay  = 0;
  ovlWinCount  = 0;
  ovlModCount  = 0;
  ovlStubCount = 0;
  ovlStubBase  = 0;
  ovlCodeFd    = 0xffff;
  ovlTocOffLo  = 0;
  ovlTocOffHi  = 0;
}

// ============================================================================
// OvlFileBase -- return pointer to the basename (last path component) of path.
// Scans for the last backslash, forward-slash, or colon.
// ============================================================================
char *OvlFileBase(char *path) {
  char *base, *c;
  base = path;
  c = path;
  while (*c != 0) {
    if (*c == '\\' || *c == '/' || *c == ':') base = c + 1;
    c++;
  }
  return base;
}

// ============================================================================
// ParseOverlayDef -- read the overlay directive file and assign each OBJ
// module to a set of modules that will be assigned to an overlay window. Must
// be called after Pass1 so modData[] is fully populated.
//
// Directive file grammar:
//   WINDOW n         -- declare a window with user-visible number n (>= 1)
//   MODULE name win   -- start a module named <name> assigned to window <win>
//   FILE.OBJ         -- assign all modules from this OBJ to the current module
//   ; ...            -- comment line (ignored)
//   blank line       -- ignored
// ============================================================================
void ParseOverlayDef() {
  uint fd;
  int curMod, winIdx, modWinId, fileIdx, modIdx, i, j;
  int found, modLen;
  char *p, *q, *basename, *modName;
  if (pathOverlay == 0) return;
  puts("  Parsing overlay directives");
  fd = safefopen(pathOverlay, "r");
  curMod = -1;
  while (readstr(line, LINEMAX, fd) != 0) {
    p = line;
    while (*p == ' ' || *p == '\t') p++;          // skip leading whitespace
    if (*p == 0 || *p == ';') continue;            // blank line or comment
    q = p + strlen(p);
    while (q > p && (*(q-1) == ' ' || *(q-1) == '\t')) q--;
    *q = 0;                                        // strip trailing whitespace
    if (*p == 0) continue;

    if (strncmp(p, "WINDOW", 6) == 0 && (p[6] == ' ' || p[6] == '\t' || p[6] == 0)) {
      // --- WINDOW n ---
      p += 6;
      while (*p == ' ' || *p == '\t') p++;
      winIdx = atoi(p);
      if (winIdx <= 0) fatal("ParseOverlayDef: WINDOW number must be positive.");
      for (i = 0; i < ovlWinCount; i++) {
        if ((int)ovlWinData[i * OVL_WIN_PER + OVLW_ID] == winIdx)
          fatalf("ParseOverlayDef: Duplicate WINDOW %u.", winIdx);
      }
      if (ovlWinCount >= OVL_WIN_MAX) fatal("ParseOverlayDef: Too many windows (max 8).");
      ovlWinData[ovlWinCount * OVL_WIN_PER + OVLW_ID]   = (uint)winIdx;
      ovlWinData[ovlWinCount * OVL_WIN_PER + OVLW_BASE]  = 0;
      ovlWinData[ovlWinCount * OVL_WIN_PER + OVLW_SIZE]  = 0;
      if (fdDebug != 0xffff)
        fprintf(fdDebug, "  OVL WINDOW %u (idx %u)\n", winIdx, ovlWinCount);
      ovlWinCount += 1;
    }
    else if (strncmp(p, "MODULE", 6) == 0 && (p[6] == ' ' || p[6] == '\t')) {
      // --- MODULE name winnum ---
      p += 6;
      while (*p == ' ' || *p == '\t') p++;
      q = p;
      while (*q != 0 && *q != ' ' && *q != '\t') q++;  // find end of name token
      modLen = q - p;
      if (modLen == 0) fatal("ParseOverlayDef: MODULE requires a name.");
      modName = AllocMem(modLen + 1, 1);
      for (i = 0; i < modLen; i++) modName[i] = p[i];
      modName[modLen] = 0;
      while (*q == ' ' || *q == '\t') q++;
      modWinId = atoi(q);
      if (modWinId <= 0) fatal("ParseOverlayDef: MODULE requires a positive window number.");
      winIdx = -1;
      for (i = 0; i < ovlWinCount; i++) {
        if ((int)ovlWinData[i * OVL_WIN_PER + OVLW_ID] == modWinId) { winIdx = i; break; }
      }
      if (winIdx < 0) fatalf("ParseOverlayDef: MODULE references undeclared WINDOW %u.", modWinId);
      if (ovlModCount >= OVL_MOD_MAX) fatal("ParseOverlayDef: Too many Modules (max 32).");
      curMod = ovlModCount;
      ovlModData[curMod * OVL_MOD_PER + OVLG_WIN] = (uint)winIdx;
      ovlModData[curMod * OVL_MOD_PER + OVLG_NAM] = (uint)modName;
      ovlModData[curMod * OVL_MOD_PER + OVLG_SIZ] = 0;
      ovlModData[curMod * OVL_MOD_PER + OVLG_ORG] = 0;
      if (fdDebug != 0xffff)
        fprintf(fdDebug, "  OVL MODULE %s (win idx %u, module idx %u)\n", modName, winIdx, curMod);
      ovlModCount += 1;
    }
    else {
      // --- OBJ filename: assign all modules from this file to curMod ---
      if (curMod < 0) fatal("ParseOverlayDef: OBJ file listed before any MODULE declaration.");
      found = 0;
      for (fileIdx = 0; fileIdx < fileCount; fileIdx++) {
        basename = OvlFileBase(filePaths[fileIdx]);
        j = 0;
        while (toupper(basename[j]) == toupper(p[j]) && basename[j] != 0) j++;
        if (basename[j] == 0 && p[j] == 0) {    // case-insensitive basename match
          found = 1;
          for (modIdx = 0; modIdx < modCount; modIdx++) {
            if ((int)(modData[modIdx * MDAT_PER + MDAT_FLG] & 0x00FF) == fileIdx) {
              if (modOvlMod[modIdx] != OVL_MOD_MOD_NONE)
                fatalf("ParseOverlayDef: Module already assigned as an overlay: %s.",
                  modData[modIdx * MDAT_PER + MDAT_NAM]);
              modOvlMod[modIdx] = (byte)curMod;
              if (fdDebug != 0xffff)
                fprintf(fdDebug, "    OBJ %s -> module %u (%s)\n",
                  modData[modIdx * MDAT_PER + MDAT_NAM], curMod,
                  (char *)ovlModData[curMod * OVL_MOD_PER + OVLG_NAM]);
            }
          }
        }
      }
      if (!found) fatalf("ParseOverlayDef: OBJ not in linker inputs: %s.", p);
    }
  }
  safefclose(fd);
  if (ovlModCount == 0)
    printf("  WARNING: Overlay directive file has no modules defined.\n");
}

// ============================================================================
// P2_ForceInclude -- mark the module that defines <name> as included, even if
// no compiled object references it via EXTDEF.  Used to pull the overlay
// manager (__ovl_check and its dependencies) out of ovllib.lib; ylink calls
// them via generated stubs rather than through the normal resolution chain.
// ============================================================================
void P2_ForceInclude(char *name) {
  int pbdfIdx, modIndex;
  pbdfIdx = FindPubDef(name, 1);
  if (pbdfIdx == NOT_DEFINED)
    fatalf("P2_ForceInclude: %s not found. Link ovllib.lib for overlay support.", name);
  modIndex = pbdfData[pbdfIdx * PBDF_PER + PBDF_WHERE] / 256;
  modData[modIndex * MDAT_PER + MDAT_FLG] |= FlgInclude;
}

// ============================================================================
// OvlWarnMod -- scan one overlay OBJ's EXTDEF records and print a warning
// for each call into a different module that shares the same window.  Two
// modules sharing a window means the window contents must be swapped both on
// the way in and on the way out -- "thrashing".
//
// Called by WarnCrossWindow for every included overlay module.
// ============================================================================
void OvlWarnMod(uint modIdx, uint fd) {
  uint length, strlength;
  byte recType, deftype;
  int pbdfIdx, targetMod, targetMod, callerMod, targetWin, callerWin;
  callerMod = (int)(byte)modOvlMod[modIdx];
  while (1) {
    recType = read_u8(fd);
    if (feof(fd) || ferror(fd)) break;
    length = read_u16(fd);
    if (recType == MODEND) break;
    if (recType != EXTDEF) { forward(fd, length); continue; }
    while (length > 1) {
      strlength = readstrpre(line, fd);
      deftype = read_u8(fd);
      length -= (strlength + 1);
      pbdfIdx = FindPubDef(line, 0);
      if (pbdfIdx < 0) continue;
      targetMod = pbdfData[pbdfIdx * PBDF_PER + PBDF_WHERE] >> 8;
      targetMod = (int)(byte)modOvlMod[targetMod];
      if (targetMod == (int)OVL_MOD_MOD_NONE) continue; // target is resident
      if (callerMod == targetMod) continue;              // same module
      callerWin = (int)ovlModData[callerMod * OVL_MOD_PER + OVLG_WIN];
      targetWin = (int)ovlModData[targetMod * OVL_MOD_PER + OVLG_WIN];
      if (callerWin == targetWin) {
        printf("  WARNING: %s calls %s (module %s, same window %u) -- thrash!\n",
          (char *)modData[modIdx * MDAT_PER + MDAT_NAM],
          line,
          (char *)ovlModData[targetMod * OVL_MOD_PER + OVLG_NAM],
          (uint)ovlWinData[callerWin * OVL_WIN_PER + OVLW_ID]);
      }
    }
    read_u8(fd); // checksum
  }
}

// ============================================================================
// OvlRegStubs -- scan one OBJ's EXTDEF records and register a stub table
// entry for each unique call target that is an overlay function in a different
// module than the caller.  Resident-to-overlay calls also need stubs.
//
// Called by PassOvl for every included module (resident and overlay alike).
// ============================================================================
void OvlRegStubs(uint modIdx, uint fd) {
  uint length, strlength;
  byte recType, deftype;
  int pbdfIdx, targetMod, targetMod, callerMod, s, alreadyHave;
  callerMod = (int)(byte)modOvlMod[modIdx];
  while (1) {
    recType = read_u8(fd);
    if (feof(fd) || ferror(fd)) break;
    length = read_u16(fd);
    if (recType == MODEND) break;
    if (recType != EXTDEF) { forward(fd, length); continue; }
    while (length > 1) {
      strlength = readstrpre(line, fd);
      deftype = read_u8(fd);
      length -= (strlength + 1);
      pbdfIdx = FindPubDef(line, 0);
      if (pbdfIdx < 0) continue;
      targetMod = pbdfData[pbdfIdx * PBDF_PER + PBDF_WHERE] >> 8;
      targetMod = (int)(byte)modOvlMod[targetMod];
      if (targetMod == (int)OVL_MOD_MOD_NONE) continue; // target is resident
      if (callerMod == targetMod) continue;              // same module, no stub needed
      // Register a stub for this pubdef if not already present
      alreadyHave = 0;
      for (s = 0; s < ovlStubCount; s++) {
        if ((int)ovlStubData[s * OVL_STB_PER + OVLS_PBS] == pbdfIdx) {
          alreadyHave = 1; break;
        }
      }
      if (!alreadyHave) {
        if (ovlStubCount >= OVL_STB_MAX) fatal("PassOvl: Too many overlay stubs (max 256).");
        ovlStubData[ovlStubCount * OVL_STB_PER + OVLS_PBS] = (uint)pbdfIdx;
        ovlStubData[ovlStubCount * OVL_STB_PER + OVLS_COF] = 0; // offset assigned in Pass3
        ovlStubData[ovlStubCount * OVL_STB_PER + OVLS_MOD] = (uint)targetMod;
        ovlStubCount += 1;
      }
    }
    read_u8(fd); // checksum
  }
}

// ============================================================================
// WarnCrossWindow -- for every included overlay module, scan its EXTDEFs and
// warn if it calls another overlay module that shares the same window.
// ============================================================================
void WarnCrossWindow() {
  int i, mdatBase, mdatFile;
  uint fd;
  unsigned long offset;
  if (ovlModCount == 0) return;
  for (i = 0; i < modCount; i++) {
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) != FlgInclude) continue;
    if (modOvlMod[i] == OVL_MOD_MOD_NONE) continue; // only check overlay callers
    mdatFile = modData[mdatBase + MDAT_FLG] & 0x00ff;
    fd = safefopen(filePaths[mdatFile], "r");
    offset = (unsigned long)modData[mdatBase + MDAT_THD];
    if (bseek(fd, &offset, 0) == EOF)
      fatalf("WarnCrossWindow: Cannot seek in %s.", filePaths[mdatFile]);
    OvlWarnMod(i, fd);
    safefclose(fd);
  }
}

// ============================================================================
// PassOvl -- scan all included OBJs for cross-module EXTDEF references and
// register a stub table entry for each unique overlay call target.  After this
// pass, ovlStubCount is final; Pass3 uses it to reserve space in the resident
// code segment.
// ============================================================================
void PassOvl() {
  int i, mdatBase, mdatFile;
  uint fd;
  unsigned long offset;
  if (ovlModCount == 0) return;
  puts("  Pass Ovl (stub detection)");
  for (i = 0; i < modCount; i++) {
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) != FlgInclude) continue;
    mdatFile = modData[mdatBase + MDAT_FLG] & 0x00ff;
    fd = safefopen(filePaths[mdatFile], "r");
    offset = (unsigned long)modData[mdatBase + MDAT_THD];
    if (bseek(fd, &offset, 0) == EOF)
      fatalf("PassOvl: Cannot seek in %s.", filePaths[mdatFile]);
    OvlRegStubs(i, fd);
    safefclose(fd);
  }
  printf("  Overlay: %u module(s), %u window(s), %u stub(s)\n",
    ovlModCount, ovlWinCount, ovlStubCount);
}

// ============================================================================
// OvlLayoutSegments -- lay out all overlay memory regions after resident code
// and data have been placed by Pass3.  Updates segLengths[] and modData[].
//
// Steps:
//   1. Accumulate code size per module (aligned to SEG_ALIGNMENT).
//   2. Each window's size = max of its member module sizes.
//   3. Reserve stub space at the end of the resident code segment.
//   4. Assign each stub its CS offset (ovlStubBase + stub_index * OVL_STUB_BYTES).
//   5. Assign each window's base address (appended to the resident code segment).
//   6. Assign each module's code origin = its window's base.
//   7. Assign each overlay module's code origin within its module.
//   8. Append overlay module DATA sizes to segLengths[SEG_DATA].
// ============================================================================
void OvlLayoutSegments() {
  uint i, g, w, seglen, modAccum[OVL_MOD_MAX];
  long lcode, ldata;
  int mdatBase;
  if (ovlModCount == 0) return;

  // Step 1: accumulate per-module code sizes (MDAT_CSO still = raw code size)
  for (g = 0; g < (uint)ovlModCount; g++) {
    ovlModData[g * OVL_MOD_PER + OVLG_SIZ] = 0;
    modAccum[g] = 0;
  }
  for (i = 0; i < modCount; i++) {
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) != FlgInclude) continue;
    if (modOvlMod[i] == OVL_MOD_MOD_NONE) continue;
    g = modOvlMod[i];
    seglen = modData[mdatBase + MDAT_CSO];
    ovlModData[g * OVL_MOD_PER + OVLG_SIZ] += seglen;
    if (ovlModData[g * OVL_MOD_PER + OVLG_SIZ] % SEG_ALIGNMENT != 0)
      ovlModData[g * OVL_MOD_PER + OVLG_SIZ] +=
        SEG_ALIGNMENT - (ovlModData[g * OVL_MOD_PER + OVLG_SIZ] % SEG_ALIGNMENT);
  }

  // Step 2: each window's size = max of its member module sizes
  for (w = 0; w < (uint)ovlWinCount; w++) ovlWinData[w * OVL_WIN_PER + OVLW_SIZE] = 0;
  for (g = 0; g < (uint)ovlModCount; g++) {
    w = ovlModData[g * OVL_MOD_PER + OVLG_WIN];
    if (ovlModData[g * OVL_MOD_PER + OVLG_SIZ] > ovlWinData[w * OVL_WIN_PER + OVLW_SIZE])
      ovlWinData[w * OVL_WIN_PER + OVLW_SIZE] = ovlModData[g * OVL_MOD_PER + OVLG_SIZ];
  }

  // Step 3: reserve stub space in the resident code segment
  ovlStubBase = segLengths[SEG_CODE];
  lcode = (long)segLengths[SEG_CODE] + (long)ovlStubCount * (long)OVL_STUB_BYTES;
  if (lcode > 65535L) fatalfl("Code overflow adding stubs: ", lcode);
  segLengths[SEG_CODE] = (uint)lcode;

  // Step 4: assign each stub's CS offset
  for (g = 0; g < (uint)ovlStubCount; g++)
    ovlStubData[g * OVL_STB_PER + OVLS_COF] = ovlStubBase + g * OVL_STUB_BYTES;

  // Step 5: assign window base addresses (each window follows the previous)
  for (w = 0; w < (uint)ovlWinCount; w++) {
    ovlWinData[w * OVL_WIN_PER + OVLW_BASE] = segLengths[SEG_CODE];
    lcode = (long)segLengths[SEG_CODE] + (long)ovlWinData[w * OVL_WIN_PER + OVLW_SIZE];
    if (lcode > 65535L) fatalfl("Code overflow adding window: ", lcode);
    segLengths[SEG_CODE] = (uint)lcode;
  }

  // Step 6: each module's code origin = its window's base address
  for (g = 0; g < (uint)ovlModCount; g++) {
    w = ovlModData[g * OVL_MOD_PER + OVLG_WIN];
    ovlModData[g * OVL_MOD_PER + OVLG_ORG] = ovlWinData[w * OVL_WIN_PER + OVLW_BASE];
  }

  // Step 7: each overlay module's code origin = module origin + offset within module
  for (i = 0; i < modCount; i++) {
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) != FlgInclude) continue;
    if (modOvlMod[i] == OVL_MOD_MOD_NONE) continue;
    g = modOvlMod[i];
    seglen = modData[mdatBase + MDAT_CSO]; // raw code size (before we overwrite it)
    modData[mdatBase + MDAT_CSO] = ovlModData[g * OVL_MOD_PER + OVLG_ORG] + modAccum[g];
    modAccum[g] += seglen;
    if (modAccum[g] % SEG_ALIGNMENT != 0)
      modAccum[g] += SEG_ALIGNMENT - (modAccum[g] % SEG_ALIGNMENT);
  }

  // Pad CODE to a paragraph boundary so DS can be computed with integer division.
  // Overlay stubs and window sizes are not guaranteed multiples of 16, so the
  // final CODE size may not be paragraph-aligned after the overlay layout.
  if (segLengths[SEG_CODE] % 16 != 0)
    segLengths[SEG_CODE] += 16 - (segLengths[SEG_CODE] % 16);

  // Step 8: append overlay module DATA origins after all resident data
  for (i = 0; i < modCount; i++) {
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) != FlgInclude) continue;
    if (modOvlMod[i] == OVL_MOD_MOD_NONE) continue;
    seglen = modData[mdatBase + MDAT_DSO];
    ldata = (long)segLengths[SEG_DATA] + (long)seglen;
    if (ldata > 65535L) fatalfl("Data segment overflow: ", ldata);
    modData[mdatBase + MDAT_DSO] = segLengths[SEG_DATA];
    segLengths[SEG_DATA] = (uint)ldata;
    if (segLengths[SEG_DATA] % SEG_ALIGNMENT != 0)
      segLengths[SEG_DATA] += SEG_ALIGNMENT - (segLengths[SEG_DATA] % SEG_ALIGNMENT);
  }

  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "  OVL stubs at CODE=0x%x (%u stubs)\n", ovlStubBase, ovlStubCount);
    for (w = 0; w < (uint)ovlWinCount; w++) {
      fprintf(fdDebug, "  OVL window %u: base=0x%x size=0x%x\n",
        (uint)ovlWinData[w * OVL_WIN_PER + OVLW_ID],
        ovlWinData[w * OVL_WIN_PER + OVLW_BASE],
        ovlWinData[w * OVL_WIN_PER + OVLW_SIZE]);
    }
  }
}

// ============================================================================
// OvlGetWriteFd -- return the correct file descriptor for a data write.
// When the overlay temp file is active (ovlCodeFd != 0xffff) and the write
// targets the CODE segment, redirect to the temp file; otherwise use outfd.
// Called from P4_LEDATA, P4_LIDATA, P4_FixExt, and P4_FixSeg.
// ============================================================================
uint OvlGetWriteFd(uint outfd, byte segType) {
  if (ovlCodeFd != 0xffff && segType == SEG_CODE) return ovlCodeFd;
  return outfd;
}

// ============================================================================
// FindStub -- return the CS offset of the stub for pubdef index pbdfIdx,
// or NOT_DEFINED if no stub has been registered for that pubdef.
// ============================================================================
int FindStub(int pbdfIdx) {
  int s;
  for (s = 0; s < ovlStubCount; s++) {
    if ((int)ovlStubData[s * OVL_STB_PER + OVLS_PBS] == pbdfIdx)
      return (int)ovlStubData[s * OVL_STB_PER + OVLS_COF];
  }
  return NOT_DEFINED;
}

// ============================================================================
// OvlLookupStub -- check whether the pubdef at raw index rawPbdfIdx requires
// an overlay stub (i.e. its owning OBJ is in an overlay module).  If so,
// return the stub's CS offset; otherwise return NOT_DEFINED.
// Called from P4_FixExt to decide whether to redirect a fixup to a stub.
// ============================================================================
int OvlLookupStub(int rawPbdfIdx) {
  int targetMod;
  targetMod = pbdfData[rawPbdfIdx * PBDF_PER + PBDF_WHERE] >> 8;
  if (modOvlMod[targetMod] == OVL_MOD_MOD_NONE) return NOT_DEFINED;
  return FindStub(rawPbdfIdx);
}

// ============================================================================
// OvlEmitStubs -- write the 11-byte dispatch stub for every registered overlay
// call target into the stub area of the resident code segment.
//
// Stub layout (OVL_STUB_BYTES = 11):
//   68 lo hi   PUSH imm16   ; encoded = (win_idx << 8) | mod_idx
//   E8 lo hi   CALL rel16   ; near call to __ovl_check
//   58         POP AX
//   68 lo hi   PUSH imm16   ; virtual CS offset of the overlay function
//   C3         RET          ; __ovl_check will jump to the loaded function
//
// Called from Pass4 after all resident modules have been written.
// ============================================================================
void OvlEmitStubs(uint outfd) {
  int s, pbdfBase, targetMod, stubChkIdx, chkMod;
  uint chkOff, stubCodeOff, encoded, callRel, pushTarget;
  unsigned long stubFilePos;
  uint modIdx, winIdx;
  if (ovlStubCount == 0) return;
  // Resolve __ovl_check's final CS offset
  stubChkIdx = FindPubDef("__ovl_check", 0);
  if (stubChkIdx == NOT_DEFINED || stubChkIdx == NOT_INCLUDED)
    fatal("OvlEmitStubs: __ovl_check not found. Link ovllib.lib for overlay support.");
  chkMod = pbdfData[stubChkIdx * PBDF_PER + PBDF_WHERE] >> 8;
  chkOff = modData[chkMod * MDAT_PER + MDAT_CSO]
         + pbdfData[stubChkIdx * PBDF_PER + PBDF_ADDR];
  // Emit one stub per registered target
  for (s = 0; s < ovlStubCount; s++) {
    pbdfBase    = (int)ovlStubData[s * OVL_STB_PER + OVLS_PBS] * PBDF_PER;
    modIdx      = ovlStubData[s * OVL_STB_PER + OVLS_MOD];
    winIdx      = ovlModData[modIdx * OVL_MOD_PER + OVLG_WIN];
    stubCodeOff = ovlStubData[s * OVL_STB_PER + OVLS_COF];
    encoded     = (winIdx << 8) | modIdx;        // hi=window, lo=module (for __ovl_check)
    targetMod   = pbdfData[pbdfBase + PBDF_WHERE] >> 8;
    pushTarget  = modData[targetMod * MDAT_PER + MDAT_CSO]
                + pbdfData[pbdfBase + PBDF_ADDR]; // virtual CS offset of overlay function
    // CALL rel16: displacement = target - (stub_byte_6_addr)
    callRel = chkOff - (stubCodeOff + 6);
    stubFilePos = (unsigned long)EXE_HDR_LEN + (unsigned long)stubCodeOff;
    bseek(outfd, &stubFilePos, 0);
    write_f8(outfd, 0x68);                        // PUSH imm16
    write_f8(outfd, (char)(encoded & 0xFF));
    write_f8(outfd, (char)((encoded >> 8) & 0xFF));
    write_f8(outfd, 0xE8);                        // CALL near rel16
    write_f8(outfd, (char)(callRel & 0xFF));
    write_f8(outfd, (char)((callRel >> 8) & 0xFF));
    write_f8(outfd, 0x58);                        // POP AX
    write_f8(outfd, 0x68);                        // PUSH imm16 (overlay function offset)
    write_f8(outfd, (char)(pushTarget & 0xFF));
    write_f8(outfd, (char)((pushTarget >> 8) & 0xFF));
    write_f8(outfd, 0xC3);                        // RET
    if (fdDebug != 0xffff)
      fprintf(fdDebug, "  Stub %u at CS:0x%x -> %s CS:0x%x (enc=0x%x)\n",
        s, stubCodeOff,
        (char *)pbdfData[(int)ovlStubData[s * OVL_STB_PER + OVLS_PBS] * PBDF_PER + PBDF_NAME],
        pushTarget, encoded);
  }
}

// ============================================================================
// WriteOvlToc -- write one 8-byte TOC entry per module to outfd.
// Entry format: win_base(2) code_size(2) exe_off_lo(2) exe_off_hi(2)
// The overlay manager uses the TOC at runtime to find and load modules.
// ============================================================================
void WriteOvlToc(uint outfd, uint *modExeOffLo, uint *modExeOffHi) {
  int g;
  uint winIdx;
  for (g = 0; g < ovlModCount; g++) {
    winIdx = ovlModData[g * OVL_MOD_PER + OVLG_WIN];
    write_f16(outfd, ovlWinData[winIdx * OVL_WIN_PER + OVLW_BASE]);
    write_f16(outfd, ovlModData[g * OVL_MOD_PER + OVLG_SIZ]);
    write_f16(outfd, modExeOffLo[g]);
    write_f16(outfd, modExeOffHi[g]);
  }
}

// ============================================================================
// PatchOvlTocOff -- write the 32-bit file offset of the TOC into the
// __ovl_toc_off variable in the DATA segment so the overlay manager can
// find the TOC at runtime.  No-op if __ovl_toc_off is not linked.
// ============================================================================
void PatchOvlTocOff(uint outfd) {
  int pbdfIdx, modOfSym;
  unsigned long dataOff;
  pbdfIdx = FindPubDef("__ovl_toc_off", 0);
  if (pbdfIdx == NOT_DEFINED || pbdfIdx == NOT_INCLUDED) return;
  modOfSym = pbdfData[pbdfIdx * PBDF_PER + PBDF_WHERE] >> 8;
  dataOff = (unsigned long)EXE_HDR_LEN
           + (unsigned long)segLengths[SEG_CODE]
           + (unsigned long)modData[modOfSym * MDAT_PER + MDAT_DSO]
           + (unsigned long)(unsigned)pbdfData[pbdfIdx * PBDF_PER + PBDF_ADDR];
  bseek(outfd, &dataOff, 0);
  write_f16(outfd, ovlTocOffLo);
  write_f16(outfd, ovlTocOffHi);
}

// ============================================================================
// WriteOvlBlocks -- append all overlay module code blocks and the TOC to the
// EXE file, then patch __ovl_toc_off.  Called from main() after Pass4.
//
// Three phases:
//   Phase 1: Link each overlay module.  CODE bytes go to a temp file at the
//            module's virtual CS address; DATA bytes go directly to the EXE.
//   Phase 2: For each module, copy its code bytes from the temp file to the
//            end of the EXE.  Record each module's EXE file offset.
//   Phase 3: Write the TOC and patch __ovl_toc_off in the DATA segment.
// ============================================================================
void WriteOvlBlocks() {
  int i, g;
  uint fd, outfd, tmpFd;
  unsigned long tmp4, tocPos;
  uint codeBase[2], dataBase[2];
  uint modExeOffLo[OVL_MOD_MAX], modExeOffHi[OVL_MOD_MAX];
  uint modSiz, chunk, j, k;
  unsigned long srcPos, seekEnd;
  uint winIdx;
  unsigned long winBase;
  if (ovlModCount == 0) return;
  puts("  Writing overlay blocks");

  // Open the EXE for random-access DATA writes and sequential code appending.
  outfd = safefopen(pathOutput, "r+");

  // Process each overlay module independently so that modules sharing the same
  // window do not overwrite each other's code in the temp file.
  // For each module: (1) link its OBJs to a fresh OVLTMP.TMP, then
  // (2) append the resulting code bytes to the end of the EXE.
  for (g = 0; g < ovlModCount; g++) {
    winIdx  = ovlModData[g * OVL_MOD_PER + OVLG_WIN];
    winBase = (unsigned long)ovlWinData[winIdx * OVL_WIN_PER + OVLW_BASE];
    modSiz  = ovlModData[g * OVL_MOD_PER + OVLG_SIZ];

    // --- Phase 1: link module g's OBJs; CODE -> temp file, DATA -> EXE ---
    unlink("OVLTMP.TMP");
    tmpFd = safefopen("OVLTMP.TMP", "a");
    ovlCodeFd = tmpFd;            // redirect CODE writes to temp file
    for (i = 0; i < modCount; i++) {
      int mdatBase, mdatFile;
      unsigned long modOffset;
      mdatBase = i * MDAT_PER;
      if ((modData[mdatBase + MDAT_FLG] & FlgInclude) != FlgInclude) continue;
      if (modOvlMod[i] != g) continue;   // skip OBJs not in this module
      mdatFile = modData[mdatBase + MDAT_FLG] & 0x00ff;
      if (fdDebug != 0xffff)
        fprintf(fdDebug, "OvlLink %s CODE=0x%x DATA=0x%x\n",
          modData[mdatBase + MDAT_NAM],
          modData[mdatBase + MDAT_CSO], modData[mdatBase + MDAT_DSO]);
      // codeBase at the module's virtual CS origin (keeps fixup displacements correct)
      tmp4 = (unsigned long)EXE_HDR_LEN + (unsigned long)modData[mdatBase + MDAT_CSO];
      codeBase[0] = (uint)tmp4; codeBase[1] = (uint)(tmp4 >> 16);
      // dataBase at the module's DATA origin in the main EXE
      tmp4 = (unsigned long)EXE_HDR_LEN + (unsigned long)segLengths[SEG_CODE]
           + (unsigned long)modData[mdatBase + MDAT_DSO];
      dataBase[0] = (uint)tmp4; dataBase[1] = (uint)(tmp4 >> 16);
      fd = safefopen(filePaths[mdatFile], "r");
      modOffset = (unsigned long)modData[mdatBase + MDAT_THD];
      if (bseek(fd, &modOffset, 0) == EOF)
        fatalf("WriteOvlBlocks: Could not seek in %s.", filePaths[mdatFile]);
      extNext = 0;
      extCount = 0;
      P4_DoMod(fd, outfd, codeBase, dataBase);
      safefclose(fd);
    }
    ovlCodeFd = 0xffff;           // deactivate redirect before closing
    safefclose(tmpFd);

    // --- Phase 2: append module g's code block to the end of the EXE --------
    tmpFd = safefopen("OVLTMP.TMP", "r");
    seekEnd = 0L;
    bseek(outfd, &seekEnd, 2);    // seek outfd to end of file
    btell(outfd, &tmp4);
    modExeOffLo[g] = (uint)tmp4;
    modExeOffHi[g] = (uint)(tmp4 >> 16);
    if (fdDebug != 0xffff)
      fprintf(fdDebug, "  OVL module %u at EXE offset 0x%x%04x size=%u\n",
        g, modExeOffHi[g], modExeOffLo[g], modSiz);
    srcPos = (unsigned long)EXE_HDR_LEN + winBase;
    bseek(tmpFd, &srcPos, 0);
    j = modSiz;
    while (j > 0) {
      chunk = (j < (uint)LINESIZE) ? j : (uint)LINESIZE;
      for (k = 0; k < chunk; k++) line[k] = read_u8(tmpFd);
      for (k = 0; k < chunk; k++) write_f8(outfd, line[k]);
      j -= chunk;
    }
    safefclose(tmpFd);
    unlink("OVLTMP.TMP");
  }

  // --- Phase 3: write TOC and patch __ovl_toc_off ---------------------------
  // outfd is positioned at end of file (after the last module's code block).
  btell(outfd, &tocPos);
  ovlTocOffLo = (uint)tocPos;
  ovlTocOffHi = (uint)(tocPos >> 16);
  WriteOvlToc(outfd, modExeOffLo, modExeOffHi);
  safefclose(outfd);
  outfd = safefopen(pathOutput, "r+");
  PatchOvlTocOff(outfd);
  safefclose(outfd);

  if (fdDebug != 0xffff)
    fprintf(fdDebug, "  OVL TOC at 0x%x%04x (%u modules)\n",
      ovlTocOffHi, ovlTocOffLo, ovlModCount);
}
