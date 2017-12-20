/*                                
** asm2.c -- Small Assembler -- Part 2: Pass 1 and 2 Functions
**
** Copyright 1988 J. E. Hendrix
*/
#include <stdio.h>
#include "obj2.h"
#include "asm.h"
#include "mit.h"

#define MASM_xSO        /* follow MASM size override conventions? */
#define EMAX   2        /* max expr subscript */
#define LEDATA 1        /* began LEDATA */
#define LIDATA 2        /* began LIDATA */

extern 
  asa,  badsym,  debug,  defuse,  enda,  ends,  endv[],
  eom,  gotcolon,  gotnam,  ilen[],  iloc[],  inst[],  loc[],
  osa,  pass,  seed,  segndx,  srpref[],  upper,  use;

extern unsigned char
  assume[],  *ep,  *exthead,  line[],  locstr[],  *lp,
  *proptr,  *segptr,  *stptr,  *strend,  stsym[];

int
  lookups,              /* total number of MIT lookups */
  x86,                  /* target which CPU */
  x87,                  /* target which NPX */
  dsrpx,                /* default seg reg pref index */
  isegx,                /* seg index of instr operand */
  isrpx,                /* seg reg pref index of instr operand */
  elast,                /* last expression's subscript - must be signed */
  curdata,              /* current LEDATA or LIDATA record */
  curuse,               /* current use */
  zero[4],              /* 64-bit zero value */
  Srange,               /* S8 out of range? */
  CScheck,              /* check for ASSUME CS:<current segment>? */
  ASOpref,              /* generate ASO prefix? (actual prefix) */
  OSOpref,              /* generate OSO prefix? (actual prefix) */
  NOPsuff,              /* number of NOPs to pad instruction with */
  nop=0x90,             /* NOP instruction */
  seglast,              /* last assigned segment index */
  grplast,              /* last assigned group index */
  S8min[] ={  -128,-1}, /* min  8-bit self-rel displ */
  S8max[] ={   127, 0}, /* max  8-bit self-rel displ */
  S16min[]={-32768,-1}, /* min 16-bit self-rel displ */
  S16max[]={ 32767, 0}, /* max 16-bit self-rel displ */
  eval[(EMAX+1)*8],     /* expression values (80 bits + 48 unused) */
  endx[ EMAX+1],        /* expression grp/seg index */
  etyp[(EMAX+1)*4],     /* expression type */
  eflg[ EMAX+1];        /* expression flags */

unsigned char
  wait = 0x9B,          /* WAIT instruction */ 
  opnds[EMAX+2];        /* operand type buffer */ 

/*****************************************************************
                   machine instructions
*****************************************************************/

/*
** detect machine instruction and process it
*/
domach() {
  asa = osa = use;                      /* default size attributes */
  isegx = isrpx = 0;                    /* nullify instr segment */
  dsrpx = P_DS;                         /* init default segment */
  if(gotcolon)                          /* set cp to mnemonic */
       return (donext(skip(2, line)));
  else return (donext(skip(1, line)));
  }

/*
** next prefix or the instruction
*/
donext(cp) unsigned char *cp; {
  char mn[MAXNAM+1];
  unsigned dsegx, ssegx, extra, pref, ms, i, j;
  i = 0;                          /* copy mnemonic for lookup */
  while(cp[i] > ' ') {
    if(i < MAXNAM) {mn[i] = toupper(cp[i]); ++i;}
    else           {synerr(); break;}
    }
  mn[i] = 0;
  if((inst[0] = find(mn, mntbl, mncnt)) == EOF)
    return(NO);                   /* not a machine instruction */
  if(CScheck) {                   /* verify ASSUME CS:<this seg> */
    CScheck = NO;
    if(srpx2sx(P_CS) != segptr[STNDX]) csaerr();
    }
  cp = skip(2, cp);               /* skip to operands */
  mov32(iloc, loc);               /* save loc for $ */
  elast = -1;
  ep = mitbuf + mntbl[inst[0] << 1] - 2;
  extra = *ep++;                  /* set ES destination property */
  if(*ep == PREF)                 /* have a prefix */
    pref = YES;
  else {                          /* have an instruction */
    pref = NO;
    if(atend(*cp) == NO) {        /* have an operand field */
      dsegx = ssegx = 0;          /* init dest & source seg indexes */
      seed = *ep;                 /* 1st mem ref try */
      ep = cp;                    /* move to operands */

      do {                        /* evaluate expressions */
        if(elast < EMAX) ++elast;
        else { synerr(); break; }
        expr(&eval[elast<<3],
             &etyp[elast<<2],
             &eflg[elast],
             &endx[elast]);
        if(endx[elast]                     /* expr has an index */
        && (eflg[elast   ] & FEXT) == 0    /* but not an ext ref */
        && (etyp[elast<<2] & TMEM)) {      /* and mem ref */
          isegx = endx[elast];
          if(elast == 0) dsegx = isegx;    /* dest seg index */
          else           ssegx = isegx;    /* sour seg index */
          }
        } while(etyp[elast<<2] && !atend(*ep++));

      setsize(0, 1);  setsize(1, 0);          /* properly size reference */
      if(isegx                                /* segment reference and */
      && isrpx == 0                           /* no segment override */
      && (isrpx = sx2srpx(isegx, P_DS)) == 0  /* no assumed seg reg and */
      && seed == M) segerr();                 /* not CALL, JUMP, or LOOP */
      if(extra) {                             /* dest must be in ES */
        if(dsegx
        && sx2srpx(dsegx, P_ES) != P_ES) segerr();
        if(ssegx == 0
        && isrpx == P_ES) isrpx = 0;    /* no seg prefix is needed */
        }
      }
    }

  Srange = ASOpref = OSOpref = NOPsuff = 0;
  lookups++;
  if(ms = look(0)) {                  /* lookup instr */
    if(future(ms++)) hdwerr();        /* future hardware? */
    if(pref) switch(mitbuf[ms+1]) {
      case 0x66: osa = (osa == 2) ? 4 : 2; goto xSO; /* OSO */
      case 0x67: asa = (asa == 2) ? 4 : 2;           /* ASO */
            xSO: if(x86 < 3) hdwerr();
      }
    fixopnds(opnds, etyp);            /* fix ambiguous opnds[] codes */
#ifdef DEBUG
    if(debug)
      printf("opnds[] = %2x %2x %2x after fixing\n",
              opnds[0], opnds[1], opnds[2]);
#endif
    if(isrpx                          /* seg reg assumed or overriden */
    && isrpx != dsrpx                 /* but not the default seg reg */
    && seed == M)                     /* and not CALL, JUMP, or LOOP */
      generate(srpref[isrpx]);        /* gen seg reg pref */
    if(ASOpref) genabs(&ASOpref, 1);  /* gen default ASO prefix */
    if(OSOpref) genabs(&OSOpref, 1);  /* gen default OSO prefix */
    generate(ms);                     /* gen instruction */
    while(NOPsuff-- > 0)              /* gen NOPs to pad fwd self-refs */
      genabs(&nop, 1);
    }
  else if(Srange) {rngerr();  add32(loc, ilen);}
       else        inverr();
  if(pref) donext(cp);                /* recursive */
  return(YES);
  }

