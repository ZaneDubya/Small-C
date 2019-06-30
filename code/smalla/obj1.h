/*
** obj1.h -- first header for .OBJ file processing
*/

#define FSIZE  (100*7)                /* fixup buffer size (max*avg) */
#define OSIZE   1027                  /* obuf[] size - DON'T INCREASE! */
#define OFREE ((OSIZE - 1) - onext)   /* obuf[] space remaining */
