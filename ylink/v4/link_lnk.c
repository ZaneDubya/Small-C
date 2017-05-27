/******************************************************************************
* link_lnk.c
* Actual linking happens here!
******************************************************************************/
#include "stdio.h"
#include "link.h"

#define tbObj_EntrySize 16
#define tbObj_Count     64
#define tbExt_Size      2048
#define tbPub_Size      2048
#define ofNms_Size      1024

#define TO_Name         0
#define TO_LibIbx       2
#define TO_HasSegs      3
#define TO_CodeSize     4
#define TO_DataSize     6
#define TO_CodeLoc      8
#define TO_DataLoc      10
#define TO_PubDefPtr    12
#define TO_ExtDefPtr    14

#define TO_SegCode      0x01
#define TO_SegData      0x02
#define TO_SegStack     0x04

byte *tbObj;      // Object Resolution Table
byte *tbExt;      // External Definition Table
byte *tbPub;      // Public Definition Table
char *tbOfNames;  // ObjFile Names
uint ofNmsNext;   

AllocLnkMemory() {
  tbObj = allocvar(tbObj_EntrySize * tbObj_Count, 1);
  tbOfNames = allocvar(ofNms_Size, 1);
  tbExt = allocvar(tbExt_Size, 1);
  tbPub = allocvar(tbPub_Size, 1);
}

LnkResolveExtDefs(uint fileCnt, uint *files) {
  uint i;
  puts("\nPass 0: Resolving extdefs... ");
  for (i = 0; i < fileCnt; i++) {
    char *filename;
    filename = files[i];
    if (!IsExt(filename, ".obj")) {
      continue;
    }
    Lnk0_AddObj(filename, i, 0);
    puts("Done.");
  }

  //  foreach (obj in objs) {
  //    Add obj to tblObjs: name, index (if lib), segment sizes.
  //    Get segment placement data.
  //    Get next free byte in tblext and tblpub.
  //    Build list of pubdefs and extdefs:
  //      when a extdef is added, if already in the extdef table, skip!
  //      when a pubdef is added, if already in the pubdef table, error!
  //  }
  //  allmatched = matchDefs();
  //  function matchDefs() {
  //    foreach (pudbef in tblPubdef) {
  //      for all matching extdefs, extdef.modidx = pubdef.modidx
  //    }
  //    return (all extdefs satisfied ? 1 : 0);
  //  }
  //  if (allmatched) {
  //    // all extdefs satisfied
  //    procede to next stage.
  //  }
  //  else {
  //    // we need to resolve missing extdefs with libraries:
  //    while (allmatched == 0) {
  //      get first unmatched extdef.
  //      byte matched = 0;
  //      foreach (lib in libs) {
  //        if (lib.haspubdef(extdef)) {
  //          matched = 1;
  //          add the library object as if it were an obj file.
  //          allmatched = matchDefs();
  //          break; // (from foreach lib in libs)
  //        }
  //      }
  //      if (matched == 0) {
  //        error "could not resolve extdef "x""; abort.
  //      }
  //    }
  //  }
  //  proceed to stage 2!
}

// ============================================================================
// Link Pass 0
// ============================================================================

Lnk0_AddObj(char* filename, uint idx, byte libidx) {
  uint fd, obj;
  printf("  %s: ", filename);
  obj = tbObj + idx * tbObj_EntrySize;
  *(obj + TO_Name) = AddName(filename, tbOfNames, &ofNmsNext, ofNms_Size);
  *(obj + TO_LibIbx) = libidx;
  if (!(fd = fopen(*(obj + TO_Name), "r"))) {
    fatalf("Could not open file '%s'", *(obj + TO_Name));
  }
  while (1) {
    byte recType;
    uint length;
    recType = read_u8(fd);
    if (feof(fd) || ferror(fd)) {
      break;
    }
    length = read_u16(fd);
    Lnk0_DoRec(recType, length, obj);
  }
  Cleanup(fd);
}

Lnk0_DoRec(byte recType, uint length, uint obj) {
  
}

// === Helper Utilities =======================================================

IsExt(char *str, char *ext) {
  int i, ln;
  ln = strlen(ext);
  str = str + (strlen(str) - strlen(ext));
  for (i = 0; i < ln; i++) {
    if (toupper(str[i]) != toupper(ext[i])) {
      return 0;
    }
  }
  return 1;
}

// searches for substr in str.
// if found, return location of first instance.
// if not found, return 0.
strstr(char *str, char *substr)
{
  char *a, *b;
  b = substr;
  if (*b == 0) {
    return str;
  }
  for ( ; *str != 0; str += 1) {
	  if (*str != *b) {
	    continue;
	  }
    a = str;
    while (1) {
        if (*b == 0) {
          return str;
        }
        if (*a++ != *b++) {
          break;
        }
    }
    b = substr;
  }
  return 0;
}

// AddName: adds a null-terminated string to a byte array. Returns ptr to
// byte array where string was placed. Fails if string will not fit.
AddName(char *name, char *names, uint next, uint max) {
  uint nameat, i;
  nameat = i = *next;
  while (names[i++] = *name++) {
    if (i >= max) {
      fprintf(stderr, "\n%s at %x exceeded names length of %x (%x)\n", 
              name, nameat, max, i);
      abort(1);
    }
  }
  *next = i;
  return nameat + names;
}
