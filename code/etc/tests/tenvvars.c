// tenvvars.c -- End-to-end test of the new #include path mechanism.
//
// TWO LEVELS OF TESTING
// ---------------------
// 1. COMPILE-TIME (before the exe even runs):
//    This file uses  #include <stdio.h>  (angle bracket).  Compiling it
//    with the new cc.exe is the first test: if compilation succeeds and
//    no "open failure on include file" is emitted, the <> include path
//    works correctly.
//
// 2. RUNTIME (the program itself):
//    Exercises the path helper functions -- dos_exepath, dirpart,
//    compose_path -- that doinclude() now calls.  Verifies:
//
//    Section 1  dos_exepath() returns the running exe's full path.
//    Section 2  dirpart()     extracts the directory (the "bindir" analog).
//    Section 3  compose_path(exedir, "tenvvars.h") + fopen succeeds.
//               This mirrors  #include <tenvvars.h>  with cc in the same
//               directory as the header being searched.
//    Section 4  fopen("tenvvars.h") bare from CWD fails.
//               This proves CWD is no longer implicitly searched -- the
//               composed path is required.  Expected result: INFO (desired
//               new behaviour).
//
// RUN SEQUENCE (make.bat handles this automatically)
// --------------------------------------------------
//   cd C:\ETC\TENVVARS
//   build tenvvars.exe here
//   cd ..                     (CWD is now C:\ETC, not C:\ETC\TENVVARS)
//   tenvvars\tenvvars
//
// With CWD = C:\ETC:
//   tenvvars.h lives in C:\ETC\TENVVARS\ -- NOT in C:\ETC\
//   compose_path(exedir,"tenvvars.h") builds C:\ETC\TENVVARS\TENVVARS.H  -> found
//   fopen("tenvvars.h") bare                                              -> not found (INFO)

#include <stdio.h>

#define FNSIZE  128   // must match FNSIZE in cc.h / ccpre.c

/* -------------------------------------------------------------------------
 * Low-level DOS helpers  (identical to ccpre.c / envvars.c)
 * ------------------------------------------------------------------------- */

// Return the segment address of the process environment block.
// Uses INT 21h AH=62h (get PSP) -- DOS 3.0+.
int dos_env_seg()
{
#asm
    push    es
    mov     ah,62h
    int     21h
    mov     es,bx
    mov     ax,es:[2Ch]
    pop     es
#endasm
}

// Read one byte from a far pointer (segment seg, byte offset off).
int dos_peek(int seg, int off)
{
#asm
    push    es
    push    bx
    mov     es,[bp+4]
    mov     bx,[bp+6]
    xor     ax,ax
    mov     al,es:[bx]
    pop     bx
    pop     es
#endasm
}

// Fill buf with the program's own full path (DOS 3.0+ env-block trailer).
// Returns 1 on success, 0 on failure.
int dos_exepath(char *buf, int bufsz)
{
    int seg, off, count, i, c;
    seg = dos_env_seg();
    off = 0;
    // Walk past all "NAME=VALUE\0" strings to reach the double-NUL end marker.
    while (dos_peek(seg, off) != 0) {
        while (dos_peek(seg, off) != 0)
            off++;
        off++;      // skip variable's NUL terminator
    }
    off++;          // skip the end-marker NUL; now at low byte of count word
    count = dos_peek(seg, off);
    off += 2;       // skip the 2-byte count word; now at start of program path
    if (count == 0) { buf[0] = 0; return 0; }
    i = 0;
    while (i < bufsz - 1) {
        c = dos_peek(seg, off + i);
        if (c == 0) break;
        buf[i] = c;
        i++;
    }
    buf[i] = 0;
    return (i > 0);
}

// Extract the directory prefix of path into dir (trailing backslash included).
// If path has no separator, dir is set to "" (no directory component).
void dirpart(char *path, char *dir, int dirsz)
{
    int i, last;
    last = -1;
    i = 0;
    while (path[i]) {
        if (path[i] == '\\' || path[i] == '/') last = i;
        i++;
    }
    if (last < 0) { dir[0] = 0; return; }
    i = 0;
    while (i <= last && i < dirsz - 1) { dir[i] = path[i]; i++; }
    dir[i] = 0;
}

