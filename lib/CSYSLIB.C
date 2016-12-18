/*
** CSYSLIB -- System-Level Library Functions
*/

#include "stdio.h"
#include "clib.h"

/*
****************** System Variables ********************
*/

int
  _cnt=1,             /* arg count for main */
  _vec[20],           /* arg vectors for main */
  _status[MAXFILES] = {OPNBIT, OPNBIT, OPNBIT, OPNBIT, OPNBIT},
  _cons  [MAXFILES],  /* fast way to tell if file is the console */
  _nextc [MAXFILES] = {EOF, EOF, EOF, EOF, EOF},
  _bufuse[MAXFILES],  /* current buffer usage: NULL, EMPTY, IN, OUT */
  _bufsiz[MAXFILES],  /* size of buffer */
  _bufptr[MAXFILES],  /* aux buffer address */
  _bufnxt[MAXFILES],  /* address of next byte in buffer */
  _bufend[MAXFILES],  /* address of end-of-data in buffer */
  _bufeof[MAXFILES];  /* true if current buffer ends file */

char
 *_memptr,           /* pointer to free memory. */
  _arg1[]="*";       /* first arg for main */

/*
*************** System-Level Functions *****************
*/

/*
**  Process command line, allocate default buffer to each fd,
**  execute main(), and exit to DOS.  Must be executed with es=psp.
**  Small default buffers are allocated because a high price is paid for
**  byte-by-byte calls to DOS.  Tests gave these results for a simple
**  copy program:
**
**          chunk size       copy time in seconds
**              1                    36
**              5                    12
**             25                     6
**             50                     6
*/
_main() {
  int fd;
  _parse();
  for(fd = 0; fd < MAXFILES; ++fd) auxbuf(fd, 32);
  if(!isatty(stdin))  _bufuse[stdin]  = EMPTY;
  if(!isatty(stdout)) _bufuse[stdout] = EMPTY;
  main(_cnt, _vec);
  exit(0);
  }

/*
** Parse command line and setup argc and argv.
** Must be executed with es == psp
*/
_parse() {
  char *ptr;
#asm
  mov     cl,es:[80h]  ; get parameter string length
  mov     ch,0       
  push    cx           ; save it
  inc     cx
  push    cx           ; 1st __alloc() arg
  mov     ax,1
  push    ax           ; 2nd __alloc() arg
  call    __alloc      ; allocate zeroed memory for args
  add     sp,4
  mov     [bp-2],ax    ; ptr = addr of allocated memory
  pop     cx
  push    es           ; exchange
  push    ds           ; es         (source)
  pop     es           ;    and
  pop     ds           ;        ds  (destination)
  mov     si,81h       ; source offset
  mov     di,[bp-2]    ; destination offset
  rep     movsb        ; move string
  mov     al,0
  stosb                ; terminate with null byte
  push    es
  pop     ds           ; restore ds
#endasm
  _vec[0]=_arg1;       /* first arg = "*" */
  while (*ptr) {
    if(isspace(*ptr)) {++ptr; continue;}
    if(_cnt < 20) _vec[_cnt++] = ptr;
    while(*ptr) {
      if(isspace(*ptr)) {*ptr = NULL; ++ptr; break;}
      ++ptr;
      }
    }
  }

/*
** Open file on specified fd.
*/
_open(fn, mode, fd) char *fn, *mode; int *fd; {
  int rw, tfd;
  switch(mode[0]) {
    case 'r': {
      if(mode[1] == '+') rw = 2; else rw = 0;
      if ((tfd = _bdos2((OPEN<<8)|rw, NULL, NULL, fn)) < 0) return (NO);
      break;
      }
    case 'w': {
      if(mode[1] == '+') rw = 2; else rw = 1;
    create:
      if((tfd = _bdos2((CREATE<<8), NULL, ARCHIVE, fn)) < 0) return (NO);
      _bdos2(CLOSE<<8, tfd, NULL, NULL);
      if((tfd = _bdos2((OPEN<<8)|rw, NULL, NULL, fn)) < 0) return (NO);
      break;
      }
    case 'a': {
      if(mode[1] == '+') rw = 2; else rw = 1;
      if((tfd = _bdos2((OPEN<<8)|rw, NULL, NULL, fn)) < 0) goto create;
      if(_bdos2((SEEK<<8)|FROM_END, tfd, NULL, 0) < 0) return (NO);
      break;
      }
    default: return (NO);
    }
  _empty(tfd, YES);
  if(isatty(tfd)) _bufuse[tfd] = NULL;
  *fd = tfd;
  _cons  [tfd] = NULL;
  _nextc [tfd] = EOF;
  _status[tfd] = OPNBIT;
  return (YES);
  }

