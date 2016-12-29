// Definitions for the various Object Module Formats
#include "stdio.h"
#include "link.h"

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
#define LIBDEP 0xF2

char twotabs[] = "\n    ";
extern char *line;
byte isLibrary = 0;
uint libDictOffset[2],  libDictSize;

do_record(byte recType, uint length, uint fd) {
    prnhexch(recType);
    fputs("  ", stdout);
    /* prnhexch(length / 256, stdout);
    prnhexch(length & 0x00ff, stdout);
    fputc(' ', stdout); */
    switch (recType) {
        case THEADR:
            do_theadr(length, fd);
            break;
        case COMMNT:
            do_commnt(length, fd);
            break;
        case MODEND:
            do_modend(length, fd);
            if (isLibrary) {
                do_library(fd);
            }
            break;
        case EXTDEF:
            do_extdef(length, fd);
            break;
        case PUBDEF:
            do_pubdef(length, fd);
            break;
        case LNAMES:
            do_lnames(length, fd);
            break;
        case SEGDEF:
            do_segdef(length, fd);
            break;
        case FIXUPP:
            do_fixupp(length, fd);
            break;
        case LEDATA:
            do_ledata(length, fd);
            break;
        case LIDATA:
            do_lidata(length, fd);
            break;
        case LIBHDR:
            do_libhdr(length, fd);
            break;
        case LIBDEP:
            do_libdep(length, fd);
            break;
        default:
            printf("Unknown record of type %x", recType);
            printf("Exiting...");
            fgetc(stdin);
            break;
    }
    fputc(NEWLINE, stdout);
}

