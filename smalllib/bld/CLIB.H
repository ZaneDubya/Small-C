/*
** CLIB.H -- Definitions for Small-C library functions.
**
** Copyright 1983  L. E. Payne and J. E. Hendrix
*/

/*
** Misc parameters
*/
#define MAXFILES 20  /* maximum open files */
#define DOSEOF   26  /* DOS end-of-file byte */
#define ARCHIVE  32  /* file archive bit */

/*
** DOS function calls
*/
#define CREATE   60  /* make file */
#define OPEN     61  /* open file */
#define CLOSE    62  /* close file or device */
#define READ     63  /* read from a file */
#define WRITE    64  /* write to a file */
#define DELETE   65  /* delete file */
#define SEEK     66  /* seek within a file */
#define CONTROL  68  /* control device */
#define FORCE    70  /* force use of a handle */
#define RETDOS   76  /* close files and return to DOS */
#define FNDFIL   78  /* find first occurrence of a file */
#define FNDNXT   79  /* find next occurrence of a file */
#define RENAME   86  /* rename file */

/*
** File status bits
*/
#define OPNBIT   1  /* open condition */
#define EOFBIT   2  /* end-of-file condition */
#define ERRBIT   4  /* error condition */

/*
** File positioning origins
*/
#define FROM_BEG  0  /* from beginning of file */
#define FROM_CUR  1  /* from current position */
#define FROM_END  2  /* from end of file */

/*
** Buffer usage codes
** NULL means the buffer is not used.
*/
#define EMPTY     1  /* buffer is currently empty */
#define IN        2  /* buffer is currently holding input data */
#define OUT       3  /* buffer is currently holding output data */

/*
** ASCII characters
*/
#define ABORT    3
#define RUB      8
#define PAUSE   19
#define WIPE    24
#define DEL    127

