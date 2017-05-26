/******************************************************************************
* link_lnk.c
* Actual linking happens here!
******************************************************************************/
#include "stdio.h"
#include "link.h"

#define tbObj_EntrySize 28
#define tbObj_Count     64
#define tbExt_Size      2048
#define tbPub_Size      2048

// Object Resolution Table
byte *tbObj;

// External Definition Table
byte *tbExt;

// Public Definition Table
byte *tbPub;

AllocLnkMemory() {
  tbObj = allocvar(tbObj_EntrySize * tbObj_Count, 1);
  tbExt = allocvar(tbExt_Size, 1);
  tbPub = allocvar(tbPub_Size, 1);
}

LnkResolveExtDefs() {
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