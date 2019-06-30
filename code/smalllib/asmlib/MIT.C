/*
** mit.c -- machine instruction table functions
*/
#include <stdio.h>
#include "asm.h"

extern unsigned
  mntbl[],            /* table of mnemonic indexes */
  optbl[],            /* table of operand indexes */
  mitbl[],            /* table of machine instr indexes */
  mncnt,              /* entries in mnemonic table */
  opcnt,              /* entries in operand table */
  micnt;              /* entries in machine instr table */

extern unsigned char
  mitbuf[];           /* instruction syntax buffer */

unsigned
  looks,              /* number of looks to find it */
  hashval;            /* global hash value for speed */

/*
** find mnemonic or operand
** return index of sought entry, else EOF
*/
find(str, tbl, cnt) unsigned char *str; unsigned tbl[], cnt; {
  unsigned h;
  h = hash(str, cnt);
  if(tbl[h << 1] == EOF) {
    ++looks;
    return (EOF);
    }
  do {
    ++looks;
    if(strcmp(mitbuf + tbl[h << 1], str) == 0) break;
    } while((h = tbl[(h << 1) + 1]) != EOF);
  return (h);
  }

/*
** calculate hash value
*/
hash(ptr, cnt) unsigned char *ptr; unsigned cnt; {
  hashval = 0;
  while(*ptr > ' ') {
    hashval = (hashval << 1) + toupper(*ptr++);
    }
  return (hashval % cnt);
  }

/*
** find instruction
** return index of sought entry, else EOF
*/
findi(instr) unsigned *instr; {
  unsigned *ip, h;
  h = hashi(instr);
  if(mitbl[h << 1] == EOF) {
    ++looks;
    return (EOF);
    }
  do {
    ++looks;
    ip = mitbuf + mitbl[h << 1];
    if((ip[0] == instr[0]) && (ip[1] == instr[1])) break;
    } while((h = mitbl[(h << 1) + 1]) != EOF);
  return (h);
  }

/*
** calculate instruction hash value
*/
hashi(inst) unsigned inst[]; {
  return(((inst[1] << 4) + inst[0]) % micnt);
  }