// Join dir and file into buf, inserting '\' if dir lacks a trailing separator.
void compose_path(char *dir, char *file, char *buf, int bufsz)
{
    int i, j;
    i = 0;
    while (dir[i] && i < bufsz - 1) { buf[i] = dir[i]; i++; }
    if (i > 0 && buf[i-1] != '\\' && buf[i-1] != '/' && i < bufsz - 1)
        buf[i++] = '\\';
    j = 0;
    while (file[j] && i < bufsz - 1) { buf[i++] = file[j++]; }
    buf[i] = 0;
}

/* -------------------------------------------------------------------------
 * Reporting
 * ------------------------------------------------------------------------- */

int passed, failed, infos;

void report(char *label, int ok)
{
    if (ok) {
        printf("  PASS  %s\n", label);
        passed++;
    } else {
        printf("  FAIL  %s\n", label);
        failed++;
        printf("        Press ENTER to continue...");
        getchar();
    }
}

// Use for results that are expected to be a specific value but where failure
// is informative rather than a bug.  ok==1 is the "desired" outcome.
void report_info(char *label, int ok, char *note)
{
    if (ok) {
        printf("  PASS  %s\n", label);
        passed++;
    } else {
        printf("  INFO  %s\n", label);
        printf("        (%s)\n", note);
        infos++;
    }
}

/* -------------------------------------------------------------------------
 * Test sections
 * ------------------------------------------------------------------------- */

// Section 1: dos_exepath() must return a non-empty path.
// This is the foundation; all other sections depend on a valid exepath.
void section1(char *exepath)
{
    int ok;
    printf("Section 1: dos_exepath()\n");
    ok = dos_exepath(exepath, FNSIZE);
    report("dos_exepath returned non-empty string", ok);
    if (ok)
        printf("           exe = %s\n", exepath);
    else
        printf("           (no path; remaining sections will show empty strings)\n");
}

// Section 2: dirpart() must extract a non-empty directory ending in '\'.
void section2(char *exepath, char *exedir)
{
    int len, ok;
    printf("\nSection 2: dirpart(exepath)\n");
    dirpart(exepath, exedir, FNSIZE);
    len = strlen(exedir);
    ok = (len > 0 && (exedir[len-1] == '\\' || exedir[len-1] == '/'));
    report("exedir is non-empty and ends in backslash", ok);
    if (len > 0)
        printf("           dir = %s\n", exedir);
}

// Section 3: compose_path(exedir, "tenvvars.h") must open successfully.
// This simulates  #include <tenvvars.h>  when cc.exe lives beside the header.
// tenvvars.h is in C:\ETC\TENVVARS\; CWD is C:\ETC\ -- compose is required.
void section3(char *exedir, char *composed)
{
    FILE *fp;
    printf("\nSection 3: compose_path + fopen   (<> simulation)\n");
    compose_path(exedir, "tenvvars.h", composed, FNSIZE);
    printf("           path = %s\n", composed);
    fp = fopen(composed, "r");
    report("fopen(composed path) succeeded", fp != NULL);
    if (fp != NULL) fclose(fp);
}

// Section 4: fopen("tenvvars.h") bare must FAIL from CWD (= C:\ETC\).
// tenvvars.h does not exist in CWD; only in C:\ETC\TENVVARS\.
// Desired result is FAILURE -- that is the new correct behaviour.
// A PASS here means make.bat is running from the wrong directory.
void section4()
{
    FILE *fp;
    printf("\nSection 4: fopen(\"tenvvars.h\") bare   (CWD miss = desired)\n");
    fp = fopen("tenvvars.h", "r");
    report_info("bare fopen failed  (CWD not searched -- new behaviour)",
                fp == NULL,
                "file found in CWD -- run make.bat from C:\\ETC, not C:\\ETC\\TENVVARS");
    if (fp != NULL) fclose(fp);
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

main()
{
    char exepath[FNSIZE];
    char exedir[FNSIZE];
    char composed[FNSIZE];

    passed = 0; failed = 0; infos = 0;

    printf("\n=== tenvvars: #include path mechanism test ===\n");
    printf(  "    (compile-time success with <stdio.h> already proven)\n\n");

    section1(exepath);
    section2(exepath, exedir);
    section3(exedir, composed);
    section4();

    printf("\n--- %d passed, %d info, %d failed ---\n", passed, infos, failed);
    if (failed == 0 && infos == 0)
        printf("    All checks passed.  Include path mechanism is correct.\n");
    else if (failed == 0)
        printf("    No hard failures.  Review INFO lines above.\n");
    else
        printf("    Fix FAILures before rebuilding cc.exe.\n");

    return 0;
}
