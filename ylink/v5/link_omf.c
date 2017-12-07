/******************************************************************************
* ylink - the ypsilon linker (c) 2017 Zane Wagner. All rights reserved.
* link_omf.c
* Each do_ method handles one kind of record found in a object or library file.
******************************************************************************/
#include "stdio.h"
#include "link.h"

extern char *line;
extern byte segIndex;
extern byte libInLib, libIdxModule;

do_record(uint outfd, byte recType, uint length, uint fd) {
  write_x8(outfd, recType);
  fputs("  ", outfd);
  switch (recType) {
    case THEADR:
      do_theadr(outfd, length, fd);
      break;
    case COMMNT:
      do_commnt(outfd, length, fd);
      break;
    case MODEND:
      do_modend(outfd, length, fd);
      break;
    case EXTDEF:
      do_extdef(outfd, length, fd);
      break;
    case PUBDEF:
      do_pubdef(outfd, length, fd);
      break;
    case LNAMES:
      do_lnames(outfd, length, fd);
      break;
    case SEGDEF:
      do_segdef(outfd, length, fd);
      break;
    case FIXUPP:
      do_fixupp(outfd, length, fd);
      break;
    case LEDATA:
      do_ledata(outfd, length, fd);
      break;
    case LIDATA:
      do_lidata(outfd, length, fd);
      break;
    case LIBHDR:
      do_libhdr(outfd, length, fd);
      break;
    case LIBEND:
      do_libend(outfd, length, fd);
      break;
    case LIBDEP:
      do_libdep(length, fd);
      break;
    default:
      fatalf("do_record: Unknown record of type %x. Exiting.", recType);
      break;
  }
  fputc(NEWLINE, outfd);
}