/*
** look for instruction in M.I.T.
** return mitbuf[] subscript, else zero
*/
look(ex) int ex; {
  int ms;
  unsigned char got;
  if(ex <= elast) {          /* fails if prefix or no operand field */
    got = opnds[ex] = etyp[ex<<2];            /* prime the pump */
    while(opnds[ex] = map(&got, &etyp[ex<<2])) {
      if(ms = look(ex+1))                     /* recursive */
        return (ms);
      }
    return (0);
    }
  opnds[ex] = 0;
#ifdef DEBUG
  if(debug)
    printf("opnds[] = %2x %2x %2x\n", opnds[0], opnds[1], opnds[2]);
#endif
  if((inst[1] = find(opnds, optbl, opcnt)) == EOF
  || (ms = findi(inst)) == EOF)
    return (0);
  ms = mitbl[ms << 1] + 4;                    /* -> hardware byte */
  return (self(ms));
  }

/*
** map operand type to next more general level
**     returns next link in "got"
**     returns operand-type (or zero) as function value
*/
map(got, et) unsigned char *got; unsigned *et; {
  int ot, sz;
  sz = *et & 0x000F;
  while(YES) {
    if(*got == 0) return (0);
    ot   = otmap [*got];
    *got = otnext[*got];
    switch(ot) {
      case P3248: 
      case M3248: if(osa == 2) {if(sz != 4) continue;}
                  else         {if(sz != 6) continue;}
                  break;
      case M3264: if(osa != sz>>1) continue;
                  break;
      case N1632:
      case M1632: if(osa != sz) continue;
                  break;
      case   I32: if(osa !=  4) continue;
      }
    return (ot);
    }
  }

/*
** set the data size of reference
** indicated by ex1 according to the size
** of the register indicated by ex2
*/
setsize(ex1, ex2) int ex1, ex2; {
  if((etyp[ex1<<2] & 0x000F) == 0) {
    switch(etyp[ex2<<2] & 0xFF) {
      case AL:   case BL:   case CL:   case DL:
      case AH:   case BH:   case CH:   case DH:
        etyp[ex1<<2] = (etyp[ex1<<2] & 0xFFF0) | 1;
        break;
      case ESP:  case EBP:  case ESI:  case EDI:
      case EAX:  case EBX:  case ECX:  case EDX:
        etyp[ex1<<2] = (etyp[ex1<<2] & 0xFFF0) | 4;
        break;
      default:
        etyp[ex1<<2] = (etyp[ex1<<2] & 0xFFF0) | 2;
      }
    }
  }

/*
** fix ambiguous codes in opnds[]
*/
fixopnds(op, et) unsigned char *op; unsigned *et; {
  int i;
  while(YES) {
    switch(*op) {
      case     0: return;
      case    _8:
      case   _16:
      case   _32:
      case _1632: if(*et & M) goto mem;
      case    AX:
      case    DX:
      case   EAX:
      case   R16:
      case   R32:
      case R1632:
      case   eAX: *op = *et;
#ifdef MASM_xSO
                  if(OSOpref == 0
                  && ((((*et & 0x00F0) == R32) + 1) << 1 != osa)) {
                    OSOpref = 0x66;
                    osa = (osa == 2) ? 4 : 2;
                    }
#endif
                  break;
             mem:
      case P3248: 
      case N1632:
      case M1632:
      case M3248:
      case M3264: *op = (*op & 0xF0) | (*et & 0x000F);
      case   _48: 
      case   _64: 
      case   _80: 
#ifdef MASM_xSO
                  if(ASOpref == 0
                  && x86 > 2
                  && use == 2
                  && (*et & TMEM)) {
                    ASOpref = 0x67;
                    asa = (asa == 2) ? 4 : 2;
                    }
#endif
                  break;
      case I1632:
      case S1632: *op = (*op & 0xF0) | (osa);
      }
    ++op;
    et += 4;
    }
  }

