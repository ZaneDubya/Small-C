// Definitions for the various Object Module Formats
#include "stdio.h"

#define THEADR 0x80
#define MODEND 0x8A
#define EXTDEF 0x8C
#define PUBDEF 0x90
#define LNAMES 0x96
#define SEGDEF 0x98
#define FIXUPP 0x9C
#define LEDATA 0xA0

char twotabs[] = "\n        ";
extern char *line;

do_record(unsigned char recType, unsigned int length, int fd) {
    prnhexch(recType, stdout);
    fputc(' ', stdout);
    prnhexch(length / 256, stdout);
    prnhexch(length & 0x00ff, stdout);
    fputc(' ', stdout);
    switch (recType) {
        case THEADR:
            do_theadr(length, fd);
            break;
        case MODEND:
            do_modend(length, fd);
            clearrecord(length, fd);
            break;
        case EXTDEF:
            do_extdef(length, fd);
            clearrecord(length, fd);
            break;
        case PUBDEF:
            do_pubdef(length, fd);
            clearrecord(length, fd);
            break;
        case LNAMES:
            do_lnames(length, fd);
            break;
        case SEGDEF:
            do_segdef(length, fd);
            break;
        case FIXUPP:
            do_fixupp(length, fd);
            clearrecord(length, fd);
            break;
        case LEDATA:
            do_ledata(length, fd);
            clearrecord(length, fd);
            break;
        default:
            fputs("UNKNOWN ", stdout);
            fgetc(stdin);
            clearrecord(length, fd);
            break;
    }
    fputc(NEWLINE, stdout);
}

clearrecord(unsigned int length, int fd) {
    while (length-- > 0) {
        unsigned char ch;
        ch = read_u8(fd);
        prnhexch(ch, stdout);
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
do_theadr(unsigned int length, int fd) {
    fputs("THEADR ", stdout);
    fputs(twotabs, stdout);
    read_strp(line, fd);
    fputs(line, stdout);
    read_u8(fd); // checksum. assume correct.
    return;
}

// 8AH MODEND Module End Record
// The MODEND record denotes the end of an object module. It also indicates
// whether the object module contains the main routine in a program, and it
// can optionally contain a reference to a program's entry point.
do_modend(unsigned int length, int fd) {
    fputs("MODEND ", stdout);
    return;
}

// 8CH EXTDEF External Names Definition Record
do_extdef(unsigned int length, int fd) {
    fputs("EXTDEF ", stdout);
    return;
}

// 90H PUBDEF Public Names Definition Record
do_pubdef(unsigned int length, int fd) {
    fputs("PUBDEF ", stdout);
    return;
}

// 96H LNAMES List of Names Record
do_lnames(unsigned int length, unsigned int fd) {
    fputs("LNAMES ", stdout);
    fputs(twotabs, stdout);
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
do_segdef(unsigned int length, int fd) {
    unsigned char segattr, segname, classname, overlayname;
    unsigned int seglength;
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
        fputs("Attribute must be 16-bit addressing.", stdout);
    }
    // segment length
    seglength = read_u16(fd);
    fputs("Length=", stdout);
    prnhexint(seglength, stdout);
    // name index
    segname = read_u8(fd);
    fputs(" Name=", stdout);
    prnhexch(segname, stdout);
    // class name
    classname = read_u8(fd);
    fputs(" Class=", stdout);
    prnhexch(classname, stdout);
    // overlay name
    overlayname = read_u8(fd);
    fputs(" Overlay=", stdout);
    prnhexch(overlayname, stdout);
    read_u8(fd); // checksum. assume correct.
    return;
}

// 9CH FIXUPP Fixup Record
do_fixupp(unsigned int length, int fd) {
    fputs("FIXUPP ", stdout);
    return;
}

// A0H  LEDATA Logical Enumerated Data Record
do_ledata(unsigned int length, int fd) {
    fputs("LEDATA ", stdout);
    return;
}