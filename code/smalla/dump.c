/*
** dump.c -- hex file dump with formatting of OBJ and EXE files
**
**  If no filename is given on the commnad line, the user is prompted
**  for the file to be dumped.  Output goes to the standard
**  output file and is, therefore, redirectable to any output
**  device or to a disk file.  Drive specifiers may be given.
**  Any keystroke will pause/resume execution.  Control-C will
**  cancel execution.
*/
#include <stdio.h>
#include "notice.h"
#include "obj3.h"

#define ANY   0
#define OBJ   1
#define LIB   2
#define EXE   3

unsigned int
  i, fd, one[2] = {1, 0}, off[2], loc, ext, word, rtcnt, bytes, eom,
  chk=NO, chknow=NO, chkword, chksum, rectyp,  reclen,  list = YES;
unsigned char
  fn[41], str1[5], str2[5], ascii[17], byte, *rtoff, *lmoff, *cp;

main(argc, argv) int argc, *argv; {
  fputs("Small Assembler Dump Utility, ", stderr);
  fputs(VERSION, stderr);
  fputs(CRIGHT1, stderr);
  if(getarg(1, fn, 41, argc, argv) == EOF) {
    if(!reqstr("file.ext: ", fn, 41)) exit(0);
    }
  else {
    if(*fn == '-' || *fn == '/')  {
      fputs("\nUsage: DUMP [file.ext]\n", stderr);
      abort(7);
      }
    }
  if(!(fd = fopen(fn, "r"))) {
    fputs("\ncan't find: ", stderr);
    fputs(fn, stderr);
    fputs("\n", stderr);
    abort(7);
    }
  if(cp = strchr(fn, '.')) {
    if(lexcmp(cp, ".OBJ") == 0) ext = OBJ;
    if(lexcmp(cp, ".LIB") == 0) ext = LIB;
    if(lexcmp(cp, ".EXE") == 0) ext = EXE;
    }
  puts2("file: ", fn);
  chksum = 0;
  switch(ext) {
    case OBJ: doobj(); break;
    case LIB: doobj(); break;
    case EXE: doexe();
    case ANY: doany();
    }
  }

doobj() {
  do {
    if(getobj() == ERR)  abort(7);
    } while(!feof(fd));
  }

doexe() {
  puts("");
  chk = YES;
  i = 0;
  while (++i <= 14) headexe();
  i = rtoff - 28;
  while(i--) {
    if(read(fd, &byte, 1) != 1) exerr();
    sum();
    itox(byte, str1, 3); zfill(str1);
    fputs(str1, stdout); fputc(' ', stdout);
    }
  fputs("\nrelocation table:\n", stdout);
  i = 0;
  while(++i <= rtcnt) {
    if(read(fd, &word, 2) != 2) exerr();
    chksum += word;
    itox(word, str2, 5); zfill(str2);
    if(read(fd, &word, 2) != 2) exerr();
    chksum += word;
    itox(word, str1, 5); zfill(str1);
    fputs(str1, stdout); fputc(':', stdout); fputs(str2, stdout);
    if(i % 8 && i != rtcnt) fputc(' ', stdout);
    else fputc('\n', stdout);
    }
  if(iscons(stdout)) reqstr("\nwaiting...", str1, 5);
  i = lmoff - rtoff - 4*rtcnt;
  while(i--) {
    if((read(fd, &byte, 1) != 1) || byte) exerr();
    }
  }

doany() {
  loc = i = 0;
  while(read(fd, &byte, 1) == 1) {
    sum();
    if(byte < 128 && isprint(byte))
         ascii[i] = byte;
    else ascii[i] = '.';
    if(++i == 1) {
      if((loc % 256) == 0)  puts("");
      itox(loc, str1, 5); zfill(str1);
      fputs(str1, stdout);
      }
    if(i == 9) fputc('-', stdout);
    else       fputc(' ', stdout);
    itox(byte, str1, 3); zfill(str1);
    fputs(str1, stdout);
    if(i == 16) xlate();
    ++loc;
    pause();
    }
  fclose(fd);
  if(i) xlate();
  if(chk) {
    itox(chksum, str1, 5); zfill(str1);
    fputs("\nchksum: ", stdout); puts(str1);
    }
  puts("");
  }

