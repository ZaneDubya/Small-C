/******************************************************************************
* ylink - the ypsilon linker (c) 2017 Zane Wagner. All rights reserved.
******************************************************************************/
#include "stdio.h"
#include "notice.h"
#include "link.h"

// --- Output variables -------------------------------------------------------
char *pathOutput; // path to the file used for writing output exe/lib file.
char *pathDebug; // path to file used for debug output.
char *pathLibInput; // path to file used as library object file input.
uint fdDebug; // fd to which we will output debug information. can be stdout
// --- DOS exe data -----------------------------------------------------------
#define EXE_HDR_LEN 512
#define RELOC_COUNT 8
#define RELOC_PER 1
int *relocData;
int relocCount;
int exeStartAddress;
// --- 'Line' (variable used when reading strings) ----------------------------
#define LINEMAX  127
#define LINESIZE 128
char *line;
// --- File paths - these are the files that are read in by the linker --------
#define FILE_MAX 96
int *filePaths; // ptrs to input file paths, including library files.
int fileCount; // equal to the number of input files.
// --- ModData - information about the modules loaded for linking -------------
#define MOD_MAX 128
#define MDAT_NAM 0
#define MDAT_CSO 1 // Before P3: code seg length, After P3: code seg origin
#define MDAT_DSO 2 // Before P3: data seg length, After P3: data seg origin
#define MDAT_THD 3 // offset to THEADR in file
#define MDAT_FLG 4 // flags / file offset
#define MDAT_PER 5
#define FlgInLib 0x0100
#define FlgStart 0x0200
#define FlgStack 0x0400
#define FlgInclude 0x800
int *modData; // each obj has name ptr, 2 fields for seg
              // origin, theadr offset in file, and flag.
              // flag is: 0x00ff file index (max 256 files), and 
              // 0x0f00 flags: 0x0100 in_lib, 0x0200 start, 0x0400 stack
int modCount; // incremented by 1 for each obj and library module in exe.
// --- ListNames - temporary LNAMES data (reloaded for each module) -----------
#define LNAMES_CNT 4
#define LNAME_NULL 0xFF
#define LNAME_CODE 0x00
#define LNAME_DATA 0x01
#define LNAME_STACK 0x02
byte *locNames;
// --- Segment Data -----------------------------------------------------------
#define SEGS_CNT 3
#define SEG_CODE 0
#define SEG_DATA 1
#define SEG_STACK 2
#define SEG_NOTPRESENT 0xFF
uint *segLengths; // length of segdefs, index is seg_xxx
byte *locSegs; // seg_xxxs in this module, in order they were defined
int segIndex; // index of next segdef that will be defined
// --- Public Definitions -----------------------------------------------------
#define PBDF_CNT 640
#define PBDF_NAME 0   // ptr to name of pubdef
#define PBDF_WHERE 1  // hi: index of module where it is located
                      // low: index of segment where it is located
#define PBDF_ADDR 2   // offset in segment (+module origin) where it is located
#define PBDF_PER 3
int *pbdfData;
int pbdfCount;
// --- ExtDef buffers ---------------------------------------------------------
#define EXTBUF_LEN 1536 // these variables serve two different purposes.
char *extBuffer; // In Pass2, list of all unresolved extdefs; 
int extCount, extNext; // In Pass4, list of extdefs in this module.
#define NOT_INCLUDED -2 // this extdef is defined but not included
#define NOT_DEFINED -1 // this extdef is not defined

main(int argc, int *argv) {
  int i;
  puts(VERSION);
  AllocAll();
  RdArgs(argc, argv);
  Initialize();
  Pass1();
  if (IsLibrary()) {
    Pass2Lib();
  }
  else {
    Pass2();
    Pass3();
    Pass4();
    if (fdDebug != 0xffff) {
      fprintf(fdDebug, "Code: %u b Data: %u b, Stack: %u b\n",
        segLengths[SEG_CODE], segLengths[SEG_DATA], segLengths[SEG_STACK]);
    }
  }
  DeAllocAll();
  return 0;
}

AllocAll() {
  line = AllocMem(LINESIZE, 1);
  filePaths = AllocMem(FILE_MAX, 2);
  modData = AllocMem(MDAT_PER * MOD_MAX, 2);
  locNames = AllocMem(LNAMES_CNT, 1);
  segLengths = AllocMem(SEGS_CNT, 2);
  locSegs = AllocMem(SEGS_CNT, 1);
  pbdfData = AllocMem(PBDF_PER * PBDF_CNT, 2);
  extBuffer = AllocMem(EXTBUF_LEN, 1);
  relocData = AllocMem(RELOC_PER * RELOC_COUNT, 2);
  pathOutput = 0;
  pathDebug = 0;
  pathLibInput = 0;
  fileCount = 0;
  fdDebug = 0xffff;
}

DeAllocAll() {
  // need to write this.
  if (fdDebug != 0xffff) {
    safefclose(fdDebug);
  }
}

Initialize() {
  int i, fd, length;
  char c;
  if (pathOutput == 0) {
    printf("  No -e parameter. Output file will be out.exe.\n");
    pathOutput = AllocMem(8, 1);
    strcpy(pathOutput, "out.exe");
  }
  unlink(pathOutput); // delete
  for (i = 0; i < SEGS_CNT; i++) {
    segLengths[i] = 0;
  }
  // pathDebug = "debug.txt";
  if (pathDebug != 0) {
    unlink(pathDebug);
    fdDebug = safefopen(pathDebug, "a");
  }
  // copy in library files
  if (pathLibInput != 0) {
    fd = safefopen(pathLibInput, "r");
    while(1) {
      if (feof(fd)) {
        break;
      }
      readstr(line, LINEMAX, fd);
      length = strlen(line);
      filePaths[fileCount] = AllocMem(length + 1, 1);
      strcpy(filePaths[fileCount], line);
      fileCount += 1;
    }
  }
  exeStartAddress = 0xffff;
}

RdArgs(int argc, int *argv) {
  int i;
  if (argc == 1) {
    fatal("No argments passed.");
  }
  for (i = 1; i < argc; i++) {
    char* c;
    getarg(i, line, LINESIZE, argc, argv);
    c = line;
    if (*c == '-') {
      // option -x=xxx
      if (*(c+2) != '=') {
        fatalf("Missing '=' in option %s", c);
      }
      switch (*(c+1)) {
        case 'e': // e=output file
          RdArgExe(c+3);
          break;
        case 'd': // d=debug file
          RdArgDebug(c+3);
          break;
        case 'l': // l=library file with input file list of object files
          RdArgLibrary(c+3);
          break;
        default:
          fatalf("Could not parse option %s", c);
          break;
      }
    }
    else {
      // read in object files
      char *start, *end;
      start = end = c;
      while (1) {
        if ((*end == 0) || (*end == ',')) {
          RdArgObj(start, end);
          if (*end == 0) {
            break;
          }
          start = ++end;
        }
        end++;
      }
      c = end;
    }
  }
}

RdArgObj(char *start, char *end) {
  char *path;
  if (fileCount == FILE_MAX) {
    fatalf("max of %u input files.", FILE_MAX);
  }
  filePaths[fileCount] = AllocMem(end - start + 1, 1);
  path = filePaths[fileCount];
  while (start != end) {
    *path++ = *start++;
  }
  *path = 0;
  fileCount += 1;
}

RdArgExe(char *str) {
  pathOutput = AllocMem(strlen(str) + 1, 1);
  strcpy(pathOutput, str);
}

RdArgDebug(char *str) {
  pathDebug = AllocMem(strlen(str) + 1, 1);
  strcpy(pathDebug, str);
}

RdArgLibrary(char *str) {
  pathLibInput = AllocMem(strlen(str) + 1, 1);
  strcpy(pathLibInput, str);
}

// ============================================================================
// === Pass1 ==================================================================
// ============================================================================
// For each object file,
// 1. get object file description and save in objdefs,
// 2. for each segment, get name, create seg def (if necessary), set origins,
// 3. for each pubdef, add to pubdefs.
byte libInLib; // if 1, we are reading a library obj.

