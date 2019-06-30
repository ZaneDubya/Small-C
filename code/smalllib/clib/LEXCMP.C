
char _lex[128] = {
       0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  /* NUL - /       */
      10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
      20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
      30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
      40, 41, 42, 43, 44, 45, 46, 47,
      65, 66, 67, 68, 69, 70, 71, 72, 73, 74,  /* 0-9           */
      48, 49, 50, 51, 52, 53, 54,              /* : ; < = > ? @ */
      75, 76, 77, 78, 79, 80, 81, 82, 83, 84,  /* A-Z           */
      85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
      95, 96, 97, 98, 99,100,
      55, 56, 57, 58, 59, 60,                  /* [ \ ] ^ _ `   */
      75, 76, 77, 78, 79, 80, 81, 82, 83, 84,  /* a-z           */
      85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
      95, 96, 97, 98, 99,100,
      61, 62, 63, 64,                          /* { | } ~       */
     127};                                     /* DEL           */

/*
** lexcmp(s, t) - Return a number <0, 0, or >0
**                as s is <, =, or > t.
*/
lexcmp(s, t) char *s, *t; {
  while(lexorder(*s, *t) == 0)
    if(*s++) ++t;
    else return (0);
  return (lexorder(*s, *t));
  }

/*
** lexorder(c1, c2)
**
** Return a negative, zero, or positive number if
** c1 is less than, equal to, or greater than c2,
** based on a lexicographical (dictionary order)
** colating sequence.
**
*/
lexorder(c1, c2) int c1, c2; {
  return(_lex[c1] - _lex[c2]);
  }