headexe() {
  if(read(fd, &word, 2) != 2) exerr();
  itox(word, str1, 5);
  chksum += word;
  switch(i) {
    case  1: puts2(str1, " EXE signature");
             if(word != 23117) exerr();
             break;
    case  2: puts2(str1, " bytes in last page"); break;
    case  3: puts2(str1, " 512-byte pages in file"); break;
    case  4: puts2(str1, " relocation entries");
             rtcnt = word;
             break;
    case  5: puts2(str1, " paragraphs in header");
             lmoff = word << 4;
             break;
    case  6: puts2(str1, " min paragraphs after program"); break;
    case  7: puts2(str1, " max paragraphs after program"); break;
    case  8: puts2(str1, " SS before relocation"); break;
    case  9: puts2(str1, " SP"); break;
    case 10: puts2(str1, " negative chksum"); break;
    case 11: puts2(str1, " IP"); break;
    case 12: puts2(str1, " CS before relocation"); break;
    case 13: puts2(str1, " bytes to relocation table");
             rtoff = word;
             break;
    case 14: puts2(str1, " overlay number"); break;
    }
  }

exerr() {
  error("not a valid EXE file");
  }

sum() {
  if(chknow) {
    chknow = NO;
    chkword = (byte << 8) | chkword;
    chksum += chkword;
    }
  else {
    chknow = YES;
    chkword = byte;
    }
  }

xlate() {
  ascii[i] = NULL;
  while(i++ < 16) fputs("   ", stdout);
  printf(" %s\n", ascii);
  i = 0;
  }

zfill(str) char *str; {
  while(*str == ' ') *str++ = '0';
  }

getobj() {
  chksum = eom = 0;
       if((rectyp = getbyte(NO)) == ERR) rectyp = EOBJ;
  else if((reclen = getword(NO)) == ERR) return (ERR);
  switch(rectyp) {
    case   EOBJ: return(NO);
    case LHEADR: getLHEADR(   ); break;
    case LFOOTR: getLFOOTR(   ); break;
    case THEADR: getTHEADR(   ); break;
    case LNAMES: getLNAMES(   ); break;
    case SEGDEF: getSEGDEF(NO ); break;
    case SEG386: getSEGDEF(YES); break;
    case GRPDEF: getGRPDEF(   ); break;
    case TYPDEF: getTYPDEF(   ); break;
    case PUBDEF: getPUBDEF(NO ); break;
    case PUB386: getPUBDEF(YES); break;
    case EXTDEF: getEXTDEF(   ); break;
    case LINNUM: getLINNUM(   ); break;
    case LEDATA: getLEDATA(NO ); break;
    case LED386: getLEDATA(YES); break;
    case LIDATA: getLIDATA(NO ); break;
    case LID386: getLIDATA(YES); break;
    case FIXUPP: getFIXUPP(NO ); break;
    case FIX386: getFIXUPP(YES); break;
    case MODEND: getMODEND(NO ); eom = 1; break;
    case MOD386: getMODEND(YES); eom = 1; break;
    case COMENT: getCOMENT(   ); break;
        default: getANY(rectyp);
    }
  getbyte(YES);
  if(reclen) {
    puts("\n- Bad record length");
    return(ERR);
    }
  if(chksum)  printf("\n- Bad checksum: %2x\n", chksum);
  if(eom && ext == LIB) while(bytes & 0x000F) getbyte(NO);
  return(YES);
  }

getbyte(chk) int chk; {
  unsigned char ch;
  if(read(fd, &ch, 1) != 1) {             /* get next byte */
    if(chksum)  puts("\n- Abnormal End of File");
    return(ERR);                          /* failure */
    }
  ++bytes;
  if(chk)  reclen--;
  chksum = (chksum + ch) & 255;
  return(ch);                             /* success */
  }