/*
** Binary-stream input of one byte from fd.
*/
_read(fd) int fd; {
  unsigned char ch;
  if(_nextc[fd] != EOF) {
    ch = _nextc[fd];
    _nextc[fd] = EOF;
    return (ch);
    }
  if(iscons(fd))  return (_getkey());
  if(_bufuse[fd]) return(_readbuf(fd));
  switch(_bdos2(READ<<8, fd, 1, &ch)) {
     case 1:  return (ch);
     case 0: _seteof(fd); return (EOF);
    default: _seterr(fd); return (EOF);
    }
  }

/*
** Fill buffer if necessary, and return next byte.
*/
_readbuf(fd) int fd; {
  int got, chunk;
  char *ptr, *max;
  if(_bufuse[fd] == OUT && _flush(fd)) return (EOF);
  while(YES) {
    ptr = _bufnxt[fd];
    if(ptr < _bufend[fd]) {++_bufnxt[fd]; return (*ptr);}
    if(_bufeof[fd]) {_seteof(fd); return (EOF);}
    max = (ptr = _bufend[fd] = _bufptr[fd]) + _bufsiz[fd];
    do {         /* avoid DMA problem on physical 64K boundary */
      if((max - ptr) < 512) chunk = max - ptr;
      else                  chunk = 512;
      ptr += (got = _bdos2(READ<<8, fd, chunk, ptr));
      if(got < chunk) {_bufeof[fd] = YES; break;}
      } while(ptr < max);
    _bufend[fd] = ptr;
    _bufnxt[fd] = _bufptr[fd];
    _bufuse[fd] = IN;
    }
  }

/*
** Binary-Stream output of one byte to fd.
*/
_write(ch, fd) int ch, fd; {
  if(_bufuse[fd]) return(_writebuf(ch, fd));
  if(_bdos2(WRITE<<8, fd, 1, &ch) != 1) {
    _seterr(fd);
    return (EOF);
    }
  return (ch);
  }

/*
** Empty buffer if necessary, and store ch in buffer.
*/
_writebuf(ch, fd) int ch, fd; {
  char *ptr;
  if(_bufuse[fd] == IN && _backup(fd)) return (EOF);
  while(YES) {
    ptr = _bufnxt[fd];
    if(ptr < (_bufptr[fd] + _bufsiz[fd])) {
      *ptr = ch;
      ++_bufnxt[fd];
      _bufuse[fd] = OUT;
      return (ch);
      }
    if(_flush(fd)) return (EOF);
    }
  }

/*
** Flush buffer to DOS if dirty buffer.
** Reset buffer pointers in any case.
*/
_flush(fd) int fd; {
  int i, j, k, chunk;
  if(_bufuse[fd] == OUT) {
    i = _bufnxt[fd] - _bufptr[fd];
    k = 0;
    while(i > 0) {     /* avoid DMA problem on physical 64K boundary */
      if(i < 512) chunk = i;
      else        chunk = 512;
      k += (j = _bdos2(WRITE<<8, fd, chunk, _bufptr[fd] + k));
      if(j < chunk) {_seterr(fd); return (EOF);}
      i -= j;
      }
    }
  _empty(fd, YES);
  return (NULL);
  }

/*
** Adjust DOS file position to current point.
*/
_adjust(fd) int fd; {
  if(_bufuse[fd] == OUT) return (_flush(fd));
  if(_bufuse[fd] == IN ) return (_backup(fd));
  }

/*
** Backup DOS file position to current point.
*/
_backup(fd) int fd; {
  int hi, lo;
  if(lo = _bufnxt[fd] - _bufend[fd]) {
    hi = -1;
    if(!_seek(FROM_CUR, fd, &hi, &lo)) {
      _seterr(fd);
      return (EOF);
      }
    }
  _empty(fd, YES);
  return (NULL);
  }

