/*
** asm3.c -- Small Assembler -- Part 3: Expression Analyzer
**
** Copyright 1988 J. E. Hendrix
**
*/
#include <stdio.h>
#include "asm.h"

#define MASM_OPS        /* accept masm operators */

extern unsigned
  asa,  badsym,  debug,  dsrpx,  gotcolon,  gotcomma,  gotnam,
  isrpx,  osa,  seed,  segndx,  upper,  x86,  zero[];

extern unsigned char
  *ep,  *stptr,  stsym[];

unsigned
  number[5],            /* value of numeric token */
  iloc[5],              /* instruction location, padded with 3 zeroes */
  ilen[2],              /* instruction length */
  regcode,              /* register code */
  segover,              /* seg override prefix in expr? */
  brackets,             /* inside brackets */
  ct;                   /* current token */

unsigned
  I6min[] ={0x0000, 0x0000}, /* min 6-bit value */
  I6max[] ={0x003F, 0x0000}, /* max 6-bit value */
  I8min[] ={0xFF80, 0xFFFF}, /* min 8-bit value */
  I8max[] ={0x00FF, 0x0000}, /* max 8-bit value */
  I16min[]={0x8000, 0xFFFF}, /* min 16-bit value */
  I16max[]={0xFFFF, 0x0000}; /* max 16-bit value */

int                     /* operators by precedence level */
  l1ops[] = {OR, NULL},
  l2ops[] = {XOR, NULL},
  l3ops[] = {AND, NULL},
  l4ops[] = {EQ, NE, NULL},
  l5ops[] = {LE, GE, LT, GT, NULL},
  l6ops[] = {LSH, RSH, NULL},
  l7ops[] = {PLUS, MINUS, PRD, LBR, NULL},
  l8ops[] = {MULT, DIV, MOD, NULL};

/*
** evaluate the next expression at ep
** caller must set ep
*/
expr(v, t, f, x) unsigned *v, *t, *f, *x; {
  ct =                      /* no current token */
  brackets =                /* not inside brackets */
  segover = 0;              /* reset seg override flag */
  pad(v, *f=0, 10);         /* default to null expression */
  pad(t, *x=0,  8);
  if(token(EOE)) return;
  if(!level1(v, t, f, x) || ct != EOE)  experr();
  if( segover && !(t[0] & M))  segerr();
  if(!segover && (t[0] & (TMEM|TCON|TIND)) == TCON)
    t[0] = (t[0] & 0xFF00) | ctype(v);
#ifdef DEBUG
  if(debug) {
    printf("%4x%4x%4x%4x%4x=value\n", v[4], v[3], v[2], v[1], v[0]);
    printf("    %4x%4x%4x%4x=type  ", t[3], t[2], t[1], t[0]);
    printf("%4x=flags  %4x=index  %4x=isrpx\n", f[0], x[0], isrpx);
    }
#endif
  }

level1(v,t,f,x) int *v,*t,*f,*x; {return (down(l1ops,level2,v,t,f,x));}
level2(v,t,f,x) int *v,*t,*f,*x; {return (down(l2ops,level3,v,t,f,x));}
level3(v,t,f,x) int *v,*t,*f,*x; {return (down(l3ops,level4,v,t,f,x));}
level4(v,t,f,x) int *v,*t,*f,*x; {return (down(l4ops,level5,v,t,f,x));}
level5(v,t,f,x) int *v,*t,*f,*x; {return (down(l5ops,level6,v,t,f,x));}
level6(v,t,f,x) int *v,*t,*f,*x; {return (down(l6ops,level7,v,t,f,x));}
level7(v,t,f,x) int *v,*t,*f,*x; {return (down(l7ops,level8,v,t,f,x));}
level8(v,t,f,x) int *v,*t,*f,*x; {return (down(l8ops, unary,v,t,f,x));}