getword(chk) int chk; {
  return(getbyte(chk) | (getbyte(chk) << 8));
  }

getindex() {
  unsigned char ch;
  int index;
  ch = getbyte(YES);
  if(ch > 127)  index = ((ch & 127) << 8) | (getbyte(YES));
  else          index = ch;
  return(index);
  }

getname(s) char *s; {
  int len;
  len = getbyte(YES);
  while(len--)  *s++ = getbyte(YES);
  *s = 0;
  }

getLFOOTR() {
  int first;  first = YES;
  printf("\nLIBRARY FOOTER RECORD:");
  i = 0;
  while(read(fd, &byte, 1) == 1) {      /* get next byte */
   if(byte < 128 && isprint(byte))
         ascii[i] = byte;
    else ascii[i] = '.';
    if(++i == 1 && first) {first = NO; puts("");}
    if(  i == 9) fputc('-', stdout);
    else         fputc(' ', stdout);
    itox(byte, str1, 3); zfill(str1);
    fputs(str1, stdout);
    if(i == 16) xlate();
    pause();
    }
  if(i) xlate();
  reclen = chksum = 0;
  }

getLHEADR() {
  printf("\nLIBRARY HEADER RECORD:");
  whatever();
  chksum = 0;
  }

getTHEADR() {
  char name[128];
  getname(name);
  printf("\nT-MODULE HEADER RECORD: module name = %s\n", name);
  }

getLNAMES() {
  char name[128];
  int count;  count = 0;
  puts("\nLIST OF NAMES RECORD:");
  while(reclen > 1) {
    getname(name);
    count++;
    printf("index =%4x    name = %s\n", count, name);
    }
  }

getSEGDEF(is386) int is386; {
  char acbp, a, c, b, p;
  int frmnum, offset, seglen[2], segnam, classnam, ovrlaynam;
  acbp = getbyte(YES);
  a = (acbp & 224) >> 5;
  c = (acbp &  28) >> 2;
  b = (acbp &   2) >> 1;
  p = (acbp &   1);
  if(a == 0) {
    frmnum = getword(YES);
    offset = getword(YES);
    }
  getoff(is386);
  segnam = getindex();
  classnam = getindex();
  ovrlaynam = getindex();
  fputs("\nSEGMENT DEFINITION RECORD", stdout);
  if(is386) puts(" (386):");
  else      puts(":");
  printf("Name Indexes:  Segment = %u  Class = %u  Overlay = %u\n",
          segnam, classnam, ovrlaynam);
  printf("Segment Length = "); putoff(is386, YES);
  printf("Attributes: ",);
  if(a == 0) printf("absolute alignment:  Frame = %4x  Offset = %4x\n",
                     frmnum, offset);
  else if(a == 1)  puts("BYTE aligned (1)");
  else if(a == 2)  puts("WORD aligned (2)");
  else if(a == 3)  puts("PARA aligned (3)");
  else if(a == 4)  puts("PAGE aligned (4)");
  else             printf("Invalid Align Type: %u\n", a);
  fputs("            ", stdout);
       if(c == 0)  puts("Not to be combined (0)");
  else if(c == 2)  puts("combine as PUBLIC (2)");
  else if(c == 4)  puts("combine as PUBLIC (4)");
  else if(c == 5)  puts("combine as STACK (5)");
  else if(c == 6)  puts("combine as COMMON (6)");
  else if(c == 7)  puts("combine as PUBLIC (7)");
  else             printf("Invalid Combine Type (%u)\n", c);
  fputs("            ", stdout);
  if(b == 0)       puts("length < 64K");
  else             puts("length = 64K");
  pause();
  }

