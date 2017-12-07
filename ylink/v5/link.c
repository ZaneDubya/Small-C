/******************************************************************************
* ylink - the ypsilon linker (c) 2017 Zane Wagner. All rights reserved.
* link.c
* Reads args, inits variables, utility functions
******************************************************************************/
#include "stdio.h"
#include "notice.h"
#include "link.h"

// --- 'Line' (variable used when reading strings) ----------------------------
#define LINEMAX  127
#define LINESIZE 128
char *line;
char *pathOutput;
// --- File paths - these are the files that are read in by the linker --------
#define FILE_MAX 8
int *filePaths; // ptrs to input file paths, including library files.
int fileCount; // equal to the number of input files.
// --- ModData - information about the modules loaded for linking -------------
#define MDAT_CNT 128
#define MDAT_PER 4
#define MDAT_NAM 0
#define MDAT_CSO 1 // code segment origin
#define MDAT_DSO 2 // data segment origin
#define MDAT_FLG 3 // data segment origin
#define FlgInLib 0x0100
#define FlgStart 0x200
#define FlgStack 0x400
int *modData; // each obj has name ptr, 2 fields for seg
              // origin, and flag.
              // flag is 0x00ff lib_mod_idx, where 0..7==lib mod index,
              // 0x0100 in_lib, 0x0200 has start, 0x0400 has stack
              // 12..15=file index
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
int *segLengths; // length of segdefs, index is seg_xxx
byte *locSegs; // seg_xxxs in this module, in order they were defined
int segIndex; // index of next segdef that will be defined
// --- Public Definitions -----------------------------------------------------
#define PBDF_PER 3
#define PBDF_CNT 512
#define PBDF_NAME 0   // ptr to name of pubdef
#define PBDF_WHERE 1  // hi: index of module where it is located
                      // low: index of segment where it is located
#define PBDF_ADDR 2   // offset in segment (+module origin) where it is located
int *pbdfData;
int pbdfCount;

main(int argc, int *argv) {
  int i;
  puts(VERSION);
  AllocAll();
  RdArgs(argc, argv);
  Initialize();
  Pass1();
  for (i = 0; i < modCount; i++) {
    printf("\n  Mod %u = %s [cs:%x ds:%x flgs:%x]", i, 
      modData[i * MDAT_PER + MDAT_NAM],
      modData[i * MDAT_PER + MDAT_CSO], 
      modData[i * MDAT_PER + MDAT_DSO], 
      modData[i * MDAT_PER + MDAT_FLG]);
  }
  printf("\nMod Count: %u\nPubdef Count: %u\nCode: %u\nData: %u\nStack: %u",
    modCount, pbdfCount, segLengths[SEG_CODE], segLengths[SEG_DATA],
    segLengths[SEG_STACK]);
  putc(NEWLINE, 0);
  write_x16(0, FindPubDef("__ult"));
  write_x16(0, FindPubDef("__ugt"));
  write_x16(0, FindPubDef("_ccargc"));
  return;
}

AllocAll() {
  line = AllocMem(LINESIZE, 1);
  filePaths = AllocMem(FILE_MAX, 2);
  modData = AllocMem(MDAT_PER * MDAT_CNT, 2);
  locNames = AllocMem(LNAMES_CNT, 1);
  segLengths = AllocMem(SEGS_CNT, 2);
  locSegs = AllocMem(SEGS_CNT, 1);
  pbdfData = AllocMem(PBDF_PER * PBDF_CNT, 2);
  pathOutput = 0;
}