clearrecord(uint length, uint fd) {
    while (length-- > 0) {
        byte ch;
        ch = read_u8(fd);
        prnhexch(ch);
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
do_theadr(uint length, uint fd) {
    read_strp(line, fd);
    printf("THEADR %s", line);
    read_u8(fd); // checksum. assume correct.
    return;
}

// A2H LIDATA Logical Iterated Data Record
// Like the LEDATA record, the LIDATA record contains binary data—executable
// code or program data. The data in an LIDATA record, however, is specified
// as a repeating pattern (iterated), rather than by explicit enumeration.
// The data in an LIDATA record can be modified by the linker if the LIDATA
// record is followed by a FIXUPP record, although this is not recommended.
do_lidata(uint length, uint fd) {
    fputs("LIDATA ", stdout);
    prnhexint(length, stdout);
    while (length-- > 0) {
        read_u8(fd);
    }
    return;
}

// 88H COMMNT Comment Record.
do_commnt(uint length, uint fd) {
    byte commtype, commclass;
    fputs("COMMNT ", stdout);
    commtype = read_u8(fd);
    commclass = read_u8(fd);
    switch (commclass) {
        case 0xA3:
            read_strp(line, fd);
            printf("LIBMOD=%s", line);
            break;
        default:
            fputs("(UNKNOWN TYPE)", stdout);
            clearrecord(length - 3, fd);
            break;
    }
    read_u8(fd); // checksum. assume correct.
    return;
}

// 8AH MODEND Module End Record
// The MODEND record denotes the end of an object module. It also indicates
// whether the object module contains the main routine in a program, and it
// can optionally contain a reference to a program's entry point.
do_modend(uint length, uint fd) {
    byte moduletype;
    fputs("MODEND ", stdout);
    moduletype = read_u8(fd); // format is MS0....1
    if (moduletype & 0x80) {
        fputs("Main ", stdout);
        abort(1);
    }
    if (moduletype & 0x40) {
        fputs("Start ", stdout);
        abort(1);
    }
    read_u8(fd); // checksum. assume correct.
    return;
}

// 8CH EXTDEF External Names Definition Record
// The EXTDEF record contains a list of symbolic external references—that is,
// references to symbols defined in other object modules. The linker resolves
// external references by matching the symbols declared in EXTDEF records
// with symbols declared in PUBDEF records.
do_extdef(uint length, uint fd) {
    byte deftype, linelength, strlength;
    fputs("EXTDEF ", stdout);
    linelength = 11;
    while (length > 1) {
        strlength = read_strpre(line, fd);
        length -= strlength + 1;
        if (linelength + strlength + 1 >= 80) {
            fputs(twotabs, stdout);
            linelength = 4;
        }
        linelength += strlength + 1;
        fputs(line, stdout);
        deftype = read_u8(fd);
        if (deftype != 0) {
            fputs("Error: Type is not 0. ", stdout);
            abort(1);
        }
        fputs(", ", stdout);
    }
    read_u8(fd); // checksum. assume correct.
    return;
}

// 90H PUBDEF Public Names Definition Record
// The PUBDEF record contains a list of public names.  It makes items defined
// in this object module available to satisfy external references in other
// modules with which it is bound or linked. The symbols are also available
// for export if so indicated in an EXPDEF comment record.
do_pubdef(uint length, uint fd) {
    byte basegroup, basesegment, typeindex;
    uint puboffset;
    fputs("PUBDEF ", stdout);
    // BaseGroup and BaseSegment fields contain indexes specifying previously
    // defined SEGDEF and GRPDEF records.  The group index may be 0, meaning
    // that no group is associated with this PUBDEF record.
    // BaseFrame field is present only if BaseSegment field is 0, but the
    // contents of BaseFrame field are ignored.
    // BaseSegment idx is normally nonzero and no BaseFrame field is present.
    basegroup = read_u8(fd);
    fputs("BaseGroup=", stdout);
    prnhexch(basegroup);
    if (basegroup != 0) {
        fputs(" Error: Must be 0.", stdout);
        abort(1);
    }
    basesegment = read_u8(fd);
    fputs(" BaseSegment=", stdout);
    prnhexch(basesegment);
    if (basesegment == 0) {
        fputs(" Error: Must be nonzero.", stdout);
        abort(1);
    }
    length -= 2;
    fputs(twotabs, stdout);
    while (length > 1) {
        length -= read_strpre(line, fd) + 3;
        fputs(line, stdout);
        puboffset = read_u16(fd);
        fputc('@', stdout);
        prnhexint(puboffset, stdout);
        typeindex = read_u8(fd);
        if (typeindex != 0) {
            fputs("Error: Type is not 0. ", stdout);
            abort(1);
        }
        fputs(", ", stdout);
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

do_lnames(uint length, uint fd) {
    fputs("LNAMES ", stdout);
    while (length > 1) {
        length -= read_strpre(line, fd);
        fputc('"', stdout);
        fputs(line, stdout);
        fputs("\" ", stdout);
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
do_segdef(uint length, uint fd) {
    byte segattr, segname, classname, overlayname;
    uint seglength;
    fputs("SEGDEF ", stdout);
    // segment attribute
    segattr = read_u8(fd);
    if ((segattr & 0xe0) != 0x60) {
        fputs("Unknown segment attribute field (must be 0x03).", stdout);
        abort(1);
    }
    if ((segattr & 0x1c) != 0x08) {
        fputs("Unknown segment acombination (must be 0x02).", stdout);
        abort(1);
    }
    if ((segattr & 0x02) != 0x00) {
        fputs("Attribute may not be big (flag 0x02).", stdout);
    }
    if ((segattr & 0x01) != 0x00) {
        fputs("Attribute must be 16-bit addressing (flag 0x01).", stdout);
    }
    seglength = read_u16(fd);
    segname = read_u8(fd);
    classname = read_u8(fd);
    overlayname = read_u8(fd);
    printf("Length=%u Name=%u Class=%u Overlay=%u", 
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
do_fixupp(uint length, uint fd) {
    uint dataOffset, targetOffset;
    byte fixup, fixdata, frame, target;
    byte relativeMode; // 1 == segment relative, 0 == self relative.
    byte location; 
    fputs("FIXUPP ", stdout);
    while (length > 1) {
        fputs(twotabs, stdout);
        length -= 3;
        fixup = read_u8(fd);
        fputs("Fixup=", stdout);
        prnhexch(fixup);
        if ((fixup & 0x80) == 0) {
            fputs("Error: must be fixup, not thread.", stdout);
            abort(1);
        }
        relativeMode = (fixup & 0x40) != 0;
        location = (fixup & 0x3c) >> 2;
        dataOffset = read_u8(fd) + ((fixup & 0x03) << 8);
        fixdata = read_u8(fd); // Format is FfffTPtt
        fputc(' ', stdout);
        prnhexch(fixdata);
        if ((fixdata & 0x80) != 0) {
            frame = (fixdata & 0x70) >> 4;
        }
        else {
            frame = read_u8(fd);
            length -= 1;
            fputc(' ', stdout);
            prnhexch(frame);
        }
        if ((fixdata & 0x08) != 0) {
            printf(" Error: target threads not handled (%x)", fixdata);
            abort(1);
        }
        else {
            target = read_u8(fd);
            length -= 1;
            fputc(' ', stdout);
            prnhexch(target);
        }
        if ((fixdata & 0x04) == 0) {
            targetOffset = read_u16(fd);
            length -= 2;
            fputc(' ', stdout);
            prnhexint(targetOffset, stdout);
        }
    }
    read_u8(fd); // checksum. assume correct.
    return;
}

// A0H  LEDATA Logical Enumerated Data Record
do_ledata(uint length, uint fd) {
    byte segindex;
    uint dataoffset;
    fputs("LEDATA ", stdout);
    segindex = read_u8(fd);
    dataoffset = read_u16(fd);
    printf("SegIndex=%u DataOffset=%u", segindex, dataoffset);
    // Data bytes
    // fputs(twotabs, stdout);
    length -= 4;
    while (length-- > 0) {
        read_u8(fd);
    }
    read_u8(fd); // checksum. assume correct.
    return;
}

// F0H  LIBHDR Library Header Record
do_libhdr(uint length, uint fd) {
    byte flags;
    fputs("LIBHDR ", stdout);
    libDictOffset[0] = read_u16(fd);
    libDictOffset[1] = read_u16(fd);
    libDictSize = read_u16(fd);
    flags = read_u8(fd);
    printf("DictOffset=%u+(%ux2^16) DictSize=%u Flags=%x ", 
        libDictOffset[0], libDictOffset[1], libDictSize, flags);
    length -= 8;
    while (length-- > 0) {
        read_u8(fd);
    }
    read_u8(fd); // checksum. assume correct.
    isLibrary = 1;
    return;
}

//   The remaining blocks in the library compose the dictionary. The number of
// blocks in the dictionary is given in the library header. The dictionary
// provides rapid searching for a name using a two-level hashing scheme. The
// number of ductionary blocks must be a prime number, each block containing
// exactly 37 dictionary indexes. A block is 512 bytes long. The first 37 bytes
// are indexes to block entries (multiply value by 2 to get offset to entry). 
// Byte 38 is an index to empty space in the block (multiply value by 2 to get
// offset to the next available space. If byte 38 is 255 the block is full.
//   Each entry is a length-prefixed string, followed by a two-byte LE mdoule 
// number in which the module in the library defining this string can be found.
do_library(uint fd) {
    byte offsets[38];
    uint iBlock, iEntry, entryBlock;
    uint blockOffset[2];
    printf("Library: dictionary at %u %u", libDictOffset[0], libDictOffset[1]);
    for (iBlock = 0; iBlock < libDictSize; iBlock++) {
        printf("\nLibrary Block %u ", iBlock);
        // advance to the dictionary block
        blockOffset[0] = iBlock * 512;
        blockOffset[1] = 0;
        offsetfd(fd, libDictOffset, blockOffset);
        read(fd, offsets, 38);
        for (iEntry = 0; iEntry < 37; iEntry++) {
            if (offsets[iEntry] == 0) {
                printf("%u=EMPTY ", iEntry);
            }
            else {
                blockOffset[0] = (iBlock * 512) + (offsets[iEntry] * 2);
                offsetfd(fd, libDictOffset, blockOffset);
                read_strp(line, fd);
                entryBlock = read_u16(fd);
                printf("%x=%s[%x] ", iEntry, line, entryBlock);
            }
        }
        blockOffset[0] = iBlock * 512;
        blockOffset[1] = 0;
        offsetfd(fd, libDictOffset, blockOffset);
    }
    blockOffset[0] = libDictSize * 512;
    offsetfd(fd, libDictOffset, blockOffset);
}

// F2H Extended Dictionary
//   The extended dictionary is optional and indicates dependencies between
// modules in the library. Versions of LIB earlier than 3.09 do not create an
// extended dictionary. The extended dictionary is placed at the end of the
// library. 
do_libdep(uint length, uint fd) {
    uint modCount, i;
    uint modPage, modOffset;
    modCount = read_u16(fd);
    printf("Libray Dependency. Module count %u\n", modCount);
    for (i = 0; i <= modCount; i++) {
        modPage = read_u16(fd);
        modOffset = read_u16(fd);
        printf("Module=%x %x ", modPage, modOffset);
        length -= 4;
    }
    clearrecord(length - 2, fd);
    return;
}