unary(v, t, f, x) unsigned *v, *t, *f, *x;  {
  int ok, ty, temp[2];
  ep = skip(1, ep);                    /* skip blanks */
  if(token(OFF)) {                     /* OFFSET */
    ok = unary(v, t, f, x);
    if(t[0] & TMEM) {
      t[0] = Ix | asa | TCON;
      *f &= ~(FSEG | FGRP);
      return (ok);
      }
    return (NO);
    }
  if(token(SEG)) {                     /* SEG */
    ok = unary(v, t, f, x);
    if(t[0] & TMEM) {
      pad(v, 0, 10);
      t[0] = Ix | asa | TCON;
      *f |= FSEG;
      return (ok);
      }
    return (NO);
    }
  if(token(SHORT)) {                   /* SHORT */
    ok = unary(v, t, f, x);
    t[0] = (t[0] & TFWD) | TSHO | TMEM | S8;
    return (ok);
    }
  if(ty = isptr()) {                   /* PTR */
    if(ty == 12) *f = FFAR;            /* tell primary() */
    ok = unary(v, t, f, x);
    switch(ty) {
      case  1:  case  2:  case  4:  case  6:  case  8:  case 10:
               t[0] = (t[0] & 0xFFF0) | ty;     break;
      case 11: *f &= ~FFAR; t[0] = TMEM | seed; break;
      case 12: *f |=  FFAR; t[0] = TMEM | ((osa==2) ? P32 : P48);
      }
    return (ok);
    }
  if(token(LBR)) {                     /* [ */
    brackets = YES;
    ok = level7(v, t, f, x);
    if(token(RBR)) {
      brackets = NO;
      return (ok);        /* require and skip right bracket */
      }
    return (NO);
    }
  if(token(CPL)) {                     /* ~ */
    if(!unary(v, t, f, x)) return (NO);
    cpl32(v);
    goto check;
    }
  if(token(NOT)) {                     /* ! */
    if(!unary(v ,t, f, x)) return (NO);
    lneg32(v);
    goto check;
    }
  if(token(MINUS)) {                   /* - */
    if(!unary(v, t, f, x)) return (NO);
    neg32(v);

    check:
    if(t[0] & TMEM) {
      relerr();                        /* can't be relocatable */
      t[0] &= ~TMEM;                   /* force ABS */
      }
    return (YES);                      /* lie about it */
    }

  ok = primary(v, t, f, x);            /* parse primary */

  if(token(COL)) {                     /* segment override prefix */
    if(*f & FSEG) {                    /* segname:expr */
      segover = YES;
      if(isrpx = sx2srpx(*x, P_DS)) *x = 0;
      else segerr();
      }
    else if(isrpx = sr2srpx(t[0]))     /* xS:expr */
      segover = YES;
    else segerr();
    t[0] &= ~TSHO;                     /* kill prefix influence */
    ok = unary(v, t, f, x);
    }  
  return (ok);
  }

primary(v, t, f, x) unsigned *v, *t, *f, *x; {
  int ok, asz;
  if(token(LPN)) {             /***** ( *****/
    ok = level1(v, t, f, x);
    if(token(RPN))  return(ok);
    return (NO);
    }
  if(token(NUM)) {             /***** number *****/
    mov80(v, number);
    if(segover || brackets)           /* set for possible mem match */
         t[0] = TCON | M;
    else t[0] = TCON | Ix;            /* set for immed opnd match */
    return (YES);
    }
  if(token(LOC)) {             /***** $ *****/
    mov80(v, iloc);
    if(*x == 0) *x = segndx;          /* not overriden */
    t[0] = TMEM | seed;
    return (YES);
    }
  if(token(REG)) {             /***** register *****/
    if(brackets) {                    /* register-indirect reference */
      t[0] = _x;                      /* set for indir mem match */
      if(scaled(t)) switch(regcode) {
        case ESI: t[3] |= TSI;  goto asa32;
        case EDI: t[3] |= TDI;  goto asa32;
        case EAX: t[3] |= TAX;  goto asa32;
        case EBX: t[3] |= TBX;  goto asa32;
        case ECX: t[3] |= TCX;  goto asa32;
        case EDX: t[3] |= TDX;  goto asa32;
        case EBP: t[3] |= TBP;
                  dsrpx = P_SS; goto asa32;
         default: return (NO);
        }
      else switch(regcode) {
        case  SI: t[1] |= TSI; goto asa16;
        case  DI: t[1] |= TDI; goto asa16;
        case  BX: t[1] |= TBX; goto asa16;
        case  BP: t[1] |= TBP;
                  dsrpx = P_SS;
           asa16: asz = 0x0200;
                  break;
        case ESI: t[2] |= TSI; goto asa32;
        case EDI: t[2] |= TDI; goto asa32;
        case EAX: t[2] |= TAX; goto asa32;
        case EBX: t[2] |= TBX; goto asa32;
        case ECX: t[2] |= TCX; goto asa32;
        case EDX: t[2] |= TDX; goto asa32;
        case ESP: t[2] |= TSP;
                  dsrpx = P_SS;    goto asa32;
        case EBP: t[2] |= TBP;
                  dsrpx = P_SS;
           asa32: asz = 0x0400;
                  break;
         default: return (NO);
        }
      t[0] |= asz;
      }
    else t[0] = TREG | regcode;       /* simple register reference */
    return (YES);
    }
  if(token(SYM)) {             /***** symbol *****/
    if(stfind() && flags(stptr) != FPUB) {  /* only pass 2 for fwd ref */
      if((flags(stptr) & FEQU)
      && val1(stptr))                 /* alias? */
        stptr = val1(stptr);          /* map to target */
      val2(v, stptr);                 /* expr value */
      if(*x == 0) *x = stptr[STNDX];  /* expr index (seg/grp/ext) */
      if((*f & FFAR) == 0             /* fwd ref needs FAR PTR? */
      && (flags(stptr) & FFAR)
      && forward(v, x)) farerr();
      *f = flags(stptr);              /* expr flags */
                                      /* expr type to start MIT search */
      if(*f & (FSET | FEQU))      t[0]  = TCON | Ix;
      else if(*f & (FSEG | FGRP)) t[0]  = TCON | I16;
      else if(brackets)           t[0]  = TMEM | _x;
      else if(*f & FCOD) {
        if(*f & FFAR)             t[0]  = TMEM | ((osa==2) ? P32 : P48);
        else if(seed == M)        t[0]  = TMEM | M;
        else if(*f & FEXT)        t[0]  = TMEM | S1632;
        else                      t[0]  = TMEM | S8; /* pass 2: short? */
        if(forward(v, x))         t[0] |= TFWD;      /* like pass 1 */
        }
      else if(*f & FDAT) {
        if(seed == M
        || stptr[STSIZE] != asa)  t[0]  = TMEM | M | stptr[STSIZE];
        else                      t[0]  = TMEM | ((asa==2) ? _16 : _32);
        }
      else experr();
      }
    else {                               /* assume forward ref */
      if(seed == M)
           t[0] = TFWD | TMEM | M;
      else t[0] = TFWD | TMEM | S1632;   /* pass 1: assume not short */
      underr();                          /* error on 2nd pass only */
      }
    return (YES);
    }
  return (NO);
  }