Pass1() {
  uint i, fd;
  puts("  Pass 1");
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "Pass 1:\n");
  }
  modCount = 0;
  pbdfCount = 0;
  for (i = 0; i < fileCount; i++) {
    fd = safefopen(filePaths[i], "r");
    if (fdDebug != 0xffff) {
      fprintf(fdDebug, "  Reading %s\n", filePaths[i]);
    }
    P1_RdFile(i, fd);
    safefclose(fd);
  }
}

P1_RdFile(uint fileIndex, uint fd) {
  uint length;
  byte recType;
  while (1) {
    recType = read_u8(fd);
    if (feof(fd) || ferror(fd)) {
      break;
    }
    length = read_u16(fd);
    P1_DoRecord(fileIndex, recType, length, fd);
  }
}

P1_DoRecord(uint fileIndex, byte recType, uint length, uint fd) {
  switch (recType) {
    case THEADR:
      P1_THEADR(fileIndex, length, fd);
      ResetSegments();
      break;
    case MODEND:
      P1_MODEND(length, fd);
      break;
    case LNAMES:
      P1_LNAMES(length, fd);
      break;
    case PUBDEF:
      P1_PUBDEF(length, fd);
      break;
    case SEGDEF:
      P1_SEGDEF(length, fd, 1);
      break;
    case LIBHDR:
      P1_LIBHDR(length, fd);
      break;
    case LIBEND:
      P1_LIBEND(length, fd);
      break;
    case COMMNT:
    case EXTDEF:
    case FIXUPP:
    case LEDATA:
    case LIDATA:
    case LIBDEP:
    case LIBEND:
      forward(fd, length);
      break;
    default:
      fatalf("DoRec: Unknown record of type %x. Exiting.", recType);
      break;
  }
}

// 80H THEADR Translator Header Record
// The THEADR record contains the name of the object module.  This name
// identifies an object module within an object library or in messages produced
// by the linker. The name string indicates the full path and filename of the
// file that contained the source code for the module.
// This record, or an LHEADR record must occur as the first object record.
// More than one header record is allowed (as a result of an object bind, or if
// the source arose from multiple files as a result of include processing).
// 82H is handled identically, but indicates the name of a module within a
// library file, which has an internal organization different from that of an
// object module.
P1_THEADR(uint fileIndex, uint length, uint fd) {
  int i;
  if (modCount == MOD_MAX) {
    fatalf("Error: max of %u object modules.", MOD_MAX);
  }
  AddModule(fileIndex, fd);
  read_u8(fd); // checksum. assume correct.
}

AddModule(uint fileIndex, uint fd) {
  int length, i;
  char *path, *c;
  uint offset[2];
  // get offset to this THEADR record. We only handle files < 65kb in size.
  btell(fd, offset);
  if (offset[1] > 0) {
    fatal("Could not load module from file; file larger than 65kb.");
  }
  // rewind offset 3 bytes to beginning of header file.
  offset[0] -= 3;
  // copy the module name and set up data.
  length = readstrpre(line, fd);
  c = line;
  path = modData[modCount * MDAT_PER + MDAT_NAM] = AllocMem(length, 1);
  for (i = 0; i < length; i++) {
    *path++ = *c++;
  }
  *path = 0;
  // set the module data.
  modData[modCount * MDAT_PER + MDAT_CSO] = 0;
  modData[modCount * MDAT_PER + MDAT_DSO] = 0;
  modData[modCount * MDAT_PER + MDAT_THD] = offset[0];
  modData[modCount * MDAT_PER + MDAT_FLG] = fileIndex & 0x00ff;
  if (fileIndex > 0xff) { // sanity
    fatal("AddModule: MDAT_FLG cannot store file index greater than 255.");
  }
  if (libInLib) {
    modData[modCount * MDAT_PER + MDAT_FLG] |= FlgInLib;
  }
}

ResetSegments() {
  int i;
  // reset segments to not present, first segment index will be 0
  for (i = 0; i < SEGS_CNT; i++) {
    locSegs[i] = SEG_NOTPRESENT;
  }
  segIndex = 0;
}

// 8AH MODEND Module End Record
// The MODEND record denotes the end of an object module. It also indicates
// whether the object module contains the main routine in a program, and it
// can optionally contain a reference to a program's entry point.
// If moduletype & 0x80, the module is a main program module. I don't use this.
// if moduletype & 0x40, the module contains a start address. I use this.
P1_MODEND(uint length, uint fd) {
  byte moduletype;
  moduletype = read_u8(fd);
  if (moduletype & 0x40) {
    // Is set if the module contains a start address; if this bit is set, the
    // field starting with the End Data byte is present and specifies the start
    // address.
    uint offset;
    byte frmType, tgtType, frame, target;
    length -= rd_fix_target(length, fd, &offset, &frmType, &tgtType, &frame, &target);
    modData[modCount * MDAT_PER + MDAT_FLG] |= FlgStart;
    exeStartAddress = offset;
  }
  // include all .obj files, excluding all lib mods for now.
  // lib mods will be included where necessary to resolve extdefs.
  if ((modData[modCount * MDAT_PER + MDAT_FLG] & FlgInLib) == 0) {
    modData[modCount * MDAT_PER + MDAT_FLG] |= FlgInclude;
  }
  read_u8(fd); // checksum. assume correct.
  // MODEND ends with a buffer to the end of the paragraph.
  ClearPara(fd);
  modCount += 1;
}

// 96H LNAMES List of Names Record
// The LNAMES record is a list of names that can be referenced by subsequent
// SEGDEF and GRPDEF records in the object module. The names are ordered by
// occurrence and referenced by index from subsequent records.  More than one
// LNAMES record may appear.  The names themselves are used as segment, class,
// group, overlay, and selector names.
P1_LNAMES(uint length, uint fd) {
  int nameIndex;
  char *c;
  nameIndex = 0;
  while (length > 1) {
    if (nameIndex == LNAMES_CNT) {
      fatalf("Error: P1_LNAMES max of %u local names.", LNAMES_CNT);
    }
    length -= readstrpre(line, fd); // line = local name at index nameIndex.
    if (strcmp(line, "") == 0) {
      locNames[nameIndex] = LNAME_NULL;
    }
    else if (strcmp(line, "CODE") == 0) {
      locNames[nameIndex] = LNAME_CODE;
    }
    else if (strcmp(line, "DATA") == 0) {
      locNames[nameIndex] = LNAME_DATA;
    }
    else if (strcmp(line, "STACK") == 0) {
      locNames[nameIndex] = LNAME_STACK;
    }
    else {
      fatalf("Error: P1_LNAMES does not handle name %s.", line);
    }
    nameIndex++;
  }
  while (nameIndex < LNAMES_CNT) {
    locNames[nameIndex++] = LNAME_NULL;
  }
  read_u8(fd); // checksum. assume correct.
}

// 90H PUBDEF Public Names Definition Record
// The PUBDEF record contains a list of public names.  It makes items defined
// in this object module available to satisfy external references in other
// modules with which it is bound or linked. The symbols are also available
// for export if so indicated in an EXPDEF comment record.
P1_PUBDEF(uint length, uint fd) {  byte typeindex;
  int i, puboffset;
  byte basegroup, basesegment, typeindex;
  char *pdbfname, *c;
  uint puboffset;
  int alreadyDefined;
  // BaseGroup and BaseSegment fields contain indexes specifying previously
  // defined SEGDEF and GRPDEF records.  The group index may be 0, meaning
  // that no group is associated with this PUBDEF record.
  // BaseFrame field is present only if BaseSegment field is 0, but the
  // contents of BaseFrame field are ignored.
  // BaseSegment idx is normally nonzero and no BaseFrame field is present.
  basegroup = read_u8(fd); // we don't use this value
  if (basegroup != 0) {
    fatal("PUBDEF: BaseGroup must be 0.");
  }
  basesegment = read_u8(fd);
  if (basesegment == 0) {
    fatal("P1_PUBDEF: BaseSegment must be nonzero.");
  }
  length -= 2;
  while (length > 1) {
    int namelen;
    namelen = readstrpre(line, fd);
    if (pbdfCount >= PBDF_CNT) {
      fatalf2("Could not add %s to pubdefs, max of %u.", line, PBDF_CNT);
    }
    alreadyDefined = FindPubDef(line, 0);
    if (alreadyDefined != NOT_DEFINED) { // works for included and not included
      int otherMod;
      otherMod = pbdfData[alreadyDefined * PBDF_PER + PBDF_WHERE] >> 8;
      printf("Duplicate pubdef of '%s' in modules %s and %s.",
        line,
        modData[MDAT_PER * otherMod + MDAT_NAM],
        modData[MDAT_PER * modCount + MDAT_NAM]);
      fatal("Duplicate pubdefs not allowed.");
    }
    puboffset = read_u16(fd);
    typeindex = read_u8(fd); // we don't use this value
    if (typeindex != 0) {
      fatal("PUBDEF: Type is not 0. ");
    }
    c = line;
    pdbfname = pbdfData[pbdfCount * PBDF_PER + PBDF_NAME] = AllocMem(namelen, 1);
    for (i = 0; i < namelen; i++) {
      *pdbfname++ = *c++;
    }
    *pdbfname = 0;
    // set the module data.
    pbdfData[pbdfCount * PBDF_PER + PBDF_WHERE] = locSegs[basesegment - 1];
    pbdfData[pbdfCount * PBDF_PER + PBDF_WHERE] |= (modCount << 8);
    pbdfData[pbdfCount * PBDF_PER + PBDF_ADDR] = puboffset;
    length -= namelen + 3;
    pbdfCount += 1;
  }
  read_u8(fd); // checksum. assume correct.
}