clearrecord(uint outfd, uint length, uint fd) {
  int count;
  byte ch;
  count = 0;
  while (length-- > 0) {
    if ((count % 32) == 0) {
      fputs("\n    ", outfd);
      write_x16(outfd, count);
      fputs(": ", outfd);
    }
    ch = read_u8(fd);
    write_x8(outfd, ch);
    count++;
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
do_theadr(uint outfd, uint length, uint fd) {
  read_strp(line, fd);
  fprintf(outfd, "THEADR %s", line);
  if (libInLib) {
    fputs("  LibMod", outfd);
    write_x8(outfd, libIdxModule++);
  }
  read_u8(fd); // checksum. assume correct.
  return;
}

// A2H LIDATA Logical Iterated Data Record
// Like the LEDATA record, the LIDATA record contains binary data—executable
// code or program data. The data in an LIDATA record, however, is specified
// as a repeating pattern (iterated), rather than by explicit enumeration.
// The data in an LIDATA record can be modified by the linker if the LIDATA
// record is followed by a FIXUPP record, although this is not recommended.
do_lidata(uint outfd, uint length, uint fd) {
  fprintf(outfd, "LIDATA");
  while (length-- > 0) {
    read_u8(fd);
  }
  return;
}

// 88H COMMNT Comment Record.
do_commnt(uint outfd, uint length, uint fd) {
  byte commtype, commclass;
  fprintf(outfd, "COMMNT ", outfd);
  commtype = read_u8(fd);
  commclass = read_u8(fd);
  switch (commclass) {
    case 0xA3:
      read_strp(line, fd);
      fprintf(outfd, "LIBMOD=%s", line);
      break;
    default:
      clearrecord(length - 3, fd);
      fatal("do_commnt: UNKNOWN TYPE");
      break;
  }
  read_u8(fd); // checksum. assume correct.
  return;
}

// 8AH MODEND Module End Record
// The MODEND record denotes the end of an object module. It also indicates
// whether the object module contains the main routine in a program, and it
// can optionally contain a reference to a program's entry point.
do_modend(uint outfd, uint length, uint fd) {
  byte moduletype;
  fprintf(outfd, "MODEND ");
  moduletype = read_u8(fd); // format is MS0....1
  if (moduletype & 0x80) {
    // Is set if the module is a main program module.
    fprintf(outfd, "Main ");
  }
  if (moduletype & 0x40) {
    // Is set if the module contains a start address; if this bit is set, the
    // field starting with the End Data byte is present and specifies the start
    // address.
    fprintf(outfd, "Start ");
    rd_fix_data(0, fd);
  }
  read_u8(fd); // checksum. assume correct.
  if (!feof(fd)) {
    int remaining;
    remaining = 16 - (ctellc(fd) % 16);
    if (remaining != 16) {
      clearsilent(remaining, fd);
    }
  }
  fputs("\n============================================================", outfd);
  segIndex = 1; // reset first segment - should always be 1.
  return;
}

// 8CH EXTDEF External Names Definition Record
// The EXTDEF record contains a list of symbolic external references—that is,
// references to symbols defined in other object modules. The linker resolves
// external references by matching the symbols declared in EXTDEF records
// with symbols declared in PUBDEF records.
do_extdef(uint outfd, uint length, uint fd) {
  byte deftype, linelength, strlength;
  fprintf(outfd, "EXTDEF ");
  linelength = 11;
  while (length > 1) {
    strlength = read_strpre(line, fd);
    length -= strlength + 1;
    if (linelength + strlength + 1 >= 80) {
      fputs(TWOTABS, outfd);
      linelength = 4;
    }
    linelength += strlength + 1;
    fprintf(outfd, "%s, ", line);
    deftype = read_u8(fd);
    if (deftype != 0) {
      fatal("do_extdef: Type is not 0. ");
    }
  }
  read_u8(fd); // checksum. assume correct.
  return;
}

// 90H PUBDEF Public Names Definition Record
// The PUBDEF record contains a list of public names.  It makes items defined
// in this object module available to satisfy external references in other
// modules with which it is bound or linked. The symbols are also available
// for export if so indicated in an EXPDEF comment record.
do_pubdef(uint outfd, uint length, uint fd) {
  byte basegroup, basesegment, typeindex;
  uint puboffset;
  fprintf(outfd, "PUBDEF ");
  // BaseGroup and BaseSegment fields contain indexes specifying previously
  // defined SEGDEF and GRPDEF records.  The group index may be 0, meaning
  // that no group is associated with this PUBDEF record.
  // BaseFrame field is present only if BaseSegment field is 0, but the
  // contents of BaseFrame field are ignored.
  // BaseSegment idx is normally nonzero and no BaseFrame field is present.
  basegroup = read_u8(fd);
  basesegment = read_u8(fd);
  fprintf(outfd, "Grp=%x Seg=%x", basegroup, basesegment);
  if (basegroup != 0) {
    fatal("do_pubdef: BaseGroup must be 0.");
  }
  if (basesegment == 0) {
    fatal("do_pubdef: BaseSegment must be nonzero.");
  }
  length -= 2;
  while (length > 1) {
    length -= (read_strpre(line, fd) + 3);
    puboffset = read_u16(fd);
    typeindex = read_u8(fd);
    if (typeindex != 0) {
      fatal("do_pubdef: Type is not 0. ");
    }
    fputs(TWOTABS, outfd);
    write_x16(outfd, puboffset);
    fputc('=', outfd);
    fputs(line, outfd);
  }
  read_u8(fd); // checksum. assume correct.
  return;
}

// 96H LNAMES List of Names Record
// The LNAMES record is a list of names that can be referenced by subsequent
// SEGDEF and GRPDEF records in the object module. The names are ordered by
// occurrence and referenced by index from subsequent records.  More than one
// LNAMES record may appear.  The names themselves are used as segment, class,
// group, overlay, and selector names.
do_lnames(uint outfd, uint length, uint fd) {
  int nameIndex; 
  nameIndex = 1;
  fputs("LNAMES ", outfd);
  while (length > 1) {
    length -= read_strpre(line, fd);
    fprintf(outfd, "%u='%s' ", nameIndex, line);
    nameIndex++;
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
do_segdef(uint outfd, uint length, uint fd) {
  byte segIndex, segattr, segname, classname, overlayname;
  uint seglength;
  fprintf(outfd, "SEGDEF ");
  // segment attribute
  segattr = read_u8(fd);
  if ((segattr & 0xe0) != 0x60) {
    fatal("do_segref: Unknown segment attribute field (must be 0x60).");
  }
  if (((segattr & 0x1c) != 0x08) && ((segattr & 0x1c) != 0x14)) {
    // 0x08 = Public. Combine by appending at an offset that meets the alignment requirement.
    // 0x14 = Stack. Combine as for C=2. This combine type forces byte alignment.
    fatalf("do_segref: Unknown combo %x (must be 0x08).", segattr & 0x1c);
  }
  if ((segattr & 0x02) != 0x00) {
    fatal("do_segref: Attribute may not be big (flag 0x02).");
  }
  if ((segattr & 0x01) != 0x00) {
    fatal("do_segref: Attribute must be 16-bit addressing (flag 0x01).");
  }
  segIndex = segIndex++;
  seglength = read_u16(fd);
  segname = read_u8(fd);
  classname = read_u8(fd); // ClassName is not used by SmallAsm.
  overlayname = read_u8(fd); // The linker ignores the Overlay Name field.
  fprintf(outfd, "Idx=%x Length=%x Name=%x Class=%x", 
          segIndex, seglength, segname, classname);
  read_u8(fd); // checksum. assume correct.
  return;
}

// A0H  LEDATA Logical Enumerated Data Record
do_ledata(uint outfd, uint length, uint fd) {
  byte segindex;
  uint dataoffset;
  fputs("LEDATA ", outfd);
  segindex = read_u8(fd);
  dataoffset = read_u16(fd);
  length -= 4; // don't count the segindex, dataoffset, or checksum.
  fprintf(outfd, "SegIndex=%x DataOffset=%x Length=%x", segindex, dataoffset, length);
  clearrecord(outfd, length, fd);
  read_u8(fd); // checksum. assume correct.
  return;
}

// F0H  LIBHDR Library Header Record
do_libhdr(uint outfd, uint length, uint fd) {
  byte flags, nextRecord;
  uint omfOffset[2], dictOffset[2], blockCount, nextLength;
  fputs("LIBHDR ", outfd);
  dictOffset[0] = read_u16(fd);
  dictOffset[1] = read_u16(fd);
  blockCount = read_u16(fd);
  flags = read_u8(fd);
  fprintf(outfd, "DictOffset=%u+(%ux2^16) Blocks=%u Flags=%x\n", 
          dictOffset[0], dictOffset[1], blockCount, flags);
  allocDictMemory(blockCount * DICT_BLOCK_CNT);
  length -= 8;
  // rest of record is zeros.
  while (length-- > 0) {
    read_u8(fd);
  }
  read_u8(fd); // checksum. assume correct.
  libInLib = 1;
  // seek to library data offset, read data, return to module data offset
  btell(fd, omfOffset);
  do_dictionary(outfd, fd, dictOffset, blockCount);
  putLibDict(outfd);
  nextRecord = read_u8(fd);
  if (nextRecord == LIBDEP) {
    nextLength = read_u16(fd);
    fputs("\nF2  LIBDEP", outfd);
    do_libdep(outfd, nextLength, fd);
  }
  else {
    fatal("do_libhdr: No dependancy information");
  }
  bseek(fd, omfOffset, 0);
  return;
}

// F1H Library End
do_libend(uint outfd, uint length, uint fd) {
  fprintf(outfd, "LIBEND Padding=%x", length);
  clearsilent(length, fd);
  if (!libInLib) {
    fatal("do_libend: not a library!", outfd);
  }
  fputc('\n', outfd);
  // writeLibData(outfd);
  fclose(fd);
}

// F2H Extended Dictionary
// The extended dictionary is optional and indicates dependencies between
// modules in the library. Versions of LIB earlier than 3.09 do not create an
// extended dictionary. The extended dictionary is placed at the end of the
// library. 
do_libdep(uint length, uint fd) {
  uint i, page, offset, count;
  count = read_u16(fd);
  length -= 2;
  allocDependancyMemory(count);
  for (i = 0; i <= count; i++) {
    page = read_u16(fd);
    offset = read_u16(fd);
    offset -= (count + 1) * 4;
    addDependancy(i, page, offset);
    length -= 4;
  }
  // ignore final null record
  readDependancies(fd, length);
  // fprintf(outfd, "\n    Library Dependencies (%u Modules)", count);
  // writeDependancies(outfd);
  // fputc('\n', outfd);
  return;
}