/*
** Is not self-relative, or is self-relative and reference is okay?
** If self-relative, convert eval[0] to displacement and
** set NOPsuff to number of NOPs to pad instruction with.
*/
self(ms) int ms; {
  unsigned disp[2], dlen, adj;
  unsigned char *cp;
  if((opnds[0] & 0xF0) != SELF)         /* not self-relative */
    return (ms);
  if(opnds[0] == S8) dlen = 1;          /* calc displ length */
  else               dlen = osa;        /* must be S1632 */
  ilen[0] = adj = 0;                    /* calc instr length */
  cp = mitbuf + ms;
  while(*++cp) {
    if(*cp == S1) ilen[0] += dlen;
    else   {                            /* must be HEX,byte pair */
      ilen[0] += 1;
      switch(*++cp) {
        case 0xE0:
        case 0xE1:
        case 0xE2:
        case 0xE3: adj = osa; break;    /* JCXZ, JECXZ, LOOPs: no NOPs */
        case 0xE9:
        case 0xEB: adj = 1;             /* unconditional jumps */
        }
      }
    }
  if(eflg[0] & FEXT)                    /* external reference */
    return (ms);
  NOPsuff = 0;
  if(etyp[0] & TFWD) {                  /* forward reference */
    if(pass == 1) return (ms);          /* pass 1: take 1st of S8 or S1632 */
    if(dlen == 1                        /* pass 2: NOPs if near -> short */
    && (etyp[0] & TSHO) == 0)
      NOPsuff = osa - adj;
    }
  if(isegx && isegx != segptr[STNDX])   /* in another segment */
    segerr();
  mov32(disp, eval);                    /* calc displ */
  sub32(disp, iloc);
  sub32(disp, ilen);
  if(dlen == 1) {                       /* 8-bit displacement */
    if(cmp32(disp, S8min ) >= 0
    && cmp32(disp, S8max ) <= 0) {
      short:
      mov32(eval, disp);
      return (ms);
      }
    if(etyp[0] & TSHO) {
      rngerr();
      goto short;
      }
    }
  else if(dlen == 2                     /* 16-bit displacement */
       && cmp32(disp, S16min) >= 0
       && cmp32(disp, S16max) <= 0)
         return (ms);
  else if(dlen == 4)                    /* 32-bit displacement */
         return (ms);
  Srange = YES;
  return (0);                           /* out of range, keep looking */
  }

/*
** instruction for future CPU or NPX?
*/
future(ms) int ms; {
  unsigned i;
  if((i = mitbuf[ms]) < 0x10) {
    if(i <= x86) return (NO);
    }
  else if(i <= x87) return (NO);
  return (YES);
  }

/*
** generate object code and bump location counter 
*/
generate(ms) unsigned ms; {
  unsigned char *ptr;
  int val;
  ptr = mitbuf + ms;             /* ms -> machine instr string */
  while(*ptr) {                  /* for each op-code directive */
    switch(*ptr++) {
      case HEX:  val = *ptr++;
                 switch(*ptr++) {
                   case PF : if(proptr && (flags(proptr) & FFAR))
                               val += 8;
                             break;
                   case PI1: val += (eval[0<<3]>>3)&7; break;
                   case PR1: val += enreg(etyp[0<<2]); break;
                   case PR2: val += enreg(etyp[1<<2]); break;
                    default: ptr--;
                   }
                 genabs(&val, 1); break;
      case M01:  genModRM(0, 0);  break;
      case M11:  genModRM(1, 0);  break;
      case M21:  genModRM(2, 0);  break;
      case M31:  genModRM(3, 0);  break;
      case M41:  genModRM(4, 0);  break;
      case M51:  genModRM(5, 0);  break;
      case M61:  genModRM(6, 0);  break;
      case M71:  genModRM(7, 0);  break;
      case MR1:  genModRM(enreg(etyp[0<<2]), 0); break;
      case MS12:
      case MR12: genModRM(enreg(etyp[0<<2]), 1); break;
      case MS21:
      case MR21: genModRM(enreg(etyp[1<<2]), 0); break;
      case MI12: genModRM(eval[0<<3] & 7, 1);    break;
      case I1:   genIMM(0); break;
      case I2:   genIMM(1); break;
      case I3:   genIMM(2); break;
      case S1:   if((opnds[0] & 0x0F) == 1) {
                   genabs(&eval[0<<3], 1);
                   break;
                   }
      case O1:   if((eflg[0] & (FDAT|FCOD|FEXT|FSEG|FGRP)) == 0) {
                   genabs(&eval[0<<3], asa);
                   break;
                   }
      case P1:   genrel(0); break;
      case O2:   if((eflg[1] & (FDAT|FCOD|FEXT|FSEG|FGRP)) == 0) {
                   genabs(&eval[1<<3], asa);
                   break;
                   }
      case P2:   genrel(1); break;
      case W6:   if(x86 < 0x01) genabs(&wait, 1); 
                 break; 
      } 
    }
  }