// 98H SEGDEF Segment Definition Record
// The SEGDEF record describes a logical segment in an object module. It
// defines the segment's name, length, and alignment, and the way the segment
// can be combined with other logical segments at bind, link, or load time.
// Object records that follow a SEGDEF record can refer to it to identify a
// particular segment.  The SEGDEF records are ordered by occurrence, and are
// referenced by segment indexes (starting from 1) in subsequent records.
// When 'pass1' is 1, this routine will set modData and segLengths. When
//    1 or 0, this routine will set locSegs.
P1_SEGDEF(uint length, uint fd, uint pass1) {
  byte segattr, segname, classname;
  uint seglen;
  // segment attribute
  segattr = read_u8(fd);
  if ((segattr & 0xe0) != 0x60) {
    fatal("SEGDEF: Unknown segment attribute field (must be 0x60).");
  }
  if (((segattr & 0x1c) != 0x08) && ((segattr & 0x1c) != 0x14)) {
    // 0x08 = Public. Combine by appending at an offset that meets the alignment requirement.
    // 0x14 = Stack. Combine as for C=2. This combine type forces byte alignment.
    fatalf("SEGDEF: Unknown combine %x (req: 0x08 or 0x14).", segattr & 0x1c);
  }
  if ((segattr & 0x02) != 0x00) {
    fatal("SEGDEF: Attribute may not be big (flag 0x02).");
  }
  if ((segattr & 0x01) != 0x00) {
    fatal("SEGDEF: Attribute must be 16-bit addressing (flag 0x01).");
  }
  seglen = read_u16(fd);
  segname = read_u8(fd);
  classname = read_u8(fd); // ClassName is not used by SmallAsm.
  read_u8(fd); // The linker ignores the Overlay Name field.
  read_u8(fd); // checksum. assume correct.;
  segname = locNames[segname - 1];
  classname = locNames[classname - 1];
  if (classname != LNAME_NULL) {
    fatalf("SEGDEF: ClassName must be null; LNAME == %x", classname);
  }
  if (segname == LNAME_CODE) {
    if (pass1 == 1) {
      modData[modCount * MDAT_PER + MDAT_CSO] = seglen;
    }
    locSegs[segIndex] = SEG_CODE;
  }
  else if (segname == LNAME_DATA) {
    if (pass1 == 1) {
      modData[modCount * MDAT_PER + MDAT_DSO] = seglen;
    }
    locSegs[segIndex] = SEG_DATA;
  }
  else if (segname == LNAME_STACK) {
    if (pass1 == 1) {
      modData[modCount * MDAT_PER + MDAT_FLG] |= FlgStack;
      segLengths[SEG_STACK] += seglen;
    }
    locSegs[segIndex] = SEG_STACK;
  }
  else {
    fatalf("SEGDEF: Linker does not handle LNAME of %x", segname);
  }
  segIndex += 1;
}

// Note: I've removed code to load library dictionary and dependancy records.
// The library dictionary contains a reference to every pubdef in every object
// in the library. The library depndancy lists the internal dependancies
// between object modules in the library. These would be incredibly useful
// if we were working with multiple libraries. For small programs, I am 
// ignoring them. Code to load these is in the v4 codebase between btell and
// bseek.
P1_LIBHDR(uint length, uint fd) {
  byte flags, nextRecord;
  uint dictOffset[2], blockCount;
  dictOffset[0] = read_u16(fd);
  dictOffset[1] = read_u16(fd);
  blockCount = read_u16(fd);
  flags = read_u8(fd); // 0x01 - is case sensitive. others 0.
  clearsilent(length - 8, fd); // rest of record is zeroes.
  read_u8(fd); // checksum. assume correct.
  libInLib = 1;
}

// Note: Following the LIBEND is the library dictionary and dependancy records.
// I don't use this information, and while it can be calculated on the fly, 
// it would be tremendously useful if we we loading multiple libraries and/or
// did not have the memory to hold all the dependancy information.
// In any case, library files generated by YLINK, for now, will end after the
// LIBEND record, and will not contain dictionary or dependancy records.
P1_LIBEND(uint length, uint fd) {
  if (!libInLib) {
    fatal("LIBEND: not a library!", 0);
  }
  clearsilent(length, fd);
  safefclose(fd);
  libInLib = 0;
}

IncSegLengthsToNextParagraph() {
  int i, next;
  for (i = 0; i < SEGS_CNT; i++) {
    next = segLengths[i] % 16;
    if (next != 0) {
      segLengths[i] += (16 - next);
    }
  }
}

// ============================================================================
// === Pass2 ==================================================================
// ============================================================================
// In Pass2, we are only making sure that all required EXTDEFs are matched by
// an existing PUBDEF. If not, then we will attempt to find a matching PUBDEF
// in an available LIBRARY file. If we can't, then error out. We will do this
// recursively until all EXTDEFs are resolved.
// If we are building a .lib file, then we can skip this step.
Pass2() {
  uint i, fd, unresolved, extLast;
  uint offset[2];
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "Pass 2: Match all extdefs to pubdefs.\n");
  }
  puts("  Pass 2");
  unresolved = 1;
  while (unresolved == 1) {
    // reset extdef buffer
    extNext = 0;
    extCount = 0;
    // foreach mod: if included, open file containing mod, forward to mod data.
    // collect all unresolved extdefs in that module.
    if (fdDebug != 0xffff) {
      fprintf(fdDebug, "  Resolving EXTDEFS in ");
    }
    for (i = 0; i < modCount; i++) {
      int mdatBase, mdatFile;
      mdatBase = i * MDAT_PER;
      if ((modData[mdatBase + MDAT_FLG] & FlgInclude) == FlgInclude) {
        mdatFile = modData[mdatBase + MDAT_FLG] & 0x00ff;
        if (fdDebug != 0xffff) {
          fprintf(fdDebug, "%s, ", modData[mdatBase + MDAT_NAME]);
        }
        fd = safefopen(filePaths[mdatFile], "r");
        offset[0] = modData[mdatBase + MDAT_THD];
        offset[1] = 0;
        if (bseek(fd, offset, 0) == EOF) {
          fatalf("Could not seek to position %u, file too short.", offset[0]);
        }
        P2_DoMod(fd);
        safefclose(fd);
      }
    }
    if (fdDebug != 0xffff) {
      fprintf(fdDebug, "\n  %u unresolved.\n", extCount);
    }
    if (extCount == 0) {
      unresolved = 0;
    }
    else {
      P2_Resolve();
    }
  }
}

P2_DoMod(uint fd) {
  uint length;
  byte recType;
  while (1) {
    recType = read_u8(fd);
    if (feof(fd) || ferror(fd)) {
      break;
    }
    length = read_u16(fd);
    if (recType == EXTDEF) {
      P2_EXTDEF(length, fd);
    }
    else if (recType == MODEND) {
      break;
    }
    else {
      // Although LIBEND requires special handling (close instead of forward),
      // in a good lib file, we will always hit MODEND before LIBEND.
      forward(fd, length);
    }
  }
}

