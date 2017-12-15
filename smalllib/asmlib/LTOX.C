/*
** ltox -- converts 32-bit nbr to hex string of length sz
**         right adjusted and blank filled, returns str
**
**        if sz > 0 terminate with null byte
**        if sz = 0 find end of string
**        if sz < 0 use last byte for data
*/
ltox(nbr, str, sz) int nbr[]; char str[]; int sz;  {
  unsigned digit, offset, n[2];
  if(sz > 0) str[--sz] = 0;
  else if(sz < 0) sz = -sz;
  else while(str[sz] != 0) ++sz;
  n[0] = nbr[0];
  n[1] = nbr[1];
  while(sz) {
    digit = n[0] & 0x0F;              /* extract low-order digit */
    n[0] = (n[0] >> 4) & 0x0FFF;      /* shift right one nibble */
    n[0] |= (n[1] & 0x000F) << 12;
    n[1] = (n[1] >> 4) & 0x0FFF;
    if(digit < 10) offset = 48;       /* set ASCII base if 0-9 */
    else           offset = 55;       /* set ASCII base if A-F */
    str[--sz] = digit + offset;       /* store ASCII digit */
    if(n[0] == 0 && n[1] == 0) break; /* done? */
    }
  while(sz) str[--sz] = ' ';
  return (str);
  }

