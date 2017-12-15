/*
** obj3.h -- third header for .OBJ file processing
*/

/*
** record-type codes
*/
#define EOBJ    0x00   /* end of obj file */
#define BLKDEF  0x7A   /* start of a program block */
#define BLKEND  0x7C   /* end of a program block */
#define COMENT  0x88   /* comment record */
#define THEADR  0x80   /* t-module header record */
#define MODEND  0x8A   /* module end record */
#define MOD386  0x8B   /* module end record (386) */
#define EXTDEF  0x8C   /* ext ref declaration record */
#define TYPDEF  0x8E   /* variable type definition */
#define PUBDEF  0x90   /* public declaration record */
#define PUB386  0x91   /* public declaration record (386) */
#define LINNUM  0x94   /* source code line number */
#define LNAMES  0x96   /* list of names record */
#define SEGDEF  0x98   /* segment definition record */
#define SEG386  0x99   /* segment definition record (386) */
#define GRPDEF  0x9A   /* group definition record */
#define FIXUPP  0x9C   /* fixup record */
#define FIX386  0x9D   /* fixup record (386) */
#define LEDATA  0xA0   /* logical enumerated data record */
#define LED386  0xA1   /* logical enumerated data record (386) */
#define LIDATA  0xA2   /* logical iterated data record */
#define LID386  0xA3   /* logical iterated data record (386) */
#define LHEADR  0xF0   /* library Header */
#define LFOOTR  0xF1   /* library Footer */

#define MAXCONT 512    /* maximum size of contents field */
#define MAXSYM   40    /* maximum symbol length allowed in OBJ file */
#define ONES     -1    /* all one bits */