// 8CH EXTDEF External Names Definition Record
// The EXTDEF record contains a list of symbolic external references—that is,
// references to symbols defined in other object modules. The linker resolves
// external references by matching the symbols declared in EXTDEF records
// with symbols declared in PUBDEF records.
P2_EXTDEF(uint length, uint fd) {
  byte deftype, strlength;
  int i, pbdfIdx;
  while (length > 1) {
    strlength = readstrpre(line, fd);
    deftype = read_u8(fd);
    length -= (strlength + 1);
    if (deftype != 0) {
      fatalf2("EXTDEF: Type of %x is %u, must by type 0. ", line, deftype);
    }
    pbdfIdx = MatchName(line, extBuffer, EXTBUF_LEN);
    if (pbdfIdx != NOT_DEFINED) {
      continue;
    }
    pbdfIdx = FindPubDef(line, 0);
    if (pbdfIdx == NOT_DEFINED) {
      fatalf("P2_EXTDEF: No pubdef matches %s.", line);
    }
    else if (pbdfIdx == NOT_INCLUDED) {
      AddName(line, extBuffer, &extNext, EXTBUF_LEN);
      extCount += 1;
    }
  }
  read_u8(fd); // checksum. assume correct.
}

P2_Resolve() {
  int i, extName, pbdfIdx, modIndex;
  // for each unincluded extdef, find the matching module and include it!
  for (i = 0; i < extCount; i++) {
    extName = GetName(i, extBuffer, EXTBUF_LEN);
    pbdfIdx = FindPubDef(extName, 1);
    if (pbdfIdx >= 0) {
      modIndex = pbdfData[pbdfIdx * PBDF_PER + PBDF_WHERE] / 256;
      modData[modIndex * MDAT_PER + MDAT_FLG] |= FlgInclude;
    }
    else {
      fatalf("P2_Resolve: Could not resolve pubdef for %s.", extName);
    }
  }
}

// ============================================================================
// === Pass3 ==================================================================
// ============================================================================
// Get segment lengths and code/data origins for each included module.
// Then set exeStartAddress
Pass3() {
  // set code segment origins as in v5.P1_SEGDEF
  uint i, seglen;
  puts("  Pass 3");
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "Pass 3: Setting segment origins.\n");
  }
  for (i = 0; i < modCount; i++) {
    int mdatBase, mdatFile;
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) == FlgInclude) {
      // set code origin for this segment in this module
      seglen = modData[mdatBase + MDAT_CSO];
      modData[mdatBase + MDAT_CSO] = segLengths[SEG_CODE];
      segLengths[SEG_CODE] += seglen;
      // set data origin for this segment in this module
      seglen = modData[mdatBase + MDAT_DSO];
      modData[mdatBase + MDAT_DSO] = segLengths[SEG_DATA];
      segLengths[SEG_DATA] += seglen;
      if ((modData[mdatBase + MDAT_FLG] & FlgStart) == FlgStart) {
        exeStartAddress += modData[mdatBase + MDAT_CSO];
      }
    }
    IncSegLengthsToNextParagraph();
  }
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "  CODE=%x\n  DATA=%x\n", 
      segLengths[SEG_CODE],
      segLengths[SEG_DATA]);
  }
}

// ============================================================================
// === Pass4 ==================================================================
// ============================================================================
// In Pass4, we are copying in the DATA from the modules, and handling all
// FIXUPP records.
Pass4() {
  uint i, fd, outfd, checksum;
  uint modOffset[2]; // offset to object module that we are currently reading
  uint codeBase[2]; // offset to code segment in output file.
  uint dataBase[2]; // offset to data segment in output file.
  relocCount = 0;
  puts("  Pass 4");
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "Pass 4:\n");
  }
  outfd = safefopen(pathOutput, "a");
  for (i = 0; i < modCount; i++) {
    int mdatBase, mdatFile;
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) == FlgInclude) {
      mdatFile = modData[mdatBase + MDAT_FLG] & 0x00ff;
      if (fdDebug != 0xffff) {
        fprintf(fdDebug, "Linking %s (%s) CODE=0x%x DATA=0x%x \n", 
          modData[mdatBase + MDAT_NAM], filePaths[mdatFile],
          modData[mdatBase + MDAT_CSO], modData[mdatBase + MDAT_DSO]);
      }
      fd = safefopen(filePaths[mdatFile], "r");
      // seek to module in input object file:
      modOffset[0] = modData[mdatBase + MDAT_THD];
      modOffset[1] = 0;
      if (bseek(fd, modOffset, 0) == EOF) {
        fatalf("Could not seek to position %u, file too short.", modOffset[0]);
      }
      // get base code and base data offsets for the output file:
      codeBase[0] = EXE_HDR_LEN + modData[mdatBase + MDAT_CSO];
      codeBase[1] = 0;
      dataBase[0] = EXE_HDR_LEN + segLengths[SEG_CODE];
      dataBase[1] = 0;
      Add1632(modData[mdatBase + MDAT_DSO], dataBase);
      // reset extdef buffer
      extNext = 0;
      extCount = 0;
      // do the mod!
      P4_DoMod(fd, outfd, codeBase, dataBase);
      safefclose(fd);
    }
  }
  safefclose(outfd);
  WriteExeHeader(0);
  checksum = CalcChecksum();
  WriteExeHeader(checksum);
}

CalcChecksum() {
  uint i, j, fd;
  i = 0;
  fd = safefopen(pathOutput, "r");
  while(1) {
    j = read_u16(fd);
    if (feof(fd)) {
      break;
    }
    i += j;
  }
  safefclose(fd);
  i = 0 - i - 1;
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "Checksum=%x\n", i, j);
  }
  return i;
}

WriteExeHeader(uint checksum) {
  uint fd;
  uint beginning[2], totalsize[2];
  uint blockcount, lastblock, rvSS, i;
  fd = safefopen(pathOutput, "r+");
  if (exeStartAddress == 0xffff) {
    fatal("No start address specified.");
  }
  beginning[0] = beginning[1] = 0;
  totalsize[0] = totalsize[1] = 0;
  Add1632(segLengths[0], totalsize);
  Add1632(segLengths[1], totalsize);
  Add1632(segLengths[2], totalsize);
  lastblock = totalsize[0] % 512;
  blockcount = totalsize[1] * 128 + totalsize[0] / 512 + 1;
  if (lastblock != 0) {
    blockcount += 1;
  }
  rvSS = (segLengths[SEG_CODE] / 16 + segLengths[SEG_DATA] / 16);
  bseek(fd, beginning, 0);
  fputs("MZ", fd); // 00: "MZ"
  write_f16(fd, lastblock); // 02: count of bytes in last 512b block
  write_f16(fd, blockcount); // 04: count of 512b blocks in file
  write_f16(fd, 0x0004); // 06: relocation entry count.
  write_f16(fd, 0x0020); // 08: size of header in paras (0x20 * 0x10 = 0x200).
  write_f16(fd, 0x0000); // 0A: paras mem needed
  write_f16(fd, 0xffff); // 0C: paras mem wanted
  write_f16(fd, rvSS); // 0E: Relative value of the stack segment.
  write_f16(fd, segLengths[SEG_STACK]); // 10: Initial value of the SP register.
  write_f16(fd, checksum); // 12: Word checksum. Does not need to be filled in.
  write_f16(fd, exeStartAddress); // 14: Initial value of the IP register.
  write_f16(fd, 0x0000); // 16: Initial value of the CS register.
  write_f16(fd, 0x001E); // 18: Offset of the first relocation item in the file.
  write_f16(fd, 0x0000); // 1A: Overlay number. 0x0000 = main program.
  write_f16(fd, 0x0001); // 1C: ??? Not in spec, but always 0x0001.
  for (i = 0; i < relocCount; i++) {
    write_f16(fd, relocData[i]);
    write_f16(fd, 0x0000);
  }
  safefclose(fd);
}