getGRPDEF() {
  unsigned char desc1;
  unsigned int  desc2, grpnam;
  puts("\nGROUP DEFINITION RECORD:");
  grpnam = getindex();
  printf("group name index = %u\n", grpnam);
  while(reclen > 1) {
    desc1 = getbyte(YES);
    desc2 = getindex(YES);
    printf("      descr type = %2x  segment index = %u\n", desc1, desc2);
    pause();
    }
  }

getTYPDEF() {
  unsigned char en, ld, name[128];
  int cnt;
  puts("\nTYPE DEFINITION RECORD:");
  getname(name);
  printf("name = %s\n", name);
  while(reclen > 1) {
    en = getbyte(YES);
    printf("eight leaf descriptor = %2x", en);
    for(cnt = 0; (cnt < 7) && (reclen > 1); cnt++) {
      ld = getbyte(YES);
      printf("  %2x",ld);
      if(ld > 128)  printf(" %4x", getword(YES));
      if(ld > 129)  printf(" %2x", getbyte(YES));
      if(ld > 132)  printf(" %2x", getbyte(YES));
      }
    pause();
    }
  puts("");
  }

getPUBDEF(is386) int is386; {
  char name[128];
  int grpndx, segndx, typndx;
  if(is386)
       puts("\nPUBLIC NAMES DEFINITION RECORD (386):");
  else puts("\nPUBLIC NAMES DEFINITION RECORD:");
  grpndx = getindex();
  segndx = getindex();
  if(grpndx || segndx)
    printf("group index = %4x  segment index = %4x\n",
            grpndx, segndx);
  else
    printf("group index = %4x  segment index = %4x  frame number = %4x\n",
            grpndx, segndx, getword(YES));
  while(reclen > 1) {
    getname(name);
    getoff(is386);
    typndx = getindex();
    printf(" offset = "); putoff(is386, NO);
    printf(" type index = %4x  public Name = %s\n", typndx, name);
    pause();
    }
  }

getEXTDEF() {
  char name[128];
  puts("\nEXTERNAL NAMES DEFINITION RECORD:");
  while(reclen > 1) {
    getname(name);
    printf(" type index = %4x   name = %s\n", getindex(), name);
    pause();
    }
  }

getLINNUM() {
  puts("\nLINE NUMBERS RECORD:");
  printf("group index = %4x  segment index = %4x\n",
         getindex(), getindex());
  while(reclen > 1) {
    printf("line number = %4x   offset = %4x\n", getword(YES), getword(YES));
    pause();
    }
  }

getLEDATA(is386) int is386; { 
  unsigned ndx;
  if(is386) fputs("\nLEDATA RECORD (386): ", stdout);
  else      fputs("\nLEDATA RECORD:       ", stdout);
  ndx    = getindex();
  getoff(is386);
  printf("segment index = %4x  offset = ", ndx);  putoff(is386, YES);
  dump(reclen-1, 0);
  }

getLIDATA(is386) int is386; { 
  unsigned ndx;
  if(is386) fputs("\nLIDATA RECORD (386): ", stdout);
  else      fputs("\nLIDATA RECORD:       ", stdout);
  ndx    = getindex();
  getoff(is386);
  printf("segment index = %4x  offset = ", ndx);  putoff(is386, YES);
  while(reclen > 1)  getidb(2, is386);
  }

getoff(is386) int is386; {
  off[0] = getword(YES);
  if(is386) off[1] = getword(YES);
  else      off[1] = 0;
  }

putoff(is386, cr) int is386, cr; {
  if(is386) {
    itox(off[1], str1, 5);
    zfill(str1);
    fputs(str1, stdout);
    }
  itox(off[0], str1, 5);
  zfill(str1);
  fputs(str1, stdout);
  if(cr) puts("");
  }

getidb(t, is386) int t, is386; {
  int blkcnt, cnt, cnt1;
  pause();
  getoff(is386);
  blkcnt = getword(YES);
  tab(t);
  printf("repeat count = ");
  putoff(is386, NO);
  printf("  block count = %4x\n", blkcnt);
  if(blkcnt)
    while(blkcnt--) getidb(t+1, is386);      /* recursive */
  else {
    cnt = getbyte(YES);
    tab(t);
    printf("count = %4x\n", cnt);
    dump(cnt, t);
    }
  }

