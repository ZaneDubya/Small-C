// ============================================================================
// Small-C Compiler -- Preprocessor Module.
// Handles physical line reading, conditional compilation (#ifdef/#ifndef/
// #else/#endif), macro definition (#define), file inclusion (#include),
// macro expansion, and character delivery (inbyte).
// Copyright 1982, 1983, 1985, 1988 J. E. Hendrix.
// All rights reserved.
// ----------------------------------------------------------------------------
// Notice of Public Domain Status
// The source code for the Small-C Compiler and runtime libraries (CP/M & DOS),
// Small-Mac Assembler (CP/M), Small-Assembler (DOS), Small-Tools programs and
// Small-Windows library to which I hold copyrights are hereby available for
// royalty free use in private or commerical endeavors. The only obligation
// being that the users retain the original copyright notices and credit all
// prior authors (Ron Cain, James Hendrix, etc.) in derivative versions.
// James E. Hendrix Jr.
// ============================================================================

#include "stdio.h"
#include "cc.h"

// === preprocessor-private globals ==========================================

int
    iflevel,    // #if... nest level
    skiplevel,  // level at which #if... skipping started
    macptr,     // macro string buffer index (next write position)
    pptr;       // pline write cursor

char
    *macn,              // macro name buffer  (MACNSIZE bytes, heap-alloc'd)
    *macq,              // macro string buffer (MACQSIZE bytes, heap-alloc'd)
    *pline,             // macro-expanded line buffer (LINESIZE bytes)
    *mline,             // raw physical line buffer   (LINESIZE bytes)
    msname[NAMESIZE];   // buffer for parsing macro names

// === externals from cc1.c ==================================================

extern char
    *line,        // current line pointer (set to mline or pline by preprocess)
    *lptr,        // current character pointer within line
    *cptr,        // work pointer -- set by findSymbol, read by callers
    *cptr2,       // secondary work pointer (used by dodefine)
    infn[30],     // current input filename
    inclfn[30];   // current include filename

extern int
    ccode,        // true while parsing C code; false inside #asm blocks
    ch,           // current character of input line
    nch,          // next character of input line
    eof,          // true on final input eof
    errflag,      // true after first error in statement (used by noiferr)
    input,        // fd for input file
    input2,       // fd for #include file
    listfp,       // file pointer to list device
    lineno,       // current line number in input file
    inclno,       // current line number in include file
    output;       // fd for output file

// === initialization =========================================================

// Allocate preprocessor buffers and point the shared line pointer at the raw
// line buffer.  Must be called from main() before getRunArgs() or openfile().
void initPreprocessor() {
    macn   = calloc(MACNSIZE, 1);
    macq   = calloc(MACQSIZE, 1);
    pline  = calloc(LINESIZE, 1);
    mline  = calloc(LINESIZE, 1);
    line   = mline;   // point current-line ptr at raw-input buffer
}

// === macro management =======================================================

// Append one character to the macro string buffer.
int putmac(char c) {
    macq[macptr] = c;
    if (macptr < MACMAX)
        ++macptr;
    return c;
}

// Define a macro: parse the name and value from the current raw line
// and store them in the macro name / string buffers.
void dodefine() {
    int k;
    if (symname(msname) == 0) {
        illname();
        kill();
        return;
    }
    k = 0;
    if (findSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX) == 0) {
        if (cptr2 = cptr)
            while (*cptr2++ = msname[k++]);
        else {
            error("macro name table full");
            return;
        }
    }
    putint(macptr, cptr + NAMESIZE, 2);
    while (white()) 
        gch();
    // Collect macro value chars up to end-of-line or null.
    // Must test for LF/CR explicitly because dodefine() is called from
    // ifline() on the raw mline buffer which still carries the trailing \n.
    // In the old flow it was called on the preprocessed pline which had
    // already had newlines stripped, so gch() reaching null was sufficient.
    while (ch && ch != LF && ch != CR) {
        putmac(gch());
    }
    putmac(0);   // always null-terminate the macro value
    if (macptr >= MACMAX) {
        error("macro string queue full");
        abort(ERRCODE);
    }
}

// Open an include file and switch input2 to read from it.
void doinclude() {
    int i; char str[30];
    blanks();       // skip over to name
    if (*lptr == '"' || *lptr == '<')  {
        ++lptr;
    }
    i = 0;
    while (lptr[i] && lptr[i] != '"' && lptr[i] != '>' && lptr[i] != '\n' && lptr[i] != CR) {
        str[i] = lptr[i];
        ++i;
    }
    str[i] = NULL;
    if ((input2 = fopen(str, "r")) == NULL) {
        input2 = EOF;
        error("open failure on include file");
    }
    else {
        strcpy(inclfn, str);
        inclno = 0;
    }
    kill();   // make next read come from new file (if open)
}

// === line buffer management =================================================

// Append character c to the preprocessed line buffer (pline).
void keepch(char c) {
    if (pptr < LINEMAX) {
        pline[++pptr] = c;
    }
}

// === error helpers ==========================================================

void noiferr() {
    error("no matching #if...");
    errflag = 0;
}

// === physical line input ====================================================