/*
** Set buffer controls to empty status.
*/
_empty(fd, mt) int fd, mt; {
  _bufnxt[fd] = _bufend[fd] = _bufptr[fd];
  _bufeof[fd] = NO;
  if(mt) _bufuse[fd] = EMPTY;
  }

/*
** Return fd's open mode, else NULL.
*/
_mode(fd) char *fd; {
  if(fd < MAXFILES) return (_status[fd]);
  return (NULL);
  }

/*
** Set eof status for fd and
*/
_seteof(fd) int fd; {
  _status[fd] |= EOFBIT;
  }

/*
** Clear eof status for fd.
*/
_clreof(fd) int fd; {
  _status[fd] &= ~EOFBIT;
  }

/*
** Set error status for fd.
*/
_seterr(fd) int fd; {
  _status[fd] |= ERRBIT;
  }

/*
** Clear error status for fd.
*/
_clrerr(fd) int fd; {
  _status[fd] &= ~ERRBIT;
  }

/*
** Allocate n bytes of (possibly zeroed) memory.
** Entry: n = Size of the items in bytes.
**    clear = "true" if clearing is desired.
** Returns the address of the allocated block of memory
** or NULL if the requested amount of space is not available.
*/
_alloc(n, clear) unsigned n, clear; {
  char *oldptr;
  if(n < avail(YES)) {
    if(clear) pad(_memptr, NULL, n);
    oldptr = _memptr;
    _memptr += n;
    return (oldptr);
    }
  return (NULL);
  }

/*
** Issue extended BDOS function and return result. 
** Entry: ax = function code and sub-function
**        bx, cx, dx = other parameters
*/
_bdos2(ax, bx, cx, dx) int ax, bx, cx, dx; {
#asm
  push bx         ; preserve secondary register
  mov  dx,[bp+4]
  mov  cx,[bp+6]
  mov  bx,[bp+8]
  mov  ax,[bp+10] ; load DOS function number
  int  21h        ; call bdos
  jnc  __bdos21   ; no error
  neg  ax         ; make error code negative  
__bdos21:
  pop  bx         ; restore secondary register
#endasm
  }

/*
** Issue LSEEK call
*/
_seek(org, fd, hi, lo) int org, fd, hi, lo; {
#asm
  push bx         ; preserve secondary register
  mov  bx,[bp+4]
  mov  dx,[bx]    ; get lo part of destination
  mov  bx,[bp+6]
  mov  cx,[bx]    ; get hi part of destination
  mov  bx,[bp+8]  ; get file descriptor
  mov  al,[bp+10] ; get origin code for seek
  mov  ah,42h     ; move-file-pointer function
  int  21h        ; call bdos
  jnc  __seek1    ; error?
  xor  ax,ax      ; yes, return false
  jmp  __seek2 
__seek1:          ; no, set hi and lo
  mov  bx,[bp+4]  ; get address of lo
  mov  [bx],ax    ; store low part of new position
  mov  bx,[bp+6]  ; get address of hi
  mov  [bx],dx    ; store high part of new position
  mov  ax,1       ; return true
__seek2:
  pop  bx         ; restore secondary register
#endasm
  }

/*
** Test for keyboard input
*/
_hitkey() {
#asm
  mov  ah,1       ; sub-service = test keyboard
  int  16h        ; call bdos keyboard services
  jnz  __hit1
  xor ax,ax       ; nothing there, return false
  jmp  __hit2
__hit1:
  mov  ax,1       ; character ready, return true
__hit2:
#endasm
  }

/*
** Return next keyboard character
*/
_getkey() {
#asm
  mov  ah,0       ; sub-service = read keyboard
  int  16h        ; call bdos keyboard services
  or   al,al      ; special character?
  jnz  __get2     ; no
  mov  al,ah      ; yes, move it to al
  cmp  al,3       ; ctl-2 (simulated null)?
  jne  __get1     ; no
  xor  al,al      ; yes, report zero
  jmp  __get2
__get1:
  add  al,113     ; offset to range 128-245
__get2:
  xor  ah,ah      ; zero ah
#endasm
  }