/*
** (mmxxxr/m) <- opnd o+1
*/
genModRM(xxx, o) int xxx, o; {
  unsigned rel, dsz, val, mm, rm, ss, iii, bbb;
  rel = (etyp[o<<2] & TMEM);              /* relative displ? */
  if(etyp[o<<2] & TIND) {                 /* indirect mem ref */
    dsz = mm = 0;                         /* assume no displacement */
    if(rel) {                             /* relative displ */
      dsz = asa;
      mm = 2;
      }
    else if(etyp[o<<2] & TCON) {          /* absolute displ */
      if(cmp32(&eval[o<<3], zero)) {      /* not zero */
        if(cmp32(&eval[o<<3], S8min) < 0
        || cmp32(&eval[o<<3], S8max) > 0) {
          dsz = asa;
          mm = 2;
          }
        else dsz = mm = 1;
        }
      }
    if(asa == 2) {                        /* 16-bit ASA */
      switch(etyp[(o << 2) + 1]) {
        case TBX+TSI: rm = 0; break;
        case TBX+TDI: rm = 1; break;
        case TBP+TSI: rm = 2; break;
        case TBP+TDI: rm = 3; break;
        case     TSI: rm = 4; break;
        case     TDI: rm = 5; break;
        case TBP    : if(mm == 0) dsz = mm = 1;
                      rm = 6; break;
        case       0: dsz = asa;  mm = 0;
                      rm = 6; break;
        case TBX    : rm = 7; break;
        default     : rm = 6; experr();
        }
      val = (mm << 6) | (xxx << 3) | rm;  /* gen ModR/M byte */
      genabs(&val, 1);
      if(rel)      genrel(o);                /* gen relative displ */
      else if(dsz) genabs(&eval[o<<3], dsz); /* gen absolute displ */
      }
    else {                                /* 32-bit ASA */
      if(etyp[(o << 2) + 3]) {            /* index register */
        rm = 4;
        ss = (etyp[(o << 2) + 3]) >> 8;
        switch((etyp[(o << 2) + 3]) & 0xFF) {
          case TAX: iii = 0; break;
          case TCX: iii = 1; break;
          case TDX: iii = 2; break;
          case TBX: iii = 3; break;
          default : experr();
          case   0: if(ss) experr();
                    iii = 4; break;
          case TBP: iii = 5; break;
          case TSI: iii = 6; break;
          case TDI: iii = 7; break;
          }
        switch(etyp[(o << 2) + 2]) {
          case TAX: bbb = 0; break;
          case TCX: bbb = 1; break;
          case TDX: bbb = 2; break;
          case TBX: bbb = 3; break;
          case TSP: bbb = 4; break;
          case TBP: if(mm == 0) dsz = mm = 1;
                    bbb = 5; break;
          case   0: dsz = asa;  mm = 0;
                    bbb = 5; break;
          case TSI: bbb = 6; break;
          case TDI: bbb = 7; break;
          default : bbb = 5; experr();
          }
        }
      else {                              /* no index register */
        switch(etyp[(o << 2) + 2]) {
          case TAX: rm = 0; break;
          case TCX: rm = 1; break;
          case TDX: rm = 2; break;
          case TBX: rm = 3; break;
          case TBP: if(mm == 0) dsz = mm = 1;
                    rm = 5; break;
          case   0: dsz = asa;  mm = 0;
                    rm = 5; break;
          case TSI: rm = 6; break;
          case TDI: rm = 7; break;
          default : rm = 5; experr();
          }
        }
      val = (mm << 6) | (xxx << 3) | rm;  /* gen ModR/M byte */
      genabs(&val, 1);
      if(rm == 4) {                       /* gen SIB byte */
        val = (ss << 6) | (iii << 3) | bbb;
        genabs(&val, 1);
        }
      if(rel)      genrel(o);                /* gen relative displ */
      else if(dsz) genabs(&eval[o<<3], dsz); /* gen absolute displ */
      }
    }
  else if(etyp[o << 2] & M) {             /* direct memory reference */
    val = (xxx << 3) | ((asa == 2) ? 6 : 5);
    genabs(&val, 1);
    if(rel) genrel(o);
    else    genabs(&eval[o<<3], asa);
    }
  else {                                  /* register operand */
    val = 0xC0 | (xxx<<3) | enreg(etyp[o<<2]);
    genabs(&val, 1);
    }
  }

/*
** generate immediate value
*/
genIMM(o) int o; {
  if(endx[o]) genrel(o);
  else        genabs(&eval[o<<3], opnds[o] & 0xF);
  }

/*
** generate an absolute value of sz bytes
*/
genabs(val, sz) int val[], sz; {
  if(pass == 2) {
    listcode(0, val, sz, " ", 0);  /* byte or words */
    setdata(LEDATA, curuse);
    putLEDATA(val,  sz);           /* gen LEDATA */
    }
  inc32(loc, sz);                  /* bump location counter */
  }

/*
** generate a relocatable item
*/
genrel(o) int o; {
  int mod, lcn, off, fra, fndx, tar, tndx, tdis, len, point, fixuse;
  char *suff;
  len = opnds[o] & 0xF0;
  if(len == SELF || len == POINT) len = opnds[o] & 0x0F;
  else if(eflg[o] & FFAR)         len = asa + 2;
  else                            len = asa;
  if(pass == 2) {
    if((opnds[o] & 0xF0) == SELF) {     /* self relative */
      mod  = F_M_SELF;
      lcn  = F_L_OFF;
      if(eflg[o] & FEXT) {              /* self relative external */
        fra = F_F_EI;
        fndx = tndx = endx[o];          /* grp/seg/ext index */
        if(cmp32(&eval[o<<3], zero)) {
          tar = F_T_EID;
          tdis = &eval[o<<3];
          }
        else {
          tar = F_T_EI0;
          tdis = 0;
          }
        }
      else {                            /* self relative local */
        fra = F_F_SI;
        fndx = tndx = segndx;
        if(cmp32(&eval[o<<3], zero))
             { tar = F_T_SID; tdis = &eval[o<<3]; }
        else { tar = F_T_SI0; tdis = 0; }
        }
      } 
    else {                              /* segment relative */
      mod = F_M_SEGMENT;
      if(eflg[o] & (FGRP|FSEG)) lcn = F_L_BASE;
      else if(eflg[o] & FFAR)   lcn = F_L_PTR;
      else                      lcn = F_L_OFF;
      if(eflg[o] & FGRP)        fra = F_F_GI;
      else if(eflg[o] & FEXT)   fra = F_F_EI;
      else                      fra = F_F_SI; 
      fndx = tndx = endx[o];            /* grp/seg/ext index */
      if(cmp32(&eval[o<<3], zero)) {
        if(eflg[o] & FGRP)      tar = F_T_GID;
        else if(eflg[o] & FEXT) tar = F_T_EID;
        else                    tar = F_T_SID; 
        tdis = &eval[o<<3];
        }
      else {
        if(eflg[o] & FGRP)      tar = F_T_GI0;
        else if(eflg[o] & FEXT) tar = F_T_EI0;
        else                    tar = F_T_SI0; 
        tdis = 0;
        }
      }
    fixuse = (use == 2) ? 2 : asa;
    setdata(LEDATA, fixuse);
    putLEDFIX(zero, len, fixuse, mod, lcn, fra, fndx, tar, tndx, tdis);
    if((eflg[o] & (FGRP|FSEG)) || len > asa)
         point = 1;
    else point = 0;
    if(eflg[o] & FEXT) suff = "e ";
    else               suff = "r ";
    listcode(0, &eval[o<<3], len, suff, point);
    }
  inc32(loc, len);                    /* bump location counter */
  }