P4_DoMod(uint fd, uint outfd, uint codeBase[], uint dataBase[]) {
  uint length, segOffset;
  byte recType, segType;
  ResetSegments();
  segType = SEG_NOTPRESENT; // in case we hit a fixupp before a data record
  while (1) {
    recType = read_u8(fd);
    if (feof(fd) || ferror(fd)) {
        fatal("P4_DoMod: Unexpected file termination.\n");
    }
    length = read_u16(fd);
    switch (recType) {
      case LNAMES:
        P1_LNAMES(length, fd); // restore names of the segments in this module
        break;
      case SEGDEF:
        // restore locSegs so we know the names of segments in this module
        P1_SEGDEF(length, fd, 0);
        break;
      case EXTDEF:
        // read into buffer
        P4_EXTDEF(length, fd);
        break;
      case LEDATA:
        P4_LEDATA(length, fd, outfd, codeBase, dataBase, &segType, &segOffset);
        break;
      case LIDATA:
        P4_LIDATA(length, fd, outfd, codeBase, dataBase, &segType, &segOffset);
        break;
      case FIXUPP:
        P4_FIXUPP(length, fd, outfd, codeBase, dataBase, segType, segOffset);
        segType = SEG_NOTPRESENT; // only one fixupp per data record allowed
        break;
      case MODEND:  
        // exit reading module
        return;
      case THEADR: 
      case PUBDEF:
      case COMMNT:
        forward(fd, length);
        break;
      default:
        fatal("P4_DoMod: Unknown record of type %u.\n", recType);
        break;
    }
  }
}

// Read all the extdefs for this module into a buffer.
// extdef names are null-terminated.
P4_EXTDEF(uint length, uint fd) {
  byte deftype;
  uint strlength;
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "  EXTDEF\n");
  }
  while (length > 1) {
    strlength = readstrpre(line, fd);
    deftype = read_u8(fd);
    length -= (strlength + 1);
    AddName(line, extBuffer, &extNext, EXTBUF_LEN);
    extCount += 1;
  }
  read_u8(fd); // checksum. assume correct.
}

P4_LEDATA(uint length, uint fd, uint outfd, uint codeBase[], uint dataBase[],
  byte *segType, int *segOffset) {
  uint segBase[2];
  uint i;
  length -= 4; // length includes segment type, offset, and checksum
  *segType = read_u8(fd);
  *segOffset = read_u16(fd);
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "  LEDATA SegIdx=%x Offs=%x Len=%x\n", 
      *segType, *segOffset, length);
  }
  if (*segType == 0 || *segType > SEGS_CNT) {
    fatalf("P4_LEDATA: segType of %u, must be between 1 and 3.", *segType);
  }
  *segType = locSegs[*segType - 1]; // transform to local segment index.
  P4_SetBase(*segType, codeBase, dataBase, segBase);
  Add1632(*segOffset, segBase); // segBase[0] += *segOffset;
  bseek(outfd, segBase, 0);
  for (i = 0; i < length; i++) {
    write_f8(outfd, read_u8(fd));
  }
  read_u8(fd); // checksum. assume correct.
}

// A2H LIDATA Logical Iterated Data Record
// Like the LEDATA record, the LIDATA record contains binary data—executable
// code or program data. The data in an LIDATA record, however, is specified
// as a repeating pattern (iterated), rather than by explicit enumeration.
// The data in an LIDATA record can be modified by the linker if the LIDATA
// record is followed by a FIXUPP record, although this is not recommended.
P4_LIDATA(uint length, uint fd, uint outfd, uint codeBase[], uint dataBase[],
  byte *segType, int *segOffset) {
  uint segBase[2];
  uint repeat, count;
  length -= 4; // length includes segment type, offset, and checksum
  *segType = read_u8(fd);
  *segOffset = read_u16(fd);
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "  LIDATA SegIdx=%x Offs=%x Len=%x\n", 
      *segType, *segOffset, length);
  }
  if (*segType == 0 || *segType > SEGS_CNT) {
    fatalf("P4_LIDATA: segType of %u, must be between 1 and 3.", *segType);
  }
  *segType = locSegs[*segType - 1]; // transform to local segment index.
  P4_SetBase(*segType, codeBase, dataBase, segBase);
  Add1632(*segOffset, segBase); // segBase[0] += *segOffset;
  bseek(outfd, segBase, 0);
  while (length > 1) {
    repeat = read_u16(fd); // number of times the Content field will repeat. 
    count = read_u16(fd); // determines interpretation of the Content field:
    length -= 4;
    // 0  Indicates that the Content field that follows is a 1-byte count value  
    //    followed by count data bytes. Data bytes will be mapped to memory, 
    //    repeated as many times as are specified in the Repeat Count field.
    // !0 Indicates that the Content field that follows is composed of one or
    //    more Data Block fields. The value in the Block Count field specifies
    //    the number of Data Block fields (recursive definition).
    if (fdDebug != 0xffff) {
      fprintf(fdDebug, "    Repeat=%x Blocks=%x\n", repeat, count);
    }
    if (count == 0) {
      P4_LIINNER(fd, outfd, repeat, &length);
    }
    else if (count == 1) {
      // count is times to repeat inner blocks.
      // I am just going to fudge this so it can handle asm.lib. It needs a 
      // rewrite to handle this recursively
      repeat = read_u16(fd);
      count = read_u16(fd);
      length -= 4;
      if (count == 0x0000) {
        P4_LIINNER(fd, outfd, repeat, &length);
      }
    }
    else {
      int i;
      if (fdDebug != 0xffff) {
        byte xxx;
        write_x16(fdDebug, repeat);
        write_x16(fdDebug, count);
        for (i = 0; i < length; i++) {
          xxx = read_u8(fd);
          write_x8(fdDebug, xxx);
        }
      }
      fatalf("P4_LIDATA: Block Count of %u, must be 0. ", count);
    }
  }
  read_u8(fd); // checksum. assume correct.
}

P4_LIINNER(uint fd, uint outfd, uint repeat, uint *length) {
  int i, j;
  uint count;
  count = read_u8(fd);
  *length -= 1 + count;
  if (count > LINESIZE) {
    // we COULD handle block size of 256 if necessary...
    fatalf("P4_LIINNER: Block Size of %u, must be 128 or less. ", count);
  }
  for (i = 0; i < count; i++) {
    line[i] = read_u8(fd);
  }
  for (i = 0; i < repeat; i++) {
    for (j = 0; j < count; j++) {
      write_f8(outfd, line[j]);
    }
  }
}

// 9CH FIXUPP Fixup Record
// The FIXUPP record contains information that allows the linker to resolve
// (fix up) and eventually relocate references between object modules. FIXUPP
// records describe the LOCATION of each address value to be fixed up, the
// TARGET address to which the fixup refers, and the FRAME relative to which
// the address computation is performed.
// Each subrecord in a FIXUPP object record either defines a thread for
// subsequent use, or refers to a data location in the nearest previous LEDATA
// or LIDATA record. The high-order bit of the subrecord determines the
// subrecord type: if the high-order bit is 0, the subrecord is a THREAD
// subrecord; if the high-order bit is 1, the subrecord is a FIXUP subrecord.
// Subrecords of different types can be mixed within one object record.
// Information that determines how to resolve a reference can be specified
// explicitly in a FIXUP subrecord, or it can be specified within a FIXUP
// subrecord by a reference to a previous THREAD subrecord. A THREAD subrecord
// describes only the method to be used by the linker to refer to a particular
// target or frame. Because the same THREAD subrecord can be referenced in
// several subsequent FIXUP subrecords, a FIXUPP object record that uses THREAD
// subrecords may be smaller than one in which THREAD subrecords are not used.
P4_FIXUPP(uint length, uint fd, uint outfd, uint codeBase[], uint dataBase[],
  byte segType, int segOffset) {
  byte lLocat;    // 0x80 set: this is a fixup (unset: a thread, not handled)
                  // 0x40 unset: Self-relative, set: segment-relative
  byte lRefType;  // type of reference
  uint lOffset;   // location offset
  byte tFType, tTType, tFrame, tTarget;
  byte tFrame;    // frame index
  byte tTarget;   // target index
  uint tOffset;   // target offset (when indicated by target type, 0 otherwise)
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "  FIXUPP\n");
  }
  while (length > 1) {
    length = rd_fix_locat(length, fd, &lOffset, &lLocat, &lRefType);
    // --- fixupp reference ---------------------------------------------------
    if ((lLocat & 0x80) == 0) {
      fatalf("P4_FIXUPP: must be fixup, not thread (locat=%x).", lLocat);
    }
    if (fdDebug != 0xffff) {
      if ((lLocat & 0x40) == 0) {
        // fixup is rel to where fixup occurs
        fprintf(fdDebug, "    Rel=IP ");
      }
      else {
        // fixup is rel to segment beginning
        fprintf(fdDebug, "    Rel=Sgmt "); 
      }
      if (lRefType == 1) {
        // make fixup at offset from beginning of data in preceding record
        fprintf(fdDebug, "Off=0x%x ", lOffset); 
      }
      else if (lRefType == 2) {
        // make fixup at offset from beginning of segment 
        fprintf(fdDebug, "Off=Seg+0x%x ", lOffset);
      }
    }
    if (lRefType != 1 && lRefType != 2) {
      fatalf("P4_FIXUPP: unhandled reference type %x.", lRefType);
    }
    // --- frame/target -------------------------------------------------------
    // tFixdata is two 4-bit fields, frame and target.
    // ffff.... is FRAME: 0x00: segment index, 0x20: external index
    //    A FRAME INDEX sets the index of the specified type.
    // ....tttt is TARGET: 0x00: seg+offs, 0x02: ext+offs, 0x06: ext only
    //    A TARGET INDEX sets the index of the specified type.
    //    A TARGET OFFSET specifies the offset from the location of the target
    //    for 0x00 or 0x02. For 0x06, an offset of zero is implied.
    length = rd_fix_target(length, fd, &tOffset, &tFType, &tTType, &tFrame, &tTarget);
    // --- do the fix up! -----------------------------------------------------
    if (tFType == 0x00) {
      // frame given by a segment index
      if ((tTType != 4) && (tTType != 0)) {
        // This is an ext: target MUST be 2 (ext only) or 6 (ext + offset).
        fatalf("P4_FIXUPP: Unhandled target type %u in seg.", tTType);
      }
      P4_FixCheckMatch(tFrame, tTarget);
      P4_FixSeg(outfd, lLocat, lRefType, lOffset, tFrame, tOffset,
        codeBase, dataBase, segType, segOffset);
      /*else if (tTType == 0x02 || tTType == 0x06) {
        // Mixed! Frame is segment, but the target is an external reference.
        if (fdDebug != 0xffff) {
          fprintf(fdDebug, " <Mixed Frm=%x/%x Tgt=%x/%x>", 
            tFType, tFrame, tTType, tTarget);
        }
        P4_FixMix(outfd, lLocat, lRefType, lOffset, tFrame, tTarget, tOffset,
          codeBase, dataBase, segType, segOffset);
      }*/
    }
    else if (tFType == 0x02) {
      // frame given by an external index
      P4_FixCheckMatch(tFrame, tTarget);
      if ((tTType != 6) && (tTType != 2)) {
        // This is an ext: target MUST be 2 (ext only) or 6 (ext + offset).
        fatalf("P4_FIXUPP: Unhandled target type %u in ext.", tTType);
      }
      P4_FixExt(outfd, lLocat, lRefType, lOffset, tFrame, tOffset,
        codeBase, dataBase, segType, segOffset);
    }
    else {
      fatalf("P4_FIXUPP: unhandled frame type of %x.", tFType);
    }
  }
  read_u8(fd); // checksum. assume correct.
}

P4_FixCheckMatch(byte tFrame, byte tTarget) {
  if (tFrame != tTarget) {
    fatalf2("P4_FIXUPP: Segment frame %x and target %x must match.",
      tFrame, tTarget);
  }
}

P4_FixExt(uint outfd, byte lLocat, byte lRefType, uint lOffset, byte fixExt,
  uint fixOffset, uint codeBase[], uint dataBase[], byte segType,
  int segOffset) {
  int extName, pbdfIndex, modOrigin;
  byte segOfExt, modOfExt;
  if (lRefType != 1) {
    fatalf("P4_FixExt: Unhandled ref type &u.", lRefType);
  }
  if (segType != SEG_CODE) {
    fatal("P4_FixExt: Segment type must be CODE");
  }
  if (fixExt > extCount) {
    fatalf2("P4_FixExt: Ext index of %u is greater than ext count of %u.", 
      fixExt, extCount);
  }
  extName = GetName(fixExt - 1, extBuffer, EXTBUF_LEN);
  if (extName == NOT_DEFINED) {
    fatalf("P4_FixExt: Could not find ext index %u.", fixExt);
  }
  pbdfIndex = FindPubDef(extName, 0);
  if (pbdfIndex == NOT_DEFINED) {
    fatalf("P4_FixExt: No pubdef defined matching extdef %s.", extName);
  }
  else if (pbdfIndex == NOT_INCLUDED) {
    // this can probably be removed - would error out in P2.
    fatalf("P4_FixExt: Unincluded pubdef matches %s.", extName);
  }
  pbdfIndex = pbdfIndex * PBDF_PER;
  segOfExt = pbdfData[pbdfIndex + PBDF_WHERE] & 0x00ff;
  modOfExt = pbdfData[pbdfIndex + PBDF_WHERE] >> 8;
  modOrigin = (pbdfData[pbdfIndex + PBDF_WHERE] >> 8) * MDAT_PER;
  switch (segOfExt) {
    case SEG_CODE:
      modOrigin = modData[modOrigin + MDAT_CSO];
      break;
    case SEG_DATA:
      modOrigin = modData[modOrigin + MDAT_DSO];
      break;
    default:
      fatalf("P4_FixExt: Ext is in unhandled seg of index %x", segOfExt);
      break;
  }
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "Tgt=Ext%x (%s; Seg=0x%x Mod=%s+0x%x)\n", 
      fixExt, extName, segOfExt, modData[(modOfExt) * MDAT_PER + MDAT_NAM],
      pbdfData[pbdfIndex + PBDF_ADDR]);
  }
  if ((lLocat & 0x40) == 0) {
    // IP-relative.
    P4_DoFixupp(outfd, modOrigin, pbdfData[pbdfIndex + PBDF_ADDR] + fixOffset, 1, 
      codeBase[0], lOffset + segOffset);
  }
  else {
    // relative to beginning of segment.
    P4_DoFixupp(outfd, modOrigin, pbdfData[pbdfIndex + PBDF_ADDR] + fixOffset, 0, 
      codeBase[0], lOffset + segOffset);
  }
}

P4_FixSeg(uint outfd, byte lLocat, byte lRefType, uint lOffset, byte fixSeg,
  uint fixOffset, uint codeBase[], uint dataBase[], byte segType,
  int segOffset) {
  uint segBase[2];
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "Tgt=Seg%u+0x%x\n", fixSeg, fixOffset);
  }
  fixSeg -= 1;
  P4_SetBase(fixSeg, codeBase, dataBase, segBase);
  if ((lLocat & 0x40) == 0) {
    // IP-relative.
    if (fixSeg != segType) {
      // Make sure we're in the same segment (this SHOULD be the case, because
      // we're fixing up code, and code is always in the same segment in the 
      // NEAR model that SmallC uses.
      fatalf2("P4_FixSeg: IP-rel, seg %u must equal seg %u.\n", fixSeg, segType);
    }
    if (lRefType == 1) {
      // 16-bit offset
      P4_DoFixupp(outfd, fixOffset - segOffset - lOffset - 2, 0, 0,
        codeBase[0], lOffset + segOffset);
    }
    else {
      // 16-bit logical segment base
      fatal("P4_FixSeg: Unhandled segment base ref in IP-relative fixupp.");
    }
  }
  else {
    // relative to beginning of a segment.
    if (lRefType == 1) {
      // 16-bit offset
      uint whereBase;
      switch (locSegs[fixSeg]) {
        case SEG_CODE:
          whereBase = codeBase[0] - EXE_HDR_LEN;
          break;
        case SEG_DATA:
          whereBase = dataBase[0] - EXE_HDR_LEN - segLengths[SEG_CODE];
          break;
        default:
          fatalf("P4_FixSeg: Unhandled relative seg base index %u.", fixSeg);
      }
      P4_DoFixupp(outfd, whereBase, fixOffset, 0,
        codeBase[0], lOffset + segOffset);
    }
    else if (lRefType == 2) {
      // 16-bit logical segment base
      uint logBase;
      switch (locSegs[fixSeg]) {
        case SEG_CODE:
          logBase = 0;
          break;
        case SEG_DATA:
          logBase = segLengths[0] / 16;
          break;
        case SEG_STACK:
          logBase = (segLengths[0] + segLengths[1]) / 16;
          break;
        default:
          fatalf2("P4_FixSeg: Unhandled logical segment base index %u, %u",
            fixSeg, locSegs[fixSeg]);
      }
      P4_DoFixupp(outfd, logBase, 0, 0, codeBase[0], lOffset + segOffset);
      relocData[relocCount++] = codeBase[0] + lOffset + segOffset - EXE_HDR_LEN;
    }
  }
}