Initialize() {
  int i;
  // handle any uninitalized variables, unlink output
  unlink(LINKTXT); // delete
  if (pathOutput == 0) {
    printf("  No -e parameter. Output file will be out.exe.\n");
    pathOutput = AllocMem(8, 1);
    strcpy(pathOutput, "out.exe");
  }
  unlink(pathOutput); // delete
  for (i = 0; i < SEGS_CNT; i++) {
    segLengths[i] = 0;
  }
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
        case 'e':
          RdArgExe(c+3);
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
    fatalf("  Error: max of %u input files.", FILE_MAX);
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

// ============================================================================
// === Pass1 ==================================================================
// ============================================================================
// For each object file,
// 1. get object file description and save in objdefs,
// 2. for each segment, get name, create seg def (if necessary), set origins,
// 3. for each pubdef, add to pubdefs.
byte libInLib; // if 1, we are in a library obj.
byte libIdxModule;

Pass1() {
  uint i, fd;
  puts("Pass 1:");
  modCount = 0;
  pbdfCount = 0;
  for (i = 0; i < fileCount; i++) {
    printf("  Reading %s... ", filePaths[i],);
    if (!(fd = fopen(filePaths[i], "r"))) {
      fatalf("Could not open file '%s'", filePaths[i]);
    }
    P1_RdFile(i, fd);
    cleanupfd(fd);
    puts("Done.");
  }
}

P1_RdFile(uint fileIndex, uint objfd) {
  uint length;
  byte recType;
  libInLib = libIdxModule = 0; // reset library vars.
  while (1) {
    recType = read_u8(objfd);
    if (feof(objfd) || ferror(objfd)) {
      break;
    }
    length = read_u16(objfd);
    P1_DoRec(fileIndex, recType, length, objfd);
  }
  return;
}

P1_DoRec(uint fileIndex, byte recType, uint length, uint fd) {
  switch (recType) {
    case THEADR:
      P1_THEADR(fileIndex, length, fd);
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
      P1_SEGDEF(length, fd);
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
      fatalf("P1_DoRec: Unknown record of type %x. Exiting.", recType);
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
  if (modCount == MDAT_CNT) {
    fatalf("  Error: max of %u object modules.", MDAT_CNT);
  }
  AddModule(fileIndex, fd);
  // reset segments to not present
  for (i = 0; i < SEGS_CNT; i++) {
    locSegs[i] = SEG_NOTPRESENT;
  }
  // first segment index is 0
  segIndex = 0;
  read_u8(fd); // checksum. assume correct.
}

AddModule(uint fileIndex, uint fd) {
  int length, i;
  char *path, *c;
  // copy the module name and set up data.
  length = read_strpre(line, fd);
  c = line;
  path = modData[modCount * MDAT_PER + MDAT_NAM] = AllocMem(length, 1);
  for (i = 0; i < length; i++) {
    *path++ = *c++;
  }
  *path = 0;
  // set the module data.
  modData[modCount * MDAT_PER + MDAT_CSO] = 0;
  modData[modCount * MDAT_PER + MDAT_DSO] = 0;
  modData[modCount * MDAT_PER + MDAT_FLG] = (fileIndex << 12);
  if (libInLib) {
    modData[modCount * MDAT_PER + MDAT_FLG] |= libIdxModule | FlgInLib;
    libIdxModule += 1;
  }
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
    rd_fix_data(0, fd);
    modData[modCount * MDAT_PER + MDAT_FLG] |= FlgStart;
  }
  read_u8(fd); // checksum. assume correct.
  // MODEND can be followed with up to a paragraph of unknown data.
  if (!feof(fd)) {
    int remaining;
    remaining = 16 - (ctellc(fd) % 16);
    if (remaining != 16) {
      clearsilent(remaining, fd);
    }
  }
  modCount += 1;
  return;
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
      fatalf("  Error: P1_LNAMES max of %u local names.", LNAMES_CNT);
    }
    length -= read_strpre(line, fd); // line = local name at index nameIndex.
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
      fatalf("  Error: P1_LNAMES does not handle name %s.", line);
    }
    nameIndex++;
  }
  while (nameIndex < LNAMES_CNT) {
    locNames[nameIndex++] = LNAME_NULL;
  }
  read_u8(fd); // checksum. assume correct.
  return;
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
    fatal("P1_PUBDEF: BaseGroup must be 0.");
  }
  basesegment = read_u8(fd);
  if (basesegment == 0) {
    fatal("P1_PUBDEF: BaseSegment must be nonzero.");
  }
  length -= 2;
  while (length > 1) {
    int namelen;
    namelen = read_strpre(line, fd);
    alreadyDefined = FindPubDef(line);
    if (alreadyDefined != -1) {
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
      fatal("P1_PUBDEF: Type is not 0. ");
    }
    c = line;
    pdbfname = pbdfData[pbdfCount * PBDF_PER + PBDF_NAME] = AllocMem(namelen, 1);
    for (i = 0; i < namelen; i++) {
      *pdbfname++ = *c++;
    }
    *pdbfname = 0;
    // set the module data.
    pbdfData[pbdfCount * PBDF_PER + PBDF_WHERE] = locSegs[basesegment - 1];
    pbdfData[pbdfCount * PBDF_PER + PBDF_WHERE] |= modCount << 8;
    pbdfData[pbdfCount * PBDF_PER + PBDF_ADDR] = puboffset;
    length -= namelen + 3;
    pbdfCount += 1;
  }
  read_u8(fd); // checksum. assume correct.
  return;
}