/*
** encode register for machine instruction
*/
enreg(r) unsigned char r; {         /* strip high-order bits */
  switch(r) {
    case AL:  case AX:  case EAX:  case  ES:  case ST0:  case DR0:  case CR0: 
      return (0); 
    case CL:  case CX:  case ECX:  case  CS:  case ST1:  case DR1: 
      return (1); 
    case DL:  case DX:  case EDX:  case  SS:  case ST2:  case DR2:  case CR2: 
      return (2); 
    case BL:  case BX:  case EBX:  case  DS:  case ST3:  case DR3:  case CR3: 
      return (3); 
    case AH:  case SP:  case ESP:  case  FS:  case ST4: 
      return (4); 
    case CH:  case BP:  case EBP:  case  GS:  case ST5: 
      return (5); 
    case DH:  case SI:  case ESI:  case ST6:  case DR6:  case TR6: 
      return (6); 
    case BH:  case DI:  case EDI:  case ST7:  case DR7:  case TR7: 
      return (7); 
    }
  error2("+ Bad Register Code for: ", line);
  }

/*****************************************************************
                   assembler instructions
*****************************************************************/

/*
** detect assembler instruction and process it
*/
doasm() {
  if(atend(*lp) && (!stsym[0] || gotcolon)) return;
  if(gotcolon) synerr();
  if(!doasm2()) {             /* lp -> 2nd field or end */
    lp = skip(1, line);       /* lp -> 1st field */
    stsym[0] = NULL;          /* declare no symbol */
    if(!doasm2()) inverr();
    }
  }

/*
** detect and perform an assembler instruction
*/
doasm2() {
  char *cp;
  lp = skip(2, cp = lp);       /* skip over mnemonic */
  if(same(cp, "DB"     )) {         dodata( 1);  return (YES);}
  if(same(cp, "DW"     )) {         dodata( 2);  return (YES);}
  if(same(cp, "DD"     )) {         dodata( 4);  return (YES);}
  if(same(cp, "DF"     )) {         dodata( 6);  return (YES);}
  if(same(cp, "DQ"     )) {         dodata( 8);  return (YES);}
  if(same(cp, "DT"     )) {         dodata(10);  return (YES);}
  if(same(cp, "EXTRN"  )) {noloc(); doextrn();   return (YES);}
  if(same(cp, "="      )) {noloc(); doset();     return (YES);}
  if(same(cp, "EQU"    )) {noloc(); doequ();     return (YES);}
  if(same(cp, "GROUP"  )) {noloc(); dogroup();   return (YES);}
  if(same(cp, "ORG"    )) {         doorg();     return (YES);}
  if(same(cp, "PUBLIC" )) {noloc(); dopublic();  return (YES);}
  if(same(cp, "SEGMENT")) {noloc(); dosegment(); return (YES);}
  if(same(cp, "ENDS"   )) {         doends();    return (YES);}
  if(same(cp, "ASSUME" )) {noloc(); doassume();  return (YES);}
  if(same(cp, "PROC"   )) {         doproc();    return (YES);}
  if(same(cp, "ENDP"   )) {         doendp();    return (YES);}
  if(same(cp, "END"    )) {noloc(); doend();     return (YES);}
  if(same(cp, ".CASE"  )) {noloc(); upper = NO;  first(); return (YES);}
  if(same(cp, ".186"   )) {noloc(); x86 = 0x01;  first(); return (YES);}
  if(same(cp, ".286"   )) {noloc(); x86 = 0x02;  first(); return (YES);}
  if(same(cp, ".287"   )) {noloc(); x87 = 0x20;  first(); return (YES);}
  if(same(cp, ".386"   )) {noloc(); x86 = 0x03;  first();
                                    defuse = 4;           return (YES);}
  if(same(cp, ".387"   )) {noloc(); x87 = 0x30;  first(); return (YES);}
  return(NO);
  }

/*
** gripe if not before first segment
*/
first() {
  if(use) segerr();
  }

/*
** name GROUP segmentname,,,
*/
dogroup() {
  int n;  n = 0;
  if(gotnam && !badsym) {
    if(!stfind()) {             /* not in table, pass 1 */
      addsym();                 /* put in table */
      setflags(stptr, FGRP);    /* set group flag */
      stptr[STNDX] = ++grplast; /* group index */
      }
    grpseg(stptr, &n);          /* process segment list */
    }
  else symerr();                /* symbol error */ 
  }

