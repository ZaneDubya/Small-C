/*
** putobj.c -- Generate OBJ Records
**
** Note: Many OBJ records contain repeating groups.  These have three
** functions to write a record:
** 
**     begXXXXXX() - begins (opens) the record - call once
**     putXXXXXX() - puts out one repetition   - call repeatedly
**     endXXXXXX() - ends (closes) the record  - call once
**
** Don't worry about record overflow.  The putXXXXXX() functions
** detect this and automatically close the open record and open
** a new one.  Only one OBJ record may be open at any given time.
**
** Records that do not have repeating groups have only a putXXXXXX()
** function which is called once to write the record.
**
** putFIXUPP and putTHREAD are special cases.  They only buffer
** data which is output when the currently open LEDATA or LIDATA
** record is closed.  Therefore, they can (and must) be called while
** one of those records is open.
*/
#include <stdio.h>
#include "obj1.h"
#include "obj2.h"
#include "obj3.h"

unsigned char
  *fbuf,               /* fixup buffer */
  *fnext,              /* next position in fixup buffer */
  *fend,               /* end of fixup buffer */
  *obuf;               /* output buffer */
unsigned
  currec,              /* current data record type */
  fixuse,              /* current data record's fixup use value */
  o1,                  /* current record's 1st arg */
  o2,                  /* current record's 2nd arg */
  o3,                  /* current record's 3rd arg */
  o4,                  /* current record's 4th arg */
  off[2],              /* LEDATA running offset */
  onext = 3,           /* output buffer data length */
  opref;               /* output buffer prefix length */

extern int
  ofd;                 /* output file descriptor */

/*
** write COMENT record to OBJ file
*/
putCOMENT(com, np, nl, class) char *com; int np, nl, class; {
  oopen(COMENT);
  obyte(((np << 1) | nl) << 6);
  obyte(class);
  oname(com, NO);
  oclose();
  }

/*
** write THEADR record to OBJ file
*/
putTHEADR(title) char *title; {
  if(obuf == 0) {
    if(!(obuf = malloc(OSIZE))
    || !(fbuf = malloc(FSIZE))) exit(1);
    fend = fbuf + FSIZE;
    }
  oopen(THEADR);
  oname(title, NO);
  oclose();
  }

/*
** write LNAMES record to OBJ file
*/
begLNAMES() {
  oopen(LNAMES);
  }
putLNAMES(name) char *name; {
  ocheck(LNAMES);
  if(OFREE < 32) {  /* another won't fit */
    oclose();       /* so close this record */
    oopen(LNAMES);  /* and start another one */
    }
  oname(name, YES);
  }
endLNAMES() {
  oclose();
  }

/*
** write SEGDEF record to OBJ file
*/
putSEGDEF(use, align, comb, big, frame, offset, len, snx, cnx, onx)
  unsigned use, align, comb, big, frame, offset[], len[], snx, cnx, onx; {
  oopen((use == 2) ? SEGDEF : SEG386);
  obyte((align << 5)      /* attributes */
      | ( comb << 2)
      | (  big << 1));
  if(align == A_ABS) {
    oword(&frame, 2);     /* frame */
    oword(offset, use);   /* offset */
    }
  oword(len, use);        /* length */
  oindex(snx);            /* segment name index */
  oindex(cnx);            /* class name index */
  oindex(onx);            /* overlay name index */
  oclose();
  }

/*
** write EXTDEF record to OBJ file
*/
begEXTDEF() {
  oopen(EXTDEF);
  }
putEXTDEF(name) char *name; {
  ocheck(EXTDEF);
  if(OFREE < 34) {  /* another won't fit */
    oclose();       /* so close this record */
    oopen(EXTDEF);  /* and start another one */
    }
  oname(name, NO);
  oindex(0);                      /* Type unspecified */
  }
endEXTDEF() {
  oclose();
  }

/*
** write GRPDEF record to OBJ file
*/
begGRPDEF(gnx) int gnx; {
  oopen(GRPDEF);
  oindex(o1 = gnx);
  }
putGRPDEF(segx) int segx; {
  ocheck(GRPDEF);
  if(OFREE < 3) {   /* another won't fit */
    oclose();       /* so close this record */
    begGRPDEF(o1);  /* and start another one */
    }
  obyte(0xFF);
  oindex(segx);
  }
endGRPDEF() {
  oclose();
  }