// 98H SEGDEF Segment Definition Record
// The SEGDEF record describes a logical segment in an object module. It
// defines the segment's name, length, and alignment, and the way the segment
// can be combined with other logical segments at bind, link, or load time.
// Object records that follow a SEGDEF record can refer to it to identify a
// particular segment.  The SEGDEF records are ordered by occurrence, and are
// referenced by segment indexes (starting from 1) in subsequent records.
P1_SEGDEF(uint length, uint fd) {
  byte segattr, segname, classname;
  uint seglen;
  // segment attribute
  segattr = read_u8(fd);
  if ((segattr & 0xe0) != 0x60) {
    fatal("P1_SEGDEF: Unknown segment attribute field (must be 0x60).");
  }
  if (((segattr & 0x1c) != 0x08) && ((segattr & 0x1c) != 0x14)) {
    // 0x08 = Public. Combine by appending at an offset that meets the alignment requirement.
    // 0x14 = Stack. Combine as for C=2. This combine type forces byte alignment.
    fatalf("P1_SEGDEF: Unknown combine %x (req: 0x08 or 0x14).", segattr & 0x1c);
  }
  if ((segattr & 0x02) != 0x00) {
    fatal("P1_SEGDEF: Attribute may not be big (flag 0x02).");
  }
  if ((segattr & 0x01) != 0x00) {
    fatal("P1_SEGDEF: Attribute must be 16-bit addressing (flag 0x01).");
  }
  seglen = read_u16(fd);
  segname = read_u8(fd);
  classname = read_u8(fd); // ClassName is not used by SmallAsm.
  read_u8(fd); // The linker ignores the Overlay Name field.
  read_u8(fd); // checksum. assume correct.
  // fprintf(0, "\nLength=%x Name=%x Class=%x", seglen, segname, classname);
  segname = locNames[segname - 1];
  classname = locNames[classname - 1];
  if (classname != LNAME_NULL) {
    fatalf("P1_SEGDEF: ClassName must be null; LNAME == %x", classname);
  }
  if (segname == LNAME_CODE) {
    modData[modCount * MDAT_PER + MDAT_CSO] = segLengths[SEG_CODE];
    locSegs[segIndex] = SEG_CODE;
    segLengths[SEG_CODE] += seglen;
  }
  else if (segname == LNAME_DATA) {
    modData[modCount * MDAT_PER + MDAT_DSO] = segLengths[SEG_DATA];
    locSegs[segIndex] = SEG_DATA;
    segLengths[SEG_DATA] += seglen;
  }
  else if (segname == LNAME_STACK) {
    modData[modCount * MDAT_PER + MDAT_FLG] |= FlgStack;
    locSegs[segIndex] = SEG_STACK;
    segLengths[SEG_STACK] += seglen;
  }
  else {
    fatalf("P1_SEGDEF: Linker does not handle LNAME of %x", segname);
  }
  segIndex += 1;
  return;
}

// Note: I've removed code to load the library dictionary and dependancies.
// This should be parsed, not loaded in memory and kept there. In the v4 
// codebase, this is everything between btell and bseek.
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
  return;
}

// F1H End Library
P1_LIBEND(uint length, uint fd) {
  if (!libInLib) {
    fatal("P1_LIBEND: not a library!", 0);
  }
  clearsilent(length, fd);
  fclose(fd);
  libInLib = 0;
}