/*
** forward reference?
*/
forward(v, x) unsigned *v, *x; {
  return ((*x > segndx) || (cmp32(v, iloc) > 0));
  }

/*
** drop to a lower level
*/
down(ops, level, v, t, f, x) int *ops, (*level)(), *v, *t, *f, *x; {
  int *op;
  if(!(*level)(v, t, f, x))  return (NO);               /* left */
  op = --ops;
  while(*++op) {
    if(token(*op)) {
      if(!down2(*op, level, v, t, f, x))  return (NO);  /* right */
      if(token(EOE))  return (YES);
      op = ops;
      }
    }
  return (YES);
  }

/*
** binary drop to a lower level
*/
down2(oper, level, v, t, f, x) int oper, (*level)(), *v, *t, *f, *x; {
  int ok, vr[5], tr[4], fr, xr;
  pad(vr, fr=0, 10);                    /* init subexpression */
  pad(tr, xr=0,  8);
  if(oper == LBR) brackets = YES;
  ok = (*level)(vr, tr, &fr, &xr);      /* call next level down */
  binary(v, oper, vr);                  /* apply operator */
  *f |= fr;                             /* merge flag bits */
  if(*x == 0) *x = xr;                  /* set seg index */
  else if(xr && *x != xr) segerr();
  if(oper == LBR) {
    if(token(RBR)) brackets = NO;       /* bypass right bracket */
    else           ok = NO;
    }
  if(tr[0] & TIND)                      /* indirect if right side is */
    t[0] = (t[0] & 0x0F800) | tr[0];
  if(t[0] & TIND) {                     /* indirect mem ref */
    t[0] |= tr[0] & 0x0F800;            /* merge high bits only */
    t[1] |= tr[1];                      /* merge  8086 indir reg bits */ 
    if(t[2])                            /* merge 80386 indir reg bits */ 
         if(tr[2]) t[3] |= tr[2];       /* 2nd reg must be index */
         else      t[3] |= tr[3];       /* scaled index */
    else if(tr[2]) t[2] |= tr[2];       /* 1st reg (unscaled) not index */
         else      t[3] |= tr[3];       /* scaled index */
    switch(oper) {
      case  PLUS:
      case MINUS:
      case   LBR:
      case   PRD: return (ok);
      }
    return (NO);
    }
  if(oper == PRD) return (NO);
  if(t[0] & TCON) {
    if(tr[0] & TCON) return (ok);       /* abs <oper> abs */
    else if(tr[0] & TMEM) {             /* abs <oper> rel */
      t[0] = tr[0];
      if(oper == PLUS) return (ok);
      return (NO);
      }
    else return (NO);                   /* abs <oper> reg */
    }
  else if(t[0] & TMEM) {
    if(tr[0] & TCON) {                  /* rel <oper> abs */
      if(oper == PLUS
      || oper == MINUS) return (ok);
      return (NO);
      }
    else if(tr[0] & TMEM) {             /* rel <oper> rel */
      if(*f & FEXT) return (NO);
      switch(oper) {
        case EQ:  case LT:  case LE:
        case NE:  case GT:  case GE:
        case MINUS:
        return (ok);
        }
      return (NO);
      }
    else return (NO);                   /* rel <oper> reg */
    }
  return (NO);                          /* reg <oper> ? */
  }