dump(cnt, t) int cnt, t; {
  loc = i = 0;
  while(cnt--) {
    byte = getbyte(YES);
    if(byte < 128 && isprint(byte))
         ascii[i] = byte;
    else ascii[i] = '.';
    if(++i == 1) {
      tab(t);
      itox(off[0], str1, 5); zfill(str1); fputs(str1, stdout);
      fputc(' ', stdout);
      itox(   loc, str1, 5); zfill(str1); fputs(str1, stdout);
      }
    if(i == 9) fputc('-', stdout);
    else       fputc(' ', stdout);
    itox(byte, str1, 3); zfill(str1);
    fputs(str1, stdout);
    if(i == 16) xlate();
    add32(off, one);
    ++loc;
    pause();
    }
  if(i) xlate();
  }

tab(t) int t; {
  while(t--) fputc(' ', stdout);
  }

getFIXUPP(is386) int is386; {
  if(is386)
       puts("\nFIXUPP RECORD (386):  off               fndx tndx     tdis");
  else puts("\nFIXUPP RECORD:        off               fndx tndx tdis");
  while(reclen > 1) getref(is386, NO);
  }

getMODEND(is386) int is386; {
  unsigned char ch, mattr, l, f, frame, t, p, method;
  if(is386)
       puts("\nMODEND RECORD (386):                    fndx tndx     tdis");
  else puts("\nMODEND RECORD:                          fndx tndx tdis");
  ch = getbyte(YES);
  mattr = ch >> 6;
  l     = ch &  1;
       if(mattr == 0)  fputs("Non-Main, No Start Address\n", stdout);
  else if(mattr == 1)  fputs("Non-Main, Start Address\n", stdout);
  else if(mattr == 2)  fputs("Main, No Start Address\n", stdout);
  else                 fputs("Main, Start Address\n",stdout);
  if(mattr & 1) {
    if(l) {
      fputs("Logical Start Address     ", stdout);
      getref(is386, YES);
      }
    else fputs("Physical Start Address Disallowed by LINK!!!\n", stdout);
    }
  }