// === Fixup Routines =========================================================

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
do_fixupp(uint outfd, uint length, uint fd) {
  fprintf(outfd, "FIXUPP");
  while (length > 1) {
    fputs("\n    ", outfd);
    rd_fix_locat(outfd, length, fd);
    length = rd_fix_data(length, fd);
  }
  read_u8(fd); // checksum. assume correct.
  return;
}

rd_fix_locat(uint outfd, uint length, uint fd) {
  byte locat;
  byte relativeMode;  // 1 == segment relative, 0 == self relative.
  byte refType;      // 4-bit value   
  uint dataOffset;
  // -----------------------------------------------------------------
  // The first bit (in the low byte) is always  one  to  indicate
  // that this block defines a "fixup" as opposed to a "thread."
  locat = read_u8(fd);
  if ((locat & 0x80) == 0) {
    fatalf("\nError: must be fixup, not thread (%x)", locat);
  }
  // -----------------------------------------------------------------
  //  The REFERENCE MODE bit indicates how the reference is made.
  //  * Self-relative references locate a target address relative to
  //    the CPU's instruction pointer (IP); that is, the target is a
  //    certain distance from the location currently indicated by
  //    IP. This sort of reference is common to the jump
  //    instructions. Such a fixup is not necessary unless the
  //    reference is to a different segment.
  //  * Segment-relative references locate a target address in any segment 
  //    relative to the beginning of the segment. This is just the
  //    "displacement" field that occurs in so many instructions.
  relativeMode = (locat & 0x40) != 0;
  if (relativeMode == 0) {
    fputs("Rel=Self, ", outfd);
  }
  else {
    fputs("Rel=Sgmt, ", outfd);
  }
  // -----------------------------------------------------------------
  // The TYPE REFERENCE bits (called the LOC  bits  in  Microsoft
  // documentation) encode the type of reference as follows:
  refType = (locat & 0x3c) >> 2; // 4 bit field.
  switch (refType) {
    case 0:   // low byte of an offset
      fputs("Loc=Low, ", outfd);
      break;
    case 1:   // 16bit offset part of a pointer
      fputs("Loc=16bit offset, ", outfd);
      break;
    case 2:   // 16bit base (segment) part of a pointer
      fputs("Loc=16bit segment, ", outfd);
      break;
    // SmallC22/SmallA do not emit 32 bit pointers or high byte offsets.
    /*case 3:   // 32bit pointer (offset/base pair)
      fputs("Loc=16bit:16bit, ", outfd);
      break;
    case 4:   // high byte of an offset
      fputs("Loc=High, ", outfd);
      break;*/
    default:
      fatalf("rd_fix_locat: refType %u in fixupp", refType);
      break;
  }
  // -----------------------------------------------------------------
  //  The DATA RECORD OFFSET subfield specifies the offset, within
  //  the preceding data record, to the reference. Since a  record
  //  can  have at most 1024 data bytes, 10 bits are sufficient to
  //  locate any reference.
  dataOffset = read_u8(fd) + ((locat & 0x03) << 8);
  fprintf(outfd, "DtOff=%x, ", dataOffset);
}