/*
** apply a binary operator
*/
binary(left, oper, right) int left[], oper, right[]; {
  switch(oper) {
    case LBR:
    case PRD:
    case PLUS:  add32(left, right); break;
    case MINUS: sub32(left, right); break;
    case MULT:  mul32(left, right); break;
    case DIV:   div32(left, right); break;
    case MOD:   mod32(left, right); break;
    case RSH:   rsh32(left, right); break;
    case LSH:   lsh32(left, right); break;
    case OR:     or32(left, right); break;
    case XOR:   xor32(left, right); break;
    case AND:   and32(left, right); break;
    case EQ:     eq32(left, right); break;
    case NE:     ne32(left, right); break;
    case LE:     le32(left, right); break;
    case GE:     ge32(left, right); break;
    case LT:     lt32(left, right); break;
    case GT:     gt32(left, right); break;
    }
  return (NULL);
  }

/*
** scan for next token
*/
token(want) int want; {
  int len;
  if(ct)  return (match(want, ct));     /* already have a token */
  while(isspace(*ep))  ++ep;
  if(register()) return (match(want, REG));
  switch(*ep++) {
    case '[': return (match(want, LBR));
    case ']': return (match(want, RBR));
    case '+': return (match(want, PLUS));
    case '-': return (match(want, MINUS));
    case '|': return (match(want, OR));
    case '^': return (match(want, XOR));
    case '~': return (match(want, CPL));
    case '&': return (match(want, AND));
    case '*': return (match(want, MULT));
    case '/': return (match(want, DIV));
    case '%': return (match(want, MOD));
    case '(': return (match(want, LPN));
    case ')': return (match(want, RPN));
    case '.': return (match(want, PRD));
    case ':': return (match(want, COL));
    case ',': --ep;             return (match(want, EOE));
    case '!': if(*ep++ == '=')  return (match(want, NE));
              --ep;             return (match(want, NOT));
    case '<': if(*ep++ == '=')  return (match(want, LE));
              --ep;
              if(*ep++ == '<')  return (match(want, LSH));
              --ep;             return (match(want, LT));
    case '>': if(*ep++ == '=')  return (match(want, GE));
              --ep;
              if(*ep++ == '>')  return (match(want, RSH));
              --ep;             return (match(want, GT));
    case '=': if(*ep++ == '=')  return (match(want, EQ));
              --ep;
    }
  if(atend(*--ep) || same(ep, "DUP")) return (match(want, EOE));
  if(same(ep, "OFFSET"))    {ep += 6; return (match(want, OFF));}
  if(same(ep, "SEG"))       {ep += 3; return (match(want, SEG));}
  if(same(ep, "SHORT"))     {ep += 5; return (match(want, SHORT));}
#ifdef MASM_OPS
  if(same(ep, "NOT")) {ep += 3; return (match(want, CPL));}
  if(same(ep, "AND")) {ep += 3; return (match(want, AND));}
  if(same(ep, "OR" )) {ep += 3; return (match(want, OR)); }
  if(same(ep, "XOR")) {ep += 3; return (match(want, XOR));}
  if(same(ep, "MOD")) {ep += 3; return (match(want, MOD));}
  if(same(ep, "EQ" )) {ep += 2; return (match(want, EQ)); }
  if(same(ep, "NE" )) {ep += 2; return (match(want, NE)); }
  if(same(ep, "LT" )) {ep += 2; return (match(want, LT)); }
  if(same(ep, "LE" )) {ep += 2; return (match(want, LE)); }
  if(same(ep, "GT" )) {ep += 2; return (match(want, GT)); }
  if(same(ep, "GE" )) {ep += 2; return (match(want, GE)); }
#endif
  ep = getsym(ep, upper, YES);
  if(stsym[0] == '$' && stsym[1] == 0)
    return (match(want, LOC));
  if(stsym[0])
    return (match(want, SYM));
  if(len = getnum(ep)) {
    ep += len;
    return (match(want, NUM));
    }
  return (NO);
  }

