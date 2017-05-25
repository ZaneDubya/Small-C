// Definitions for the various Object Module Formats
#include "stdio.h"
#include "link.h"

#define TWOTABS "\n    "
#define LNAME_MAX 4

extern char *line;

do_record(uint outfd, byte recType, uint length, uint fd) {
        int x;
  puthexch(outfd, recType);
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
      do_libdep(outfd, length, fd);
      break;
    default:
      printf("\n    do_record: Unknown record of type %x. Exiting.", recType);
      abort(1);
      break;
  }
  fputc(NEWLINE, outfd);
}

clearrecord(uint outfd, uint length, uint fd) {
  while (length-- > 0) {
    byte ch;
    ch = read_u8(fd);
    puthexch(outfd, ch);
  }
}

clearsilent(uint outfd, uint length, uint fd) {
  while (length-- > 0) {
    read_u8(fd);
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
      printf("Error: UNKNOWN TYPE");
      clearrecord(length - 3, fd);
      abort(1);
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
    rd_fix_data(outfd, 0, fd);
  }
  read_u8(fd); // checksum. assume correct.
  if (!feof(fd)) {
    int remaining;
    remaining = 16 - (ctellc(fd) % 16);
    if (remaining != 16) {
      clearsilent(outfd, remaining, fd);
    }
  }
  
  fputc('\n', outfd);
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
      printf("Error: Type is not 0. ");
      abort(1);
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
        printf(" Error: BaseGroup must be 0.");
        abort(1);
    }
    if (basesegment == 0) {
        printf(" Error: BaseSegment must be nonzero.");
        abort(1);
    }
    length -= 2;
    fputs(TWOTABS, outfd);
    while (length > 1) {
        length -= read_strpre(line, fd) + 3;
        puboffset = read_u16(fd);
        fprintf(outfd, "%s@%x ", line, puboffset);
        typeindex = read_u8(fd);
        if (typeindex != 0) {
            fputs("Error: Type is not 0. ", outfd);
            abort(1);
        }
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
    byte segattr, segname, classname, overlayname;
    uint seglength;
    fprintf(outfd, "SEGDEF ");
    // segment attribute
    segattr = read_u8(fd);
    if ((segattr & 0xe0) != 0x60) {
        printf("Unknown segment attribute field (must be 0x60).",);
        abort(1);
    }
    if (((segattr & 0x1c) != 0x08) && ((segattr & 0x1c) != 0x14)) {
      // 0x08 = Public. Combine by appending at an offset that meets the alignment requirement.
      // 0x14 = Stack. Combine as for C=2. This combine type forces byte alignment.
      printf("Unknown segment combination %x (must be 0x08).", segattr & 0x1c);
      abort(1);
    }
    if ((segattr & 0x02) != 0x00) {
        printf("Attribute may not be big (flag 0x02).");
        abort(1);
    }
    if ((segattr & 0x01) != 0x00) {
        printf("Attribute must be 16-bit addressing (flag 0x01).");
        abort(1);
    }
    seglength = read_u16(fd);
    segname = read_u8(fd);
    classname = read_u8(fd);
    overlayname = read_u8(fd);
    fprintf(outfd, "Length=%u Name=%u Class=%u Overlay=%u", 
        seglength, segname, classname, overlayname);
    read_u8(fd); // checksum. assume correct.
    return;
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
do_fixupp(uint outfd, uint length, uint fd) {
  fprintf(outfd, "FIXUPP");
  while (length > 1) {
    fputs("\n    Fix=", outfd);
    rd_fix_locat(outfd, length, fd);
    length = rd_fix_data(outfd, length, fd);
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
    printf("\nError: must be fixup, not thread (%x)", locat);
    abort(1);
  }
  // -----------------------------------------------------------------
  //  The  REFERENCE MODE bit indicates how the reference is made.
  //  Self-relative references locate a target address relative to
  //  the CPU's instruction pointer (IP); that is, the target is a
  //  certain distance from the location  currently  indicated  by
  //  IP.   This   sort   of  reference  is  common  to  the  jump
  //  instructions. Such a  fixup  is  not  necessary  unless  the
  //  reference   is  to  a  different  segment.  Segment-relative
  //  references locate a target address in any  segment  relative
  //  to   the   beginning  of  the  segment.  This  is  just  the
  //  "displacement" field that occurs in so many instructions.
  relativeMode = (locat & 0x40) != 0;
  if (relativeMode == 0) {
    fputs("Self-Rel, ", outfd);
  }
  else {
    fputs("Sgmt-Rel, ", outfd);
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
    case 3:   // 32bit pointer (offset/base pair)
      fputs("Loc=16bit:16bit, ", outfd);
      break;
    case 4:   // high byte of an offset
      fputs("Loc=High, ", outfd);
      break;
    default:
      fprintf(stdout, "\nError: refType %u in fixupp", refType);
      abort(1);
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

rd_fix_data(uint outfd, uint length, uint fd) {
  uint targetOffset;
  byte fixdata, frame, target;
  // -----------------------------------------------------------------
  //  FRAME/TARGET METHODS is a byte which encodes the methods by which 
  //  the fixup "frame" and "target" are to be determined, as follows.
  //  In the first three cases  FRAME  INDEX  is  present  in  the
  //  record  and  contains an index of the specified type. In the
  //  last two cases there is no FRAME INDEX field.
  fixdata = read_u8(fd); // Format is fffftttt
  switch ((fixdata & 0xf0) >> 4) {
    case 0:     // frame given by a segment index
      frame = read_u8(fd);
      length -= 1;
      fprintf(outfd, "Frm=Seg %u, ", frame);
      break;
    case 1:     // frame given by a group index
      frame = read_u8(fd);
      length -= 1;
      fprintf(outfd, "Frm=Grp %u, ", frame);
      break;
    case 2:     // frame given by an external index
      frame = read_u8(fd);
      length -= 1;
      fprintf(outfd, "Frm=Ext %u, ", frame);
      break;
    case 4:     // frame is that of the reference location
      fputs("Frm=Ref, ", outfd);
      break;
    case 5:     // frame is determined by the target
      fputs("Frm=Tgt, ", outfd);
      break;
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
      fprintf(outfd, "Tgt=Seg+%x", targetOffset);
      break;
    case 1 :  // target given by a group index + displacement
      targetOffset = read_u16(fd);
      length -= 2;
      fputs(outfd, "Tgt=Grp+%x", targetOffset);
      break;
    case 2 :  // target given by an external index + displacement
      targetOffset = read_u16(fd);
      length -= 2;
      fputs(outfd, "Tgt=Ext+%x", targetOffset);
      break;
    case 4 :  // target given by a segment index alone
      fputs("Tgt=Seg+0", outfd);
      break;
    case 5 :  // target given by a group index alone
      fputs("Tgt=Grp+0", outfd);
      break;
    case 6 :  // target given by an external index alone
      fputs("Tgt=Ext+0", outfd);
      break;
    default:
      printf("\nError: Unknown target method %x", fixdata & 0x0f);
      break;
  }
  length -= 3;
  return length;
}

// A0H  LEDATA Logical Enumerated Data Record
do_ledata(uint outfd, uint length, uint fd) {
    byte segindex;
    uint dataoffset;
    fputs("LEDATA ", outfd);
    segindex = read_u8(fd);
    dataoffset = read_u16(fd);
    length -= 4; // don't count the segindex, dataoffset, or checksum.
    fprintf(outfd, "SegIndex=%u DataOffset=%u Length=%u\n", segindex, dataoffset, length);
    // Data bytes
    while (length-- > 0) {
        char ch;
        ch = read_u8(fd);
        puthexch(outfd, ch);
    }
    read_u8(fd); // checksum. assume correct.
    return;
}