/*
** write PUBDEF record to OBJ file
*/
begPUBDEF(use, grpx, segx, frame) unsigned use, grpx, segx, frame; {
  oopen((use == 2) ? PUBDEF : PUB386);
  o1 = use;
  oindex(o2 = grpx);
  oindex(o3 = segx);
  o4 = frame;
  if(segx == 0) oword(&frame, 2);
  }
putPUBDEF(name, offset) char *name; unsigned offset[]; {
  ocheck((o1 == 2) ? PUBDEF : PUB386);
  if(OFREE < 38) {             /* another won't fit */
    oclose();                  /* so close this record */
    begPUBDEF(o1, o2, o3, o4); /* and start another one */
    }
  oname(name, NO);
  oword(offset, o1);
  oindex(0);
  }
endPUBDEF() {
  oclose();
  }

/*
** write LIDATA record to OBJ file
*/
begLIDATA(duse, fuse, segx, offset) unsigned duse, fuse, segx, offset[]; {
  begDATA(duse, fuse, segx, offset, (duse == 2) ? LIDATA : LID386);
  }
putLIDATA(rep, data, len) unsigned rep, len; char *data; {
  int zero; zero = 0;
  ocheck((o1 == 2) ? LIDATA : LID386);
  if(OFREE < (len + 5)) {          /* another won't fit */
    endDATA();                             /* so close this record */
    begDATA(o1, fixuse, o2, off, LIDATA);  /* and start another one */
    }
  oword(&rep, 2); if(o1 > 2) oword(&zero, 2);
  oword(&zero, 2);
  obyte(len);
  while(len--) obyte(*data++);
  inc32(off, rep*len);      /* bump running offset */
  }
endLIDATA() {
  endDATA();
  }

/*
** write LEDATA record to OBJ file
*/
begLEDATA(duse, fuse, segx, offset) unsigned duse, fuse, segx, offset[]; {
  begDATA(duse, fuse, segx, offset, (duse == 2) ? LEDATA : LED386);
  }
putLEDATA(data, len) char *data; unsigned len; {
  ocheck((o1 == 2) ? LEDATA : LED386);
  if(OFREE < len && len <= o1) {      /* may FIXUP, so don't span records */
    endDATA();                             /* close this record */
    begDATA(o1, fixuse, o2, off, LEDATA);  /* and start another one */
    }
  while(len--) {
    if(OFREE < 1) {                         /* another byte won't fit so */
      endDATA();                            /* close this record */
      begDATA(o1, fixuse, o2, off, LEDATA); /* and start another one */
      }
    obyte(*data++);
    inc32(off, 1);                          /* bump running offset */
    }
  }
endLEDATA() {
  endDATA();
  }

/*
** begin either type DATA record
*/
begDATA(duse, fuse, segx, offset, rec)
  unsigned duse, fuse, segx, offset[], rec; {
  oopen(rec);
  o1 = duse;
  oindex(o2 = segx);
  off[0] = offset[0];
  if(duse > 2) off[1] = offset[1];
  else         off[1] = 0;
  oword(off, duse);
  opref = onext;        /* note length of prefix */
  fnext = fbuf;         /* init fixup buffer */
  currec = rec;         /* note current rec type */
  fixuse = fuse;        /* note current fixup use value */
  }

/*
** end either type DATA record
*/
endDATA() {
  unsigned len, fix;
  unsigned char *f;
  oclose();
  if(fbuf < fnext) {
    fix = (fixuse == 2)           /* set current fixup record type */
        ? FIXUPP : FIX386;
    oopen(fix);                   /* open fixup record */
    f = fbuf;
    while(f < fnext) {            /* write bufferred fixups */
      len = *f++;
      if(OFREE < len) {           /* another won't fit */
        oclose();                 /* so close this record */
        oopen(fix);               /* and start another one */
        }
      while(--len) obyte(*f++);   /* decr first since len is 1 high */
      }
    oclose();                     /* close final fixup record */
    }
  }

/*
** put a fixup TRHEAD entry in the fixup buffer
*/
putTHREAD(thr, mth, ndx) int thr, mth, ndx; {
  unsigned char *len;
  if((fend - fnext) < 4) {        /* another may not fit */
    endDATA();                    /* so close & fixup current data rec */
    begDATA(o1, fixuse, o2, off, currec); /* and start another one */
    }
  len = fnext++;                  /* skip and note length byte */
  fbyte((mth << 2) |  thr);  /* TRD DAT byte */
  if((mth & 7) < 3) findex(ndx);
  *len = fnext - len;             /* set length of entry */
  }