/*
** scan for a register name
*/
register() {
  char *op;
  op = ep;
  ep = getsym(ep, YES, YES);         /* xlate to upper case */
  if(strlen(stsym) == 2) {
    switch((stsym[0] << 8) | stsym[1]) {
      case 'AL': return (regcode = AL);
      case 'BL': return (regcode = BL);
      case 'CL': return (regcode = CL);
      case 'DL': return (regcode = DL);
      case 'AH': return (regcode = AH);
      case 'BH': return (regcode = BH);
      case 'CH': return (regcode = CH);
      case 'DH': return (regcode = DH);
      case 'AX': return (regcode = AX);
      case 'BX': return (regcode = BX);
      case 'CX': return (regcode = CX);
      case 'DX': return (regcode = DX);
      case 'SP': return (regcode = SP);
      case 'BP': return (regcode = BP);
      case 'SI': return (regcode = SI);
      case 'DI': return (regcode = DI);
      case 'CS': return (regcode = CS);
      case 'SS': return (regcode = SS);
      case 'DS': return (regcode = DS);
      case 'ES': return (regcode = ES);
      case 'FS': return (regcode = FS);
      case 'GS': return (regcode = GS);
      case 'ST': if(token(LPN) == NO) return (regcode = ST0);
                 if(token(NUM) && token(RPN))
                   switch(number[0]) {
                     case 0: return (regcode = ST0);
                     case 1: return (regcode = ST1);
                     case 2: return (regcode = ST2);
                     case 3: return (regcode = ST3);
                     case 4: return (regcode = ST4);
                     case 5: return (regcode = ST5);
                     case 6: return (regcode = ST6);
                     case 7: return (regcode = ST7);
                     }
      }
    }
  else if(strlen(stsym) == 3) {
    switch(stsym[0]) {
      case 'E': switch((stsym[1] << 8) | stsym[2]) {
                  case 'AX': return (regcode = EAX);
                  case 'BX': return (regcode = EBX);
                  case 'CX': return (regcode = ECX);
                  case 'DX': return (regcode = EDX);
                  case 'SP': return (regcode = ESP);
                  case 'BP': return (regcode = EBP);
                  case 'SI': return (regcode = ESI);
                  case 'DI': return (regcode = EDI);
                  }
      case 'C': switch((stsym[1] << 8) | stsym[2]) {
                  case 'R0': return (regcode = CR0);
                  case 'R2': return (regcode = CR2);
                  case 'R3': return (regcode = CR3);
                  }
      case 'D': switch((stsym[1] << 8) | stsym[2]) {
                  case 'R0': return (regcode = DR0);
                  case 'R1': return (regcode = DR1);
                  case 'R2': return (regcode = DR2);
                  case 'R3': return (regcode = DR3);
                  case 'R6': return (regcode = DR6);
                  case 'R7': return (regcode = DR7);
                  }
      case 'T': switch((stsym[1] << 8) | stsym[2]) {
                  case 'R6': return (regcode = TR6);
                  case 'R7': return (regcode = TR7);
                  }
      }
    }
  ep = op;
  return (0);
  }

/*
** recognize *1, *2, *3, *4 for 80386 scaling indexes
*/
scaled(t) unsigned *t; {
  if(token(MULT) && token(NUM)) {
    if(number[0] == 1) {               return (YES);}
    if(number[0] == 2) {t[3] = 0x0100; return (YES);}
    if(number[0] == 4) {t[3] = 0x0200; return (YES);}
    if(number[0] == 8) {t[3] = 0x0300; return (YES);}
    }
  return (NO);
  }

/*
** recognize ... PTR operator
*/
isptr() {
  int ty;
  if(ct == SYM) {
    if(same(stsym, "BYTE" )) {ty =  1; goto maybe;}
    if(same(stsym, "WORD" )) {ty =  2; goto maybe;}
    if(same(stsym, "DWORD")) {ty =  4; goto maybe;}
    if(same(stsym, "FWORD")) {ty =  6; goto maybe;}
    if(same(stsym, "QWORD")) {ty =  8; goto maybe;}
    if(same(stsym, "TBYTE")) {ty = 10; goto maybe;}
    if(same(stsym, "NEAR" )) {ty = 11; goto maybe;}
    if(same(stsym, "FAR"  )) {ty = 12; goto maybe;}
    }
  return (0);

  maybe:
  if(same(ep, "PTR")) {ep += 3; ct = NULL; return (ty);}
  return (0);
  }

/*
** match what was sought and what was found
*/
match(want, have) int want, have; {
  ct = have;                    /* new current token */
  if(ct == want) {              /* was it sought? */
    if(ct != EOE)  ct = NULL;   /* yes, pass it by */
    return (YES);               /* caller has a hit */
    }
  return (NO);                  /* sorry, no hit */
  }

/*
** Get
**   (1) character
**   (2) hex, dec, or oct integer
**   (3) real number
** as binary value in number.
** Return length of field processed, else zero.
*/ 
getnum(cp) unsigned char *cp; {
  unsigned bump, len;
  unsigned char *end;
  if(((cp[0] == '\'') || (cp[0] == '"')) && (cp[0] == cp[2])) {
    number[0] = cp[1];      /* ascii value of character */
    number[1] = 0;
    return (3);
    }
  if(isdigit(*cp)) {
    len = atolb(cp, number, base(cp, &bump, &end));
    if(len != (end - cp))  experr();         /* bad number */
    return ((end - cp) + bump);
    }
  return (0);
  }

/*
** discover apparent base
*/
base(cp, bump, end) unsigned char *cp; unsigned *bump, *end; {
  *end = cp;
  while(isxdigit(*cp)) ++cp;
  if(*end < cp) {
    *bump = 1;
    *end = cp;
    switch(*cp) {
      case 'q': case 'Q':
      case 'o': case 'O': return (8);
      case 'h': case 'H': return (16); 
      }
    *end = --cp;
    switch(*cp) {
      case 'b': case 'B': return (2);
      case 'd': case 'D': return (10);
      }
    ++*end;
    }
  *bump = 0;
  return (10);
  }

/*
** get a symbol into stsym
*/
getsym(cp, up, ref) char *cp; int up, ref; {
  int j;
  j = badsym = gotcolon = gotcomma = gotnam = 0;
  if(!isdigit(*cp)) {
    while(YES) {
      switch(toupper(*cp)) {
        default: 
          if(ref)  
            break;
          badsym = YES;
        case 'A':  case 'B':  case 'C':  case 'D':  case 'E':  case 'F':
        case 'G':  case 'H':  case 'I':  case 'J':  case 'K':  case 'L':
        case 'M':  case 'N':  case 'O':  case 'P':  case 'Q':  case 'R':
        case 'S':  case 'T':  case 'U':  case 'V':  case 'W':  case 'X':
        case 'Y':  case 'Z':  case '0':  case '1':  case '2':  case '3':
        case '4':  case '5':  case '6':  case '7':  case '8':  case '9':
        case '_':  case '$':  case '?':  case '@':
          if(j < MAXNAM) {
            if(up) {
              stsym[j++] = toupper(*cp);
            }
            else {
              stsym[j++] = *cp;
            }
          }
          ++cp;
          continue;
        case ':': 
          if(!ref) {
            gotcolon = YES; ++cp;
          } 
          break;
        case ',': 
          if(!ref) {
            gotcomma = YES; ++cp;
          } 
          break;
          synerr();
        case ' ':
        case '\t':
        case '\n':
        case '\'':
        case COMMENT:  
        case NULL:
      }
      while(isspace(*cp)) { 
        ++cp;
      }
      break;
    }
  }
  stsym[j] = NULL;
  if(j && !gotcolon)
    gotnam = YES;
  else   
    gotnam = NO;
  return cp;
}

/*
** encode type of constant
*/
ctype(n) int n[]; {
  if(n[1] == 0) {
    if(n[0] == 1) return (ONE);
    if(n[0] == 3) return (THREE);
    }
  if(cmp32(n, I6min ) >= 0 && cmp32(n, I6max ) <= 0) return (I6);
  if(cmp32(n, I8min ) >= 0 && cmp32(n, I8max ) <= 0) return (I8);
  if(osa == 2
  ||(cmp32(n, I16min) >= 0 && cmp32(n, I16max) <= 0)) return (I16);
  return (I32);
  } 