/*
** identify next segment in a group
*/
grpseg(gp, n) unsigned char *gp; int *n; {
  unsigned char ndx, *cp;
  if(!atend(*lp)) {
    lp = getsym(lp, upper, NO);
    if(gotnam && !badsym) {
      if(!stfind()) {             /* add new segment? */
        addsym();
        newseg();
        }
      if(flags(stptr) & FSEG) {   /* have a segment name? */
        ++*n;                     /* bump count */
        ndx = stptr[STNDX];       /* save seg ndx locally */
        lp = skip(1, lp);         /* skip white space */
        cp = grpseg(gp, n);       /* recursive */

        if(pass == 2) return;
        cp[(*n)--] = ndx;         /* plug seg ndx into list */
        return (cp);
        }
      else segerr();
      }
    else symerr();
    }

  end:
  if(pass == 1) {
    cp = malloc(*n+1);     /* allocate space for group list */
    setval1(gp, cp);       /* put list pointer in group st entry */
    cp[(*n)--] = 0;        /* null terminal entry */
    }
  return (cp);
  }

/*
** name SEGMENT [sizes]  [align]  [combine]  [class]
**               USE16    BYTE     PUBLIC     'name'
**               USE32    WORD     STACK
**                        PARA     COMMON
**                        PAGE
*/
dosegment() {
  lp = getsym(skip(1, line), upper, NO); /* fetch segment name */
  CScheck = YES;
  use = defuse;                        /* assume default use */
  if(badsym) symerr();                 /* symbol error */
  else {
    if(stfind()) {                     /* already in table? */
      if(flags(stptr) == FCLS)         /* only a class name */
        newseg();                      /* so, make it a segment too */
      if(!(flags(stptr) & FSEG)) {     /* redefinition error */
        rederr();
        return;
        }
      }
    else {                             /* add new segment to table */
      addsym();
      newseg();
      }
    if(segptr) {                       /* segment nesting? */
      segerr();                        /* not allowed */
      setval2(segptr, loc);            /* assume ENDS */
      setdata(0, curuse);              /* end LEDATA or LIDATA */
      }
    val2(loc, segptr = stptr);         /* new segptr, loc, segndx */
    segndx = stptr[STNDX];             /* from old or new seg */
    lp = skip(3, line);                /* locate 1st operand field */
    while(!atend(*lp)) {
      if(*lp == '\'') {                /* class name? */
        lp = getsym(++lp, upper, NO);
        if(badsym)  symerr();
        else if(*lp == '\'') {
          ++lp;
          if(!stfind()) {
            addsym();
            orflags(stptr, FCLS);      /* designate as class name */
            }
          if(flags(stptr) & FCLS)
            setval1(segptr, stptr);    /* point seg to class name */
          else rederr();
          }
        else synerr();
        continue;
        }
      lp = getsym(lp, YES, NO);
      if(badsym) symerr();
      else if(same(stsym, "BYTE"))    {setseg(segptr, A_BYTE,   7, 3);}
      else if(same(stsym, "WORD"))    {setseg(segptr, A_WORD,   7, 3);}
      else if(same(stsym, "PARA"))    {setseg(segptr, A_PARA,   7, 3);}
      else if(same(stsym, "PAGE"))    {setseg(segptr, A_PAGE,   7, 3);}
      else if(same(stsym, "PUBLIC"))  {setseg(segptr, C_PUBLIC, 7, 0);}
      else if(same(stsym, "STACK"))   {setseg(segptr, C_STACK,  7, 0);}
      else if(same(stsym, "COMMON"))  {setseg(segptr, C_COMMON, 7, 0);}
      else if(x86 > 2) {
             if(same(stsym, "USE32")) {setseg(segptr, 1, 1, 6); use=4;}
        else if(same(stsym, "USE16")) {setseg(segptr, 0, 1, 6); use=2;}
        else symerr();
        }
      else symerr();
      }
    }
  curuse = use;
  }

/*
** establish this symbol table entry as a new segment
*/
newseg() {
  orflags(stptr, FSEG);
  if(seglast < MAXSEG) ++seglast;
  else error("+ Too Many Segments");
  stptr[STNDX] = seglast;          /* segment index */
  setseg(stptr, C_NOT,  7, 0);     /* assume no combining */
  setseg(stptr, A_PARA, 7, 3);     /* assume para alignment */
  setseg(stptr, (defuse == 2) ? 0 : 1, 1, 6);
  }

/*
** splice a segment attribute into its symbol table entry
*/
setseg(ptr, value, mask, shift) char *ptr; int value, mask, shift; {
  ptr[STSIZE] = (ptr[STSIZE] & ~(mask << shift)) | (value << shift);
  }

/*
** ENDS
*/
doends() {
  getsym(skip(1, line), upper, NO);
  if(!stfind()
  || !segptr
  || !same(symbol(segptr), symbol(stptr))) segerr();
  setval2(segptr, loc);  /* save old seg loc ctr */
  segptr = 0;            /* no longer in a segment */
  setdata(0, curuse);    /* end LEDATA or LIDATA */
  }

/*
** name PROC [distance]
**            NEAR
**            FAR
*/
doproc() {
  if(pass == 1) {                       /* pass 1 */
    if(newsym(YES, NO, FPRO|FCOD)) {
      if(same(lp, "NEAR"))     orflags(stptr, FCOD);
      else if(same(lp, "FAR")) orflags(stptr, FCOD | FFAR);
      else if(!atend(*lp))     synerr();
      setval2(stptr, loc);              /* value */
      stptr[STNDX] = segndx;            /* segment index */
      }
    }
  else {                                /* pass 2 */
    if(stfind()
    && cmp32(stptr + STVAL2, loc)) phserr();
    }
  if(proptr) proerr();                  /* no nesting */
  proptr = stptr;
  }

