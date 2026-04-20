// STDIO.H -- Standard Small C Definitions.
#define stdin    0  // file descriptor for standard input file
#define stdout   1  // file descriptor for standard output file
#define stderr   2  // file descriptor for standard error file
#define stdaux   3  // file descriptor for standard auxiliary port
#define stdprn   4  // file descriptor for standard printer
#define FILE  char  // supports "FILE *fp;" declarations
#define ERR   (-2)  // return value for errors
#define EOF   (-1)  // return value for end-of-file
#define YES      1  // true
#define NO       0  // false
#define NULL     0  // zero
#define CR      13  // ASCII carriage return
#define LF      10  // ASCII line feed
#define BELL     7  // ASCII bell
#define SPACE  ' '  // ASCII space
#define NEWLINE LF  // Small C newline character

// Small-C SmallLib function prototypes:
extern void abort(int error);
extern int abs(int nbr);
extern int atoi(char *s);
extern int atoib(char *s, int b);
extern int avail(int abort);
extern int auxbuf(int fd, int size);
extern int bseek(int fd, int offset[], int base);
extern int btell(int fd, int offset[]);
extern char *calloc(unsigned n, unsigned size);
extern void clearerr(int fd);
extern int cseek(int fd, int offset, int base);
extern int ctell(int fd);
extern int ctellc(int fd);
extern int dtoi(char *decstr, int *nbr);
extern void exit(int ec);
extern int fclose(int fd);
extern int feof(int fd);
extern int ferror(int fd);
extern int fgetc(int fd);
extern char *fgets(char *str, unsigned size, unsigned fd);
extern int fopen(char *fn, char *mode);
extern int fprintf(int fd, char *fmt, ...);
extern int fputc(int ch, int fd);
extern void fputs(char *string, int fd);
extern char *free(char *ptr);
extern int fread(char *buf, unsigned sz, unsigned n, unsigned fd);
extern int freopen(char *fn, char *mode, int fd);
extern int fscanf(int fd, char *fmt, ...);
extern int fwrite(char *buf, unsigned sz, unsigned n, unsigned fd);
extern int getarg(int n, char *s, int size, int argc, char **argv);
extern int getchar();
extern int isalnum(int c);
extern int isalpha(int c);
extern int isascii(unsigned c);
extern int isatty(int fd);
extern int iscntrl(int c);
extern int iscons(int fd);
extern int isdigit(int c);
extern int isgraph(int c);
extern int islower(int c);
extern int isprint(int c);
extern int ispunct(int c);
extern int isspace(int c);
extern int isupper(int c);
extern int isxdigit(int c);
extern void itoa(int n, char *s);
extern void itoab(int n, char *s, int b);
extern char *itod(int nbr, char str[], int sz);
extern char *itoo(int nbr, char str[], int sz);
extern char *itou(int nbr, char str[], int sz);
extern char *itox(int nbr, char str[], int sz);
extern void left(char *str);
extern int lexcmp(char *s, char *t);
extern char *ltoa(long n, char *s);
extern void ltoab(long n, char *s, int b);
extern char *malloc(unsigned size);
extern int otoi(char *octstr, int *nbr);
extern void pad(char *dest, unsigned ch, unsigned n);
extern int poll(int pause);
extern int printf(char *fmt, ...);
extern int scanf(char *fmt, ...);
extern int sprintf(char *buf, char *fmt, ...);
extern int putchar(int ch);
extern void puts(char *string);
extern int read(unsigned fd, char *buf, unsigned n);
extern int rename(char *from, char *to);
extern void reverse(char *s);
extern int rewind(int fd);
extern int sign(int nbr);
extern char *skip(int n, char *str);
extern char *strcat(char *s, char *t);
extern char *strchr(char *str, char c);
extern int strcmp(char *s, char *t);
extern char *strcpy(char *s, char *t);
extern int strlen(char *s);
extern char *strncat(char *s, char *t, int n);
extern int strncmp(char *s, char *t, int n);
extern char *strncpy(char *dest, char *sour, int n);
extern char *strrchr(char *s, char c);
extern void structcp(char *dst, char *src, int n);
extern int toascii(int c);
extern int tolower(int c);
extern int toupper(int c);
extern int ungetc(int c, int fd);
extern int unlink(char *fn);
extern int utoi(char *decstr, int *nbr);
extern int write(unsigned fd, char *buf, unsigned n);
extern int xtoi(char *hexstr, int *nbr);