getref(is386, end) int is386, end; {
  unsigned char ch, mth, thd, m, s, loc, fra, tar;
  ch = getbyte(YES);
  if(end || ch & 128) {
    if(!end) {
      fputs("FIXUP: ", stdout);
      m   = (ch & 64) >> 6;
      s   = (ch & 32) >> 5;
      loc = (ch & 28) >> 2;
      if(m) fputs("M_SEG  ", stdout);
      else  fputs("M_SELF ", stdout);
           if(loc ==  0) fputs("L_LO   ", stdout);
      else if(loc ==  1) fputs("L_OFF  ", stdout);
      else if(loc ==  2) fputs("L_BASE ", stdout);
      else if(loc ==  3) fputs("L_PTR  ", stdout);
      else if(loc ==  4) fputs("L_HI   ", stdout);
      else               printf("loc=%1x  ", loc); 
      printf("%4x ", ((ch & 3) << 8) | getbyte(YES));
      ch = getbyte(YES);
      }
    fra = ch >> 4;
         if(fra ==  0) fputs("F_SI   ", stdout);
    else if(fra ==  1) fputs("F_GI   ", stdout);
    else if(fra ==  2) fputs("F_EI   ", stdout);
    else if(fra ==  4) fputs("F_LOC  ", stdout);
    else if(fra ==  5) fputs("F_TAR  ", stdout);
    else if(fra ==  8) fputs("F_TH0  ", stdout);
    else if(fra ==  9) fputs("F_TH1  ", stdout);
    else if(fra == 10) fputs("F_TH2  ", stdout);
    else if(fra == 11) fputs("F_TH3  ", stdout);
    else               printf("frm=%2x ", fra); 
    tar = ch & 15;
         if(tar ==  0) fputs("T_SID  ", stdout);
    else if(tar ==  1) fputs("T_GID  ", stdout);
    else if(tar ==  2) fputs("T_EID  ", stdout);
    else if(tar ==  4) fputs("T_SI0  ", stdout);
    else if(tar ==  5) fputs("T_GI0  ", stdout);
    else if(tar ==  6) fputs("T_EI0  ", stdout);
    else if(tar ==  8) fputs("T_TH0  ", stdout);
    else if(tar ==  9) fputs("T_TH1  ", stdout);
    else if(tar == 10) fputs("T_TH2  ", stdout);
    else if(tar == 11) fputs("T_TH3  ", stdout);
    else               printf("tar=%2x ", tar); 
    switch(fra) {
      default: fputs("    ", stdout); break;
      case 0:  case 1:  case 2:
      printf("%4x ", getindex());
      }
    switch(tar) {
      default: fputs("    ", stdout); break;
      case 0:  case 1:  case 2:
      case 4:  case 5:  case 6:
      printf("%4x ", getindex());
      }
    switch(tar) {
      case 0:  case 1:  case 2:
      getoff(is386);
      putoff(is386, NO);
      }
    puts("");
    }
  else {
    fputs("THREAD: ", stdout);
    mth = ch >> 2;
         if(mth ==  0) fputs("M_T_SI  ", stdout);
    else if(mth ==  1) fputs("M_T_GI  ", stdout);
    else if(mth ==  2) fputs("M_T_EI  ", stdout);
    else if(mth == 16) fputs("M_F_SI  ", stdout);
    else if(mth == 17) fputs("M_F_GI  ", stdout);
    else if(mth == 18) fputs("M_F_EI  ", stdout);
    else if(mth == 20) fputs("M_F_LOC ", stdout);
    else if(mth == 21) fputs("M_F_TAR ", stdout);
    else               printf("mth=%2x  ", mth); 
    thd  = ch & 3;
         if(thd ==  0) fputs("T_TH0 ", stdout);
    else if(thd ==  1) fputs("T_TH1 ", stdout);
    else if(thd ==  2) fputs("T_TH2 ", stdout);
    else               fputs("T_TH3 ", stdout);
    switch(thd) {
      case  0:  case  1:  case  2:
      case 16:  case 17:  case 18:
      printf("ndx=%4x", getindex());
      }
    puts("");
    }
  pause();
  }

getCOMENT() {
  unsigned char type, np, nl, class;
  puts("\nCOMMENT RECORD:");
  type = getbyte(YES);
  np = (type & 128) >> 7;
  nl = (type &  64) >> 6;
  class = getbyte(YES);
  printf("NoPurge = %u  NoList = %u  Class = %2xh ", np, nl, class);
       if(class ==   0) fputs("(Language Translator Comment)", stdout);
  else if(class ==   1) fputs("(Intel Copyright)", stdout);
  else if(class == 129) fputs("(Search Specified Library)", stdout);
  else if(class <= 155) fputs("(Intel Comment)", stdout);
  else if(class == 156) fputs("(DOS Level Number)", stdout);
  else                  fputs("(User Comment)", stdout);
  whatever();
  puts("");
  }

getANY(type) unsigned char type; {
  printf("\n%2x RECORD:", type);
  whatever();
  }

whatever() {
  int first;  first = YES;
  i = 0;
  while(reclen > 1) {
    byte = getbyte(YES);
    if(byte < 128 && isprint(byte))
         ascii[i] = byte;
    else ascii[i] = '.';
    if(++i == 1 && first) {first = NO; puts("");}
    if(  i == 9) fputc('-', stdout);
    else         fputc(' ', stdout);
    itox(byte, str1, 3); zfill(str1);
    fputs(str1, stdout);
    if(i == 16) xlate();
    pause();
    }
  if(i) xlate();
  }

pause() {
  if(poll(YES)) while(!poll(YES)) ;
  }