/*
** write LEDATA item and put a fixup FIXUPP entry in the fixup buffer
*/
putLEDFIX(dat, len, use, mod, loc, fra, fdat, tar, tdat, tdis)
  char dat[];
  int len, use, mod, loc, fra, fdat, tar, tdat, tdis[]; {
  unsigned char *sz, *offset;
  if((fend - fnext) < 12) {       /* another may not fit */
    endDATA();                    /* so close & fixup this rec */
    begDATA(o1, fixuse, o2, off, currec); /* and start another one */
    }
  putLEDATA(dat, len);            /* may overflow too */
  offset = onext - len - opref;   /* now calc obj rec offset */
  sz = fnext++;                   /* skip and note size byte */
  fbyte(0x80                      /* 1st LOCAT Byte */
      | (mod << 6)
      | (loc << 2)
      |((offset >> 8) & 3));
  fbyte(offset);                  /* 2nd LOCAT Byte */
  fbyte((fra << 4) |  tar);       /* FIX DAT Byte */
  if(fra < 3) findex(fdat);
  if(tar < 8) findex(tdat);
  if(tar < 4) fword(tdis, use);
  *sz = fnext - sz;               /* set size of entry */
  }

/*
** write MODEND record to OBJ file
** (use FIXUPP parameters for "fra" and "tar")
*/
putMODEND(use, attr, fra, fdat, tar, tdat, tdis)
      int use, attr, fra, fdat, tar, tdat, tdis[]; {
  oopen((use == 2) ? MODEND : MOD386);
  obyte((attr << 6) | 1);
  if(attr & 1) {               /* give address */
    obyte((fra << 4) |  tar);  /* END DAT Byte */
    if(fra < 3) oindex(fdat);
    if(tar < 8) oindex(tdat);
    if(tar < 4) oword(tdis, use);
    }
  oclose();
  }

/*
** open an output OBJ record
*/
oopen(type) unsigned type; {
  obuf[0] = type;              /* set record type */
  onext = opref = 3;           /* reset for new record */
  }

/*
** check that the same type of OBJ record is open
*/
ocheck(type) unsigned type; {
  if(obuf[0] != type) error("+ OBJ Record Conflict");
  }

/*
** close an output OBJ record
*/
oclose() {
  unsigned i;
  unsigned char chksum;
  putint(obuf + 1, onext - 2); /* set record length */
  chksum = i = 0;              /* calc check sum */
  while(i < onext)  chksum += obuf[i++];
  obyte(-chksum);              /* set check sum */
  write(ofd, obuf, onext);     /* write record */
  obuf[0] = 0;                 /* designate record closed */
  }

/*
** output a name field into the OBJ record
*/
oname(name, up) char name[]; int up; {
  unsigned i, first;
  i = 0;
  first = onext++;                /* make room for length */
  while(name[i] && i < 127)       /* copy name */
    obyte(up ? toupper(name[i++]) : name[i++]);
  obuf[first] = i;                /* store length */
  }

/*
** output an index field into the OBJ record
*/
findex(ndx) unsigned ndx; {
  if(ndx > 127)                 /* two byte index */
    fbyte((ndx >> 8) | 0x80);   /* high 7 bits first */
  fbyte(ndx);                   /* low (or only) 8 bits */
  }

/*
** output a word field (of any length) into the OBJ record
*/
fword(word, sz) char word[]; int sz; {
  int i;  i = 0;
  while(i < sz) fbyte(word[i++]);
  }

/*
** output a byte field into the OBJ record
*/
fbyte(byte) char byte; {
  *fnext++ = byte;
  }

/*
** output an index field into the OBJ record
** return index length
*/
oindex(ndx) unsigned ndx; {
  if(ndx > 127)                 /* two byte index */
    obyte((ndx >> 8) | 0x80);   /* high 7 bits first */
  obyte(ndx);                   /* low (or only) 8 bits */
  }

/*
** output a word field (of any length) into the OBJ record
*/
oword(word, sz) char word[]; int sz; {
  int i;  i = 0;
  while(i < sz) obyte(word[i++]);
  }

/*
** output a byte field into the OBJ record
*/
obyte(byte) char byte; {
  if(onext < OSIZE) obuf[onext++] = byte;
  else error("+ obuf[] overflow");
  }