P4_FixMix(uint outfd, byte lLocat, byte lRefType, uint lOffset, byte frmSeg,
  byte frmExt, uint fixOffset, uint codeBase[], uint dataBase[], byte segType,
  int segOffset) {
  // adjust the fixOffset here.
  P4_FixExt(outfd, lLocat, lRefType, lOffset, frmExt, fixOffset,
        codeBase, dataBase, segType, segOffset);
}

P4_DoFixupp(uint fd, uint what, uint whatOff, uint whatRelative, 
  uint where, uint whereOff) {
  uint outAddr[2];
  uint nowAddr[2];
  uint offset;
  if (whatRelative == 1) {
    offset = where + whereOff + 2 - EXE_HDR_LEN;
  }
  else {
    offset = 0;
  }
  if (fdDebug != 0xffff) {
    fprintf(fdDebug, "    WRITE 0x%x at 0x%x\n", 
      what + whatOff - offset, 
      where + whereOff);
  }
  btell(fd, nowAddr);
  outAddr[0] = where + whereOff;
  outAddr[1] = 0;
  bseek(fd, outAddr, 0);
  write_f16(fd, what + whatOff - offset);
  bseek(fd, nowAddr);
  if (what + whatOff - offset == 0xa84) {
    // abort(1);
  }
}

P4_SetBase(byte segType, uint codeBase[], uint dataBase[], uint segBase[]) {
  if (segType == SEG_CODE) {
    segBase[0] = codeBase[0];
    segBase[1] = codeBase[1];
  }
  else if (segType == SEG_DATA) {
    segBase[0] = dataBase[0];
    segBase[1] = dataBase[1];
  }
  else if (segType == SEG_STACK) {
    // this is why we can only have one stack segment.
    segBase[0] = EXE_HDR_LEN + segLengths[SEG_CODE];
    segBase[1] = 0;
    Add1632(segLengths[SEG_DATA], segBase);
  }
  else {
    fatalf("P4_XXDATA: Local SegType of %u, must be 0 or 1. ", segType);
  }
}

TestAdd() {
  uint a, b[2];
  a = 8000;
  b[0] = 60000;
  b[1] = 1;
  printf("Add: %u + [%u %u]\n", a, b[0], b[1]);
  Add1632(a, b);
  printf("Add: %u + [%u %u]\n", a, b[0], b[1]);
  abort(1);
}

Add1632(uint a, uint b[]) {
  if ((a > 0) && (a > 0xffff - b[0])) { /* `a + x` would overflow */
    b[1] += 1;
  }
  b[0] += a;
}

// ============================================================================
// === Pass2 (Library files) ==================================================
// ============================================================================
// If we are reading in a library file, then we just concat the library file!
Pass2Lib() {
  uint fd, fdmod, i;
  uint modOffset[2]; // offset to object module that we are currently reading
  if (fdDebug != 0) {
    fprintf(fdDebug, "Pass 2: Build library file.\n");
  }
  puts("  Pass 2 (Library file)");
  fd = safefopen(pathOutput, "a");
  P2L_LIBHDR(fd);
  for (i = 0; i < modCount; i++) {
    int mdatBase, mdatFile;
    mdatBase = i * MDAT_PER;
    if ((modData[mdatBase + MDAT_FLG] & FlgInclude) == FlgInclude) {
      mdatFile = modData[mdatBase + MDAT_FLG] & 0x00ff;
      if (fdDebug != 0xffff) {
        fprintf(fdDebug, "Adding %s (%s) to library... \n", 
          modData[mdatBase + MDAT_NAM], filePaths[mdatFile]);
      }
      fdmod = safefopen(filePaths[mdatFile], "r");
      // seek to module in input object file:
      modOffset[0] = modData[mdatBase + MDAT_THD];
      modOffset[1] = 0;
      if (bseek(fdmod, modOffset, 0) == EOF) {
        fatalf("Could not seek to position %u, file too short.", modOffset[0]);
      }
      P2L_ADD(fd, fdmod);
      safefclose(fdmod);
    }
  }
  P2L_LIBEND(fd);
  safefclose(fd);
}

P2L_LIBHDR(uint fd) {
  int i;
  write_f8(fd, LIBHDR);
  write_f16(fd, 0x000d); // length of record
  write_f16(fd, 0x0000); // low offset to lib dict
  write_f16(fd, 0x0000); // hi offset to lib dict
  write_f16(fd, 0x0000); // count blocks in lib dict.
  write_f8(fd, 0x00); // flags.
  for (i = 0; i < 5; i++) {
    write_f8(fd, 0x00); // fill rest of record with zeros.
  }
  write_f8(fd, 0x00); // checksum
}

P2L_LIBEND(uint fd) {
  int i;
  write_f8(fd, LIBEND);
  write_f16(fd, 0x000d); // length of record
  for (i = 0; i < 12; i++) {
    write_f8(fd, 0x00); // fill rest of record with zeros.
  }
  write_f8(fd, 0x00); // checksum
}

P2L_ADD(uint fdout, uint fdmod) {
  uint length, i;
  byte recType, c;
  while (1) {
    recType = read_u8(fdmod);
    if (feof(fdmod) || ferror(fdmod)) {
      fatal("Unexpected EOF in file.");
    }
    length = read_u16(fdmod);
    write_f8(fdout, recType);
    write_f16(fdout, length);
    for (i = 0; i < length; i++) {
      c = read_u8(fdmod);
      write_f8(fdout, c);
    }
    if (recType == MODEND) {
      int remaining;
      remaining = 16 - (ctellc(fdout) % 16);
      if (remaining != 16) {
        for (i = 0; i < remaining; i++) {
          write_f8(fdout, 0x00);
        }
      }
      break;
    }
  }
}

// ============================================================================
// === Common Routines ========================================================
// ============================================================================

// Returns index of pubdef with this name, error code for not defined/present.
// if retAllDefined == 1, return the index regardless of whether it is 
// included or not included
FindPubDef(char* name, int retAllDefined) {
  int i, j, length, modIndex;
  char* matching;
  length = strlen(name);
  for (i = 0; i < pbdfCount; i++) {
    // look for matching pubdef name
    matching = pbdfData[i * PBDF_PER + PBDF_NAME];
    for (j = 0; j <= length; j++) {
      if (name[j] == 0 && matching[j] == 0) {
        modIndex = pbdfData[i * PBDF_PER + PBDF_WHERE] / 256;
        if ((modData[modIndex * MDAT_PER + MDAT_FLG] & FlgInclude) == 0) {
          if (retAllDefined) {
            return i;
          }
          else {
            return NOT_INCLUDED;
          }
        }
        return i;
      }
      if (toupper(name[j]) != toupper(matching[j])) {
        break;
      }
    }
  }
  return NOT_DEFINED;
}