/*
** ENDP
*/
doendp() {
  if(proptr == 0) proerr();
  proptr = 0;
  }

/*
** ASSUME register:name,,,
**        CS       segmentname
**        SS       groupname
**        DS       NOTHING
**        ES
**        FS
**        GS
**        NOTHING
*/
doassume() {
  int *sp, reg, i;
  while(!atend(*lp)) {
    lp = getsym(lp, YES, NO);       /* register, skips the colon */
    if(same(stsym, "NOTHING")) {
      printf("\nMatch Assume Nothing.");
      nosegs();                     /* clear all assumed seg regs */
      if(!atend(*lp)) synerr();
      return;
    }
      printf("\nMatch Assume, not nothing...");
    ep = stsym;                     /* set-up for register() */
    if(gotcolon && (reg = register()) && (reg = sr2srpx(reg))) {
      /* map to srpx */
      if(srpref[reg]) {             /* sr is known to this CPU */
        i = 7;                      /* clear this assumed seg reg */
        while(++i < ((MAXSEG+1)<<3)) {
          if(assume[i] == reg) {
            while(assume[i] = assume[i+1]) ++i;  /* close the gap */
            break;                  /* will hit null for this seg */
          }
        }
        lp = getsym(lp, upper, NO); /* segname, skips the comma */
        if(same(stsym, "NOTHING")) {
          continue;
        }
        else if(stfind()) {
          if(flags(stptr) & FSEG) { /* have a segment name */
            i = stptr[STNDX] << 3;  /* look for blank slot */
            while(assume[i]) {
              ++i;   /* cannot overflow 6 slots */
            }
            assume[i] = reg;        /* plug srpx into slot */
          }
          else segerr();
        }
        else underr();
      }
      else hdwerr();
    }
    else {
      synerr();
    }
  }
}

/*
** PUBLIC name,,,
*/
dopublic() {
  while(!atend(*lp)) {
    lp = getsym(lp, upper, NO); /* next name, skips comma */
    if(badsym) {                /* symbol error */
      symerr();
      continue;
      }
    if(pass == 1) {
      if(!stfind()) addsym();
      orflags(stptr, FPUB);     /* Public flag */
      }
    }
  }

/*
** EXTRN name:type,,,
**            BYTE
**            WORD
**            DWORD
**            FWORD
**            QWORD
**            TBYTE
**            NEAR
**            FAR
*/
doextrn() {
  while(!atend(*lp)) {
    lp = getsym(lp, upper, NO);   /* next name, bypasses colon */
    if(!gotcolon) {synerr(); return;}
    if(newsym(YES, YES, FEXT)) {
      setval1(stptr, exthead);    /* build backward chain */
      exthead = stptr;
      lp = getsym(lp, YES, NO);   /* type, bypasses comma */
           if(same(stsym, "BYTE" )) {orflags(stptr, FDAT); stptr[STSIZE] =  1;}
      else if(same(stsym, "WORD" )) {orflags(stptr, FDAT); stptr[STSIZE] =  2;}
      else if(same(stsym, "DWORD")) {orflags(stptr, FDAT); stptr[STSIZE] =  4;}
      else if(same(stsym, "FWORD")) {orflags(stptr, FDAT); stptr[STSIZE] =  6;}
      else if(same(stsym, "QWORD")) {orflags(stptr, FDAT); stptr[STSIZE] =  8;}
      else if(same(stsym, "TBYTE")) {orflags(stptr, FDAT); stptr[STSIZE] = 10;}
      else if(same(stsym, "NEAR" )) {orflags(stptr, FCOD);}
      else if(same(stsym, "FAR"  )) {orflags(stptr, FCOD|FFAR);}
      else synerr();
      }
    else lp = getsym(lp, YES, NO);     /* skip type */
    }
  }

/*
** [name] DB value,,,
** [name] DW value,,,
** [name] DD value,,,
** [name] DF value,,,
** [name] DQ value,,,
** [name] DT value,,,
*/
dodata(sz) int sz; {
  if(segptr == 0) segerr();        /* not in a segment */
  if(pass == 1) {
    newsym(NO, NO, FDAT);          /* table optional data name */
    if(stptr) {                    /* new or old symbol */
      orflags(stptr, FDAT);
      setval2(stptr, loc);
      stptr[STSIZE] = sz;
      stptr[STNDX] = segndx;
      }
    }
  while(!atend(*lp)) {
    dodata2(sz);
    while(*lp != ',' && isgraph(*lp)) {++lp; experr();}  /* trailing junk */
    while(*lp == ',' || isspace(*lp)) {++lp;}
    }
  } 

/*
** define one data item
*/
dodata2(sz) int sz; {
  int dlm, cnt, rep;
  char *cp;
  cp = lp;
  while(isgraph(*cp) && *cp != ',') ++cp;
  while(isspace(*cp) && !atend(*cp)) ++cp;
  if(same(cp, "DUP")) {
    cp = skip(1, cp += 3);
    if(*cp != '(') goto nodup;
    doexpr(lp, NO, YES);                /* evaluate count (constant) */
    rep = eval[0<<3];
    lp = dodata3(cp, sz, YES);          /* evaluate data (constant) */
    if(pass == 2) {
      listcode(rep, eval, sz, " ", 0);  /* byte or words */
      setdata(LIDATA, curuse);
      putLIDATA(rep, eval, sz);
      }
    inc32(loc, rep * sz);
    return (rep * sz);
    }

  nodup:
  pad(eval, 0, 10);
  if((*lp == '\"') || (*lp == '\'')) {  /* string */
    cnt = 0;
    dlm = *lp;
    while(!atend(*++lp)) {
      if((*lp == dlm) && (*++lp != dlm)) break;
      eval[0<<3] = *lp;
      genabs(eval, sz);
      ++cnt;
      }
    return (cnt * sz);
    }
  lp = dodata3(lp, sz, NO);             /* evaluate data */
  opnds[0] = etyp[0<<2];                /* make genrel() see etyp[0] */
  if(endx[0] == 0)   genabs(eval, sz);  /* generate absolute value */
  else if(sz == asa) genrel(0);         /* generate relative value */
  else experr();
  return (sz);
  }