// Read one physical line from the current input source into mline (via line).
// Handles include-file switching and tracks line numbers.
void doInline() {
    int k, unit;
    poll(1);           // allow operator interruption
    if (input == EOF)
        openfile();
    if (eof)
        return;
    if ((unit = input2) == EOF)
        unit = input;
    line[LINEMAX - 2] = '\n';  // sentinel: fgets overwrites only if buffer fills
    if (fgets(line, LINEMAX, unit) == NULL) {
        fclose(unit);
        if (input2 != EOF)
            input2 = EOF;
        else
            input = EOF;
        *line = NULL;
    }
    else {
        if (input2 != EOF) {
            ++inclno;
        }
        else {
            ++lineno;
        }
        // If the buffer filled without a newline the physical line was longer
        // than LINEMAX-1.  Drain the remainder so the next read starts on a
        // fresh line, then report the error exactly once.
        if (line[LINEMAX - 2] != '\n' && line[LINEMAX - 2] != '\0') {
            int c;
            while ((c = fgetc(unit)) != '\n' && c != EOF) ;
            error("line too long");
        }
        if (listfp) {
            if (listfp == output)
                fputc(';', output);
            fputs(line, listfp);
        }
    }
    bump(0);
}

// === conditional compilation ================================================

// Read physical lines, processing ALL preprocessor directives
// (#ifdef, #ifndef, #else, #endif, #define, #include) until a non-directive,
// non-skipped line is ready in mline with ch/nch/lptr set to its first char.
void ifline() {
    while (1) {
        doInline();
        if (eof) return;
        if (isMatch("#ifdef")) {
            ++iflevel;
            if (skiplevel) 
                continue;
            symname(msname);
            if (findSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX) == 0)
                skiplevel = iflevel;
            continue;
        }
        if (isMatch("#ifndef")) {
            ++iflevel;
            if (skiplevel) continue;
            symname(msname);
            if (findSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX))
                skiplevel = iflevel;
            continue;
        }
        if (isMatch("#else")) {
            if (iflevel) {
                if (skiplevel == iflevel) skiplevel = 0;
                else if (skiplevel == 0)  skiplevel = iflevel;
            }
            else noiferr();
            continue;
        }
        if (isMatch("#endif")) {
            if (iflevel) {
                if (skiplevel == iflevel) skiplevel = 0;
                --iflevel;
            }
            else noiferr();
            continue;
        }
        if (skiplevel) continue;
        if (ch == 0) continue;
        break;
    }
}

// === macro expansion ========================================================

// Preprocess the next source line: read it (via ifline or doInline),
// expand macros, strip comments.  On return, line == pline and
// ch/nch/lptr are positioned at the first character of the expanded line.
// Loops internally to consume #define and #include directives so that
// callers always receive a non-directive line (or eof).  This preserves
// the invariant that dodefine/doinclude see an already macro-expanded
// pline, matching the original architecture.
// In asm-passthrough mode (ccode == 0), just reads the raw line and returns.
void preprocess() {
    int k;
    char c;
    while (1) {
    if (ccode) {
        line = mline;
        ifline();
        if (eof) return;
    }
    else {
        doInline();
        return;
    }
    pptr = -1;
    while (ch != LF && ch != CR && ch) {
        if (white()) {
            keepch(' ');
            while (white()) gch();
        }
        else if (ch == '"') {
            keepch(ch);
            gch();
            while (ch != '"' || (*(lptr - 1) == 92 && *(lptr - 2) != 92)) {
                if (ch == NULL) {
                    error("no quote");
                    break;
                }
                keepch(gch());
            }
            gch();
            keepch('"');
        }
        else if (ch == 39) {
            keepch(39);
            gch();
            while (ch != 39 || (*(lptr - 1) == 92 && *(lptr - 2) != 92)) {
                if (ch == NULL) {
                    error("no apostrophe");
                    break;
                }
                keepch(gch());
            }
            gch();
            keepch(39);
        }
        else if (ch == '/' && nch == '*') {
            bump(2);
            while ((ch == '*' && nch == '/') == 0) {
                if (ch) bump(1);
                else {
                    ifline();
                    if (eof) break;
                }
            }
            bump(2);
        }
        // ignore C99-style comments
        else if (ch == '/' && nch == '/') {
            bump(2);
            while ((ch == LF || ch == CR) == 0) {
                if (ch) bump(1);
                else {
                    ifline();
                    if (eof) break;
                }
            }
            bump(1);
            if (ch == LF) {
                bump(1);
            }
        }
        else if (an(ch)) {
            k = 0;
            while (an(ch) && k < NAMEMAX) {
                msname[k++] = ch;
                gch();
            }
            msname[k] = NULL;
            if (findSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX)) {
                k = getint(cptr + NAMESIZE, 2);
                while (c = macq[k++]) {
                  keepch(c);
                }
                while (an(ch)) {
                  gch();
                }
            }
            else {
                k = 0;
                while (c = msname[k++]) {
                  keepch(c);
                }
                while (an(ch)) {
                  keepch(ch);
                  gch();
                }
            }
        }
        else keepch(gch());
    }
    if (pptr >= LINEMAX) error("line too long");
    keepch(NULL);
    line = pline;
    bump(0);
    // Handle #define and #include on the expanded pline before returning.
    // Guard with ch=='#': avoids calling isMatch (which calls blanks) when
    // the expanded line is empty (ch==0).  blanks() on an empty pline would
    // recurse back into preprocess(), causing one extra nested call per blank
    // or comment line -- significant overhead on large headers like cc.h.
    if (ch == '#') {
        if (isMatch("#define"))  { dodefine();  continue; }
        if (isMatch("#include")) { doinclude(); continue; }
    }
    return;
    } // while(1)
}

// === character delivery =====================================================

// Return the next preprocessed character, calling preprocess() as needed
// when the current line is exhausted (ch == 0).
int inbyte() {
    while (ch == 0) {
        if (eof) return 0;
        preprocess();
    }
    return gch();
}