// rd_fix_data: Reads a fixupp data from a fixupp record.
//    Returns the count of bytes remaining in the record.
rd_fix_data(uint length, uint fd) {
  uint targetOffset;
  byte fixdata, frame, target;
  // -----------------------------------------------------------------
  //  FRAME/TARGET METHODS is a byte which encodes the methods by which 
  //  the fixup "frame" and "target" are to be determined, as follows.
  //  In the first three cases  FRAME  INDEX  is  present  in  the
  //  record  and  contains an index of the specified type. In the
  //  last two cases there is no FRAME INDEX field.
  fixdata = read_u8(fd); // Format is fffftttt
  // I have omitted the fixups that SmallC22/SmallA do not emit.
  switch ((fixdata & 0xf0) >> 4) {
    case 0:     // frame given by a segment index
      frame = read_u8(fd);
      length -= 1;
      // fprintf(outfd, "Frame=Seg %x, ", frame);
      break;
    case 2:     // frame given by an external index
      frame = read_u8(fd);
      length -= 1;
      // fprintf(outfd, "Frame=Ext %x, ", frame);
      break;
    /*case 1:     // frame given by a group index
      frame = read_u8(fd);
      length -= 1;
      fprintf(outfd, "Frame=Grp %x, ", frame);
      break;
    case 4:     // frame is that of the reference location
      fputs("Frame=Ref, ", outfd);
      break;
    case 5:     // frame is determined by the target
      fputs("Frame=Tgt, ", outfd);
      break;*/
    default:
      printf("\nError: Unknown frame method %x", fixdata >> 4);
      break;
  }
  // -----------------------------------------------------------------
  //  The TARGET bits tell LINK to determine  the  target  of  the
  //  reference in one of the following ways. In each case TARGET INDEX
  //  is present in the record and contains an index of the indicated
  //  type. In the first three cases TARGET OFFSET is present and
  //  specifies the offset from the location of the segment, group, 
  //  or external address to the target. In the last three cases there
  //  is no TARGET OFFSET because an offset of zero is assumed.
  target = read_u8(fd);
  length -= 1;
  switch ((fixdata & 0x0f)) {
    case 0 :  // target given by a segment index + displacement
      targetOffset = read_u16(fd);
      length -= 2;
      // fprintf(outfd, "Target=Seg+%x [%x]", targetOffset, target);
      break;
    case 2 :  // target given by an external index + displacement
      targetOffset = read_u16(fd);
      length -= 2;
      // fputs(outfd, "Target=Ext+%x [%x]", targetOffset, target);
      break;
    case 6 :  // target given by an external index alone
      // fputs(outfd, "Target=Ext [%x]", target);
      break;
    /*case 1 :  // target given by a group index + displacement
      targetOffset = read_u16(fd);
      length -= 2;
      fputs(outfd, "Target=Grp+%x", targetOffset);
      break;
    case 4 :  // target given by a segment index alone
      fputs("Target=Seg+0", outfd);
      break;
    case 5 :  // target given by a group index alone
      fputs("Target=Grp+0", outfd);
      break;*/
    default:
      printf("\nError: Unknown target method %x", fixdata & 0x0f);
      break;
  }
  length -= 3;
  return length;
}

// ============================================================================
// === Pass2 ==================================================================
// ============================================================================
// returns index of pubdef with this name, or -1 (0xffff) if it is not present
FindPubDef(char* name) {
  int i, j, length;
  char* matching;
  length = strlen(name);
  // printf("\nFindPubDef for %s, length=%u", name, length);
  for (i = 0; i < pbdfCount; i++) {
    matching = pbdfData[i * PBDF_PER + PBDF_NAME];
    // printf("\nAttempt Match against for %s...", matching);
    for (j = 0; j <= length; j++) {
      // printf("\n  %u: %c vs %c", j, name[j], matching[j]);
      if (name[j] == 0 && matching[j] == 0) {
        return i;
      }
      if (toupper(name[j]) != toupper(matching[j])) {
        break;
      }
    }
  }
  return -1;
}

// === Write Hexidecimal numbers ==============================================

write_x16(uint fd, uint value) {
  write_x8(fd, value >> 8);
  write_x8(fd, value & 0x00ff);
  return;
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
  return;
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
read_strpre(char* str, uint fd) {
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

// === File Management ========================================================

cleanupfd(uint fd) {
  if (fd != 0) {
    fclose(fd);
  }
  return;
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

clearsilent(uint length, uint fd) {
  while (length-- > 0) {
    read_u8(fd);
  }
}

// === Error Routines =========================================================

fatal(char *str) {
  errout(str);
  abort(1);
}

fatalf(char *format, char *arg) {
  erroutf(format, arg);
  abort(1);
}

errout(char *str) {
    fputs("    Error: ", stderr);
    fputs(str, stderr);
    fputc(NEWLINE, stderr);
}

erroutf(char *format, char *arg) {
    fputs("    Error: ", stderr); 
    fprintf(stderr, format, arg);
    fputc(NEWLINE, stderr);
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

// === String Functions =======================================================

strcmp(char *s1, char *s2)
{
    for(; *s1 == *s2; ++s1, ++s2)
        if(*s1 == 0)
            return 0;
    return *s1 < *s2 ? -1 : 1;
}