// rd_fix_target: Reads the target location of a fixupp.
//    Returns the count of bytes remaining in the record.
rd_fix_target(uint length, uint fd, uint *offset, byte *frmType, byte *tgtType,
  byte *frame, byte *target) {
  // -----------------------------------------------------------------
  //  FRAME/TARGET METHODS is a byte which encodes the methods by which 
  //  the fixup "frame" and "target" are to be determined, as follows.
  //  In the first three cases  FRAME  INDEX  is  present  in  the
  //  record  and  contains an index of the specified type. In the
  //  last two cases there is no FRAME INDEX field.
  byte methods;
  methods = read_u8(fd); // Format is fffftttt
  *frmType = methods >> 4;
  *tgtType = methods & 0x0f;
  length -= 1;
  // I have omitted frame/target methods that SmallC22/SmallA do not emit.
  switch (*frmType) {
    case 0x00:     // frame given by a segment index
    case 0x02:     // frame given by an external index
      *frame = read_u8(fd);
      length -= 1;
      if (*frame >= 0x80) {
        if (*frame == 0x80) {
          *frame = read_u8(fd);
          length -= 1;
        }
        else {
          fatalf("rd_fix_target: unhandled extended frame of 0x%x.", *frame);
        }
      }
      break;
    default:
      fatalf2("rd_fix_target: unhandled frame method 0x%x (0x%x)", 
      *frmType, methods);
      break;
  }
  // -----------------------------------------------------------------
  //  The TARGET bits tell LINK to determine the target of  the
  //  reference in one of the following ways. In each case TARGET INDEX
  //  is present in the record and contains an index of the indicated
  //  type. In the first three cases TARGET OFFSET is present and
  //  specifies the offset from the location of the segment, group, 
  //  or external address to the target. In the last three cases there
  //  is no TARGET OFFSET because an offset of zero is assumed.
  *target = read_u8(fd);
  length -= 1;
  if (*target >= 0x80) {
    if (*target == 0x80) {
      *target = read_u8(fd);
      length -= 1;
    }
    else {
      fatalf("rd_fix_target: unhandled extended target of 0x%x.", *frame);
    }
  }
  // As with frame methods, I have omitted targets SmallC22/SmallA do not emit.
  switch (*tgtType) {
    case 0x00 :  // target given by a segment index + displacement
    case 0x02 :  // target given by an external index + displacement
      *offset = read_u16(fd);
      length -= 2;
      break;
    case 0x04 :  // target given by a segment index alone, no displacement.
    case 0x06 :  // target given by an external index alone
      *offset = 0;
      break;
    default:
      printf("d_fix_target: unhandled target method %x.", *tgtType);
      break;
  }
  return length;
}

// rd_fix_locat: reads the place where the fixupp will be written.
//    Returns the count of bytes remaining in the record.
rd_fix_locat(uint length, uint fd, uint *offset, byte *locat, byte *lRefType) {
  // -----------------------------------------------------------------
  // The first bit (in the low byte) is always  one  to  indicate
  // that this block defines a "fixup" as opposed to a "thread."
  //  The REFERENCE MODE bit (0x40) indicates how the reference is made.
  //  * Self-relative references locate a target address relative to
  //    the CPU's instruction pointer (IP); that is, the target is a
  //    certain distance from the location currently indicated by
  //    IP. This sort of reference is common to the jump
  //    instructions. Such a fixup is not necessary unless the
  //    reference is to a different segment.
  //  * Segment-relative references locate a target address in any segment 
  //    relative to the beginning of the segment. This is just the
  //    "displacement" field that occurs in so many instructions.
  *locat = read_u8(fd);
  // -----------------------------------------------------------------
  // The TYPE REFERENCE bits (called the LOC  bits  in  Microsoft
  // documentation) encode the type of reference.
  *lRefType = (*locat & 0x3c) >> 2; // 4 bit field.
  // -----------------------------------------------------------------
  //  The DATA RECORD OFFSET subfield specifies the offset, within
  //  the preceding data record, to the reference. Since a  record
  //  can  have at most 1024 data bytes, 10 bits are sufficient to
  //  locate any reference.
  *offset = read_u8(fd) + ((*locat & 0x03) << 8);
  length -= 2;
  return length;
}

// === Program State ==========================================================
// returns 1 if we are building a library file, 0 otherwise.
IsLibrary() {
  if (pathLibInput != 0) {
    return 1;
  }
  return 0;
}

// === Write Hexidecimal numbers ==============================================

write_x16(uint fd, uint value) {
  write_x8(fd, value >> 8);
  write_x8(fd, value & 0x00ff);
}

write_x8(uint fd, byte value) {
  byte ch0;
  ch0 = (value & 0xf0) >> 4;
  if (ch0 < 10) {
    ch0 += 48;
  }
  else {
    ch0 += 55;
  }
  fputc(ch0, fd);
  ch0 = (value & 0x0f);
  if (ch0 < 10) {
    ch0 += 48;
  }
  else {
    ch0 += 55;
  }
  fputc(ch0, fd);
}

write_f8(uint fd, char value) {
  int c0, c1;
  c0 = (value & 0x00ff);
  _write(c0, fd);
}

write_f16(uint fd, uint value) {
  int c0, c1;
  c0 = (value & 0x00ff);
  c1 = (value >> 8);
  _write(c0, fd);
  _write(c1, fd);
}

// === Binary Reading Routines ================================================
read_u8(uint fd) {
  byte ch;
  ch = _read(fd);
  return ch;
}

read_u16(uint fd) {
  uint i;
  i = (_read(fd) & 0x00ff);
  i += (_read(fd) & 0x00ff) << 8;
  return i;
}

// read string that is prefixed by length.
readstrpre(char* str, uint fd) {
  byte length, retval;
  char* next;
  next = str;
  retval = length = read_u8(fd);
  while (length > 0) {
      *next++ = read_u8(fd);
      length--;
  }
  *next++ = NULL;
  return retval + 1;
}

// reimplementation of fgets. reads until eof, \n, or \r.
readstr(char* str, uint size, uint fd) {
  int backup; char *next;
  next = str;
  while(--size > 0) {
    switch (*next = fgetc(fd)) {
      case  EOF: 
        *next = NULL;
        if(next == str) {
          return NULL;
        }
        return str;
      case '\r':
      case '\n':
        *next = NULL;
        return str;
      default:
        ++next;
        break;
    }
  }
  *next = NULL;
  return str;
}

// === File Management ========================================================

safefopen(char* path, char* opts) {
  uint fd;
  if (!(fd = fopen(path, opts))) {
    fatalf("Could not open %s.", path);
  }
  return fd;
}

safefclose(uint fd) {
  if (fd != 0) {
    fclose(fd);
  }
}

offsetfd(uint fd, uint base[], uint offset[]) {
    bseek(fd, base, 0);
    bseek(fd, offset, 1);
}

forward(uint fd, uint offset) {
  uint iOffset[2];
  iOffset[0] = offset;
  iOffset[1] = 0;
  bseek(fd, iOffset, 1);
}

ClearPara(uint fd) {
  if (!feof(fd)) {
    int remaining;
    remaining = 16 - (ctellc(fd) % 16);
    if (remaining != 16) {
      clearsilent(remaining, fd);
    }
  }
}

clearsilent(uint length, uint fd) {
  while (length-- > 0) {
    read_u8(fd);
  }
}

// === Error Routines =========================================================

fatal(char *str) {
  fputs("  Error: ", stderr);
  fputs(str, stderr);
  fputc(NEWLINE, stderr);
  abort(1);
}

fatalf(char *format, char *arg) {
  fputs("  Error: ", stderr); 
  fprintf(stderr, format, arg);
  fputc(NEWLINE, stderr);
  abort(1);
}
  
fatalf2(char *format, char *arg0, char *arg1) {
  fputs("  Error: ", stderr); 
  fprintf(stderr, format, arg0, arg1);
  fputc(NEWLINE, stderr);
  abort(1);
}

// === Memory Management ======================================================

AllocMem(int nItems, int itemSize) {
  int result;
  result = calloc(nItems, itemSize);
  if (result == 0) {
    printf("Could not allocate mem: %u x %u\n", nItems, itemSize);
    abort(0);
  }
  return result;
}

// AddName: adds a null-terminated string to a byte array. Returns ptr to
// byte array where string was placed. Fails if string will not fit.
AddName(char *name, char *names, uint *next, uint max) {
  char* namesaved;
  uint nameat, i;
  nameat = i = *next;
  namesaved = name;
  while (*name != 0) {
    names[i++] = *name++;
    if (i >= max) {
      fprintf(stderr, "\n%s at 0x%x exceeded names length of 0x%x.\n", 
              namesaved, nameat, max, i);
      abort(1);
    }
  }
  names[i++] = 0;
  *next = i;
  return names + nameat;
}

GetName(int index, char *names, uint max) {
  uint i, start, count;
  i = start = count = 0;
  while (i < max) {
    if (count == index) {
      return names + start;
    }
    if (names[i] == 0) {
      start = i + 1;
      count += 1;
    }
    i++;
  }
  return NOT_DEFINED;
}

MatchName(char* name, char *names, uint max) {
  uint i, j;
  for (i = 0; i < max; i++) {
    if (names[i] != 0) {
      j = 0;
      while (toupper(names[i]) == toupper(name[j])) {
        if (names[i] == 0 && name[j] == 0) {
          return 1;
        }
        i++;
        j++;
      }
    }
  }
  return NOT_DEFINED;
}