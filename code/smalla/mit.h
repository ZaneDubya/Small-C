/*
** mit.h -- machine instruction table
*/

extern unsigned
  mntbl[],            /* table of mnemonic indexes */
  optbl[],            /* table of operand indexes */
  mitbl[],            /* table of machine instr indexes */
  mncnt,              /* entries in mnemonic table */
  opcnt,              /* entries in operand table */
  micnt;              /* entries in machine instr table */

extern unsigned char
  mitbuf[],           /* instruction syntax buffer */
  otnext[],           /* operand-type mapping link */
  otmap [];           /* operand-type mapping value */

extern unsigned       /* reside in mit.c */
  looks,              /* number of looks to find it */
  hashval;            /* global hash value for speed */