/*
** parse packed const, real const, or expression (const or var)
*/
dodata3(cp, sz, con) char *cp; int sz, con; {
  int bfp;
  bfp = maybe_bfp(cp);            /* 0=>binary, 1=>float, 2=>packed */
  if(bfp == 1 && (sz == 4 || sz == 8 || sz == 10)) {
    endx[0] = 0;
    cp += atoft(eval, cp);        /* encode as floating point number */
    if(sz < 10) {
      mov80(eval+8, eval);
      if(sz < 8) ft2s(eval, eval+8);
      else       ft2d(eval, eval+8);
      }
    }
  else if(bfp == 2 && sz == 10)   /* encode as packed decimal number */
    cp += atop(eval, cp);         
  else
    cp = doexpr(cp, NO, con);     /* encode as binary integer */
  return (cp);
  }

/*
** current token could be:
** (0) binary integer (suffix b, o, q, d, h), or
** (1) floating point (decimal point), or
** (2) packed decimal (default)?
*/
maybe_bfp(cp) char *cp; {
  if(*cp == '+' || *cp == '-') ++cp;
  while(isdigit(*cp)) ++cp;
  switch(tolower(*cp)) {
    case 'b': case 'o':
    case 'q': case 'd': case '?':
    case 'h': return (0);
    case '.': return (1);
    }
  return (2);
  }

/*
** name  EQU constantexpression
** alias EQU symbol
**
** not redefinable
*/
doequ() {
  char *ptr;
  ptr = donamexp(FEQU);                 /* do name & expr */
  if((etyp[0<<2] & TCON)
  && !(eflg[0] & (FSEG|FGRP))) {        /* constant? */
    if(pass == 1)
      setval2(ptr, &eval[0<<3]);        /* value */
    else                                /* list value */
      listcode(0, &eval[0<<3], 2, " =", 0);
    }
  else {                                /* alias? */
    lp = getsym(lp, upper, YES);
    if(atend(*lp)                       /* nothing else */
    && stfind()                         /* defined symbol */
    && (flags(stptr) & FEQU) == 0)      /* but not another EQU */
      setval1(ptr, stptr);              /* point to symbol */
    else experr();
    }
  }

/*
** name  =  constantexpression
**
** redefinable
*/
doset() {
  char *ptr;
  ptr = donamexp(FSET);                 /* do name & expr */
  if((etyp[0<<2] & TCON) == 0
  || (eflg[0] & (FSEG|FGRP))) experr();
  setval2(ptr, &eval[0<<3]);            /* value */
  if(pass == 2)                         /* list value */
    listcode(0, &eval[0<<3], 2, " =", 0);
  }

/*
** process name and expression for EQU and = directives
*/
donamexp(fbit) int fbit; {
  char *ptr;
  if(!stsym[0] || badsym || gotcolon) { /* proper name? */
    symerr();
    return;
    }
  if(stfind()) {                        /* already defined? */
    if((flags(stptr) & fbit) == 0) {
      rederr();
      return;
      }
    }
  else {                                /* not yet defined  */
    addsym();
    setflags(stptr, fbit);
    }
  ptr = stptr;                          /* preserve stptr */
  doexpr(lp, YES, NO);                  /* evaluate expression */
  return (ptr);
  }

/*
**  END [start]
*/
doend() {
  eom = YES;                         /* flag end of module */
  if(segptr) segerr();
  doexpr(lp, YES, NO);
  if(etyp[0<<2]) {                   /* expr not null */
    if(etyp[0<<2] & TMEM) {
      enda = M_A_MA;
      ends = endx[0];
      mov32(endv, eval);
      }
    else naderr();
    }
  }

/*
** ORG expression
*/
doorg() {
  int rec;
  doexpr(lp, YES, NO);
  if(etyp[0<<2] & TCON) {
    rec = curdata;         /* remember open data rec type */
    setdata(0, curuse);    /* close data record */
    mov32(loc, eval);      /* modify location counter */
    setdata(rec, use);     /* reopen data record */
    ltox(loc, locstr, 9);  /* will list new value */
    }
  else experr();           /* not absolute */
  }

/*
** evaluate an expression
*/
doexpr(cp, one, con) char *cp;  int one, con; {
  ep = cp;
  seed = M;                      /* avoid residual S8 or S1632 */
  expr(eval, etyp, eflg, endx);
  if((one && atend(*ep) == 0)
  || (con && (etyp[0<<2] & TCON) == 0)) experr();
  return (ep);
  }

/*
** set up for LEDATA, LIDATA, or no object data
*/
setdata(newdata, newuse) int newdata, newuse; {
  if(curdata != newdata || curuse != newuse) {
    if(curdata) endDATA();
    switch(newdata) {
      case LEDATA: begLEDATA(defuse, newuse, segndx, loc); break;
      case LIDATA: begLIDATA(defuse, newuse, segndx, loc);
      }
    curdata = newdata;
    curuse  = newuse;
    }
  }

