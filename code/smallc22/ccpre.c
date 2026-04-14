// ============================================================================
// Small-C Compiler -- Preprocessor Module.
// Handles physical line reading, conditional compilation (#ifdef/#ifndef/
// #else/#endif/#error/#undef/#line), macro definition (#define), file inclusion
// (#include, up to MAXINCLUDE=4 deep),
// macro expansion, predefined macro expansion (__FILE__/__LINE__/__DATE__/__TIME__),
// and character delivery (inbyte).
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
// ----------------------------------------------------------------------------
// Pipeline overview (outermost to innermost):
//
//   inbyte()           -- delivers one expanded character to the parser.
//     preprocess()     -- reads and expands one source line into pline.
//       ifline()       -- reads raw lines honoring #ifdef/#endif/#error/#undef state.
//         doInline()   -- reads one physical line; handles include switching.
//       scanLiteral()  -- passes string/char literals through unexpanded.
//       scanComment()  -- strips /* */ block and // line comments.
//   [macro expand] -- replaces #define tokens inline with their values;
//                     predefined macros (__FILE__ etc.) are checked first.
//       dodefine()     -- processes #define directives on the expanded line.
//       doinclude()    -- processes #include directives on the expanded line.
//
//   blanks() [cc2.c]   -- skips whitespace; calls preprocess() as needed.
// ============================================================================

#include "stdio.h"
#include "cc.h"

// === preprocessor-private globals ==========================================

int
    inclno,     // current include file line number
    iflevel,    // #if... nest level
    skiplevel,  // level at which #if... skipping started
    macptr,     // macro string buffer index (next write position)
    pptr,       // pline write cursor
    incldepth,  // current #include nesting depth (0 = not in any include)
    inclnostk[MAXINCLUDE],          // saved line numbers for outer include levels
    inclfds[MAXINCLUDE];            // saved file descriptors for outer levels
                                    // Must be int: stores input2 which uses EOF==-1
                                    // as sentinel; a char round-trip zero-extends
                                    // 0xFF to 0x00FF != EOF, breaking the test.

char
    input2 = EOF,       // include-file descriptor; EOF (-1) means "none open".
                        // Valid DOS fd values are small positive integers that
                        // fit in a signed char, so -1 is an unambiguous sentinel.
    *macn,              // macro name buffer  (MACNSIZE bytes, heap-alloc'd)
    *macq,              // macro string buffer (MACQSIZE bytes, heap-alloc'd)
    *pline,             // macro-expanded line buffer (LINESIZE bytes)
    *mline,             // raw physical line buffer   (LINESIZE bytes)
    msname[NAMESIZE],   // buffer for parsing macro names
    inclfn[30],         // current include filename
    predef_date[14],    // __DATE__ token: "\"Mmm DD YYYY\"" (13 chars + NUL)
    predef_time[11];    // __TIME__ token: "\"HH:MM:SS\""   (10 chars + NUL)

// Flat filename stack: MAXINCLUDE slots of 30 bytes each.
// Declared separately to avoid multi-dim array in comma list (Small-C codegen
// does not reliably compute row addresses for char arr[M][N] in that context).
char inclstk[120];                  // saved filenames for outer include levels

// Month abbreviation table: 3 chars per month
char monnam[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

// === externals from cc1.c ==================================================

extern char
    *line,        // current line pointer (set to mline or pline by preprocess)
    *lptr,        // current character pointer within line
    *cptr,        // work pointer -- set by findSymbol, read by callers
    *cptr2,       // secondary work pointer (used by dodefine)
    infn[30];     // current input filename

extern int
    ccode,        // true while parsing C code; false inside #asm blocks
    ch,           // current character of input line
    nch,          // next character of input line
    eof,          // true on final input eof
    errflag,      // true after first error in statement (used by noiferr)
    input,        // fd for input file
    listfp,       // file pointer to list device
    lineno,       // current line number in input file
    output;       // fd for output file

// === functions used from cc2.c (no declarations; resolved at link time) =======
//   an(c)              -- true if c is alphanumeric or underscore
//   alpha(c)           -- true if c is alphabetic or underscore
//   astreq(s1,s2,len)  -- compare up to len significant chars; returns 0 on mismatch
//                         (used by tryExpandPredefined for exact identifier matching)
//   blanks()           -- skip whitespace; calls preprocess() when line empties
//   bump(n)            -- advance lptr by n; or reset to line when n == 0
//   findSymbol(...)    -- hash-table lookup in a named symbol/macro buffer
//                         (tombstone-aware: skips deleted slots, reclaims them
//                          for new insertions -- see TOMBSTONE in cc.h)
//   gch()              -- return ch and advance lptr by one character
//   getint(addr, len)  -- read a little-endian integer of len bytes
//   illname()          -- report "illegal symbol" and skip to the next token
//   isMatch(lit)       -- skip blanks, then match and consume a literal string
//   kill()             -- zero line[0] and call bump(0), discarding the line
//   putint(i, addr, n) -- write integer i as n little-endian bytes at addr
//   symname(buf)       -- skip blanks and read an identifier into buf
//   white()            -- true if the character at lptr is a whitespace byte

// === initialization =========================================================

// Allocate preprocessor buffers and point the shared line pointer at the raw
// line buffer. Must be called from main() before getRunArgs() or openfile().
void initPreprocessor() {
    macn   = calloc(MACNSIZE, 1);
    macq   = calloc(MACQSIZE, 1);
    pline  = calloc(LINESIZE, 1);
    mline  = calloc(LINESIZE, 1);
    line   = mline;   // point current-line ptr at raw-input buffer
    initPredefined();
}

// === build-time constant helpers ============================================

// Write exactly 2 decimal digits (zero-padded) to buf[0] and buf[1].
// No NUL is written. buf must be at least 2 bytes.
// Used by initPredefined() to format date and time fields.
void twoDigit(int n, char *buf) {
    buf[0] = '0' + n / 10;
    buf[1] = '0' + n % 10;
}

// Convert non-negative int n to a NUL-terminated decimal string in buf.
// buf must be at least 6 bytes (max signed int 32767 = 5 digits + NUL).
// Used by tryExpandPredefined() for __LINE__ expansion.
void lineToStr(int n, char *buf) {
    int i;
    int tmp;
    i = 0;
    tmp = n;
    // Count digits.
    if (tmp == 0) {
        buf[i++] = '0';
    }
    else {
        while (tmp > 0) {
            buf[i++] = '0' + tmp % 10;
            tmp = tmp / 10;
        }
        // Reverse digits in place.
        {
            int lo;
            int hi;
            char t;
            lo = 0;
            hi = i - 1;
            while (lo < hi) {
                t = buf[lo];
                buf[lo] = buf[hi];
                buf[hi] = t;
                ++lo;
                --hi;
            }
        }
    }
    buf[i] = NULL;
}

// === DOS date/time helper ==================================================
// Fill a 6-element int array from two INT 21h calls.
// Called once by initPredefined() at startup.
// dt[0]=year, dt[1]=month(1-12), dt[2]=day(1-31),
// dt[3]=hour(0-23), dt[4]=min(0-59), dt[5]=sec(0-59).
// DOS GetDate (fn 2Ah): year -> CX, month -> DH, day -> DL.
// DOS GetTime (fn 2Ch): hour -> CH, min -> CL, sec -> DH.
// dt is the first argument, at 4[BP] (standard Small-C near-call frame:
// saved BP at [BP+0], return address at [BP+2], first arg at [BP+4]).
void getDosDateTime(int *dt) {
#asm
    mov  bx,4[bp]     ; bx = dt
    mov  ah,2Ah       ; GetDate
    int  21h
    mov  [bx],cx      ; dt[0] = year (full 16-bit)
    xor  ah,ah
    mov  al,dh        ; month
    mov  2[bx],ax     ; dt[1] = month
    xor  ah,ah
    mov  al,dl        ; day
    mov  4[bx],ax     ; dt[2] = day
    mov  ah,2Ch       ; GetTime
    int  21h
    xor  ah,ah
    mov  al,ch        ; hour
    mov  6[bx],ax     ; dt[3] = hour
    xor  ah,ah
    mov  al,cl        ; min
    mov  8[bx],ax     ; dt[4] = min
    xor  ah,ah
    mov  al,dh        ; sec
    mov  10[bx],ax    ; dt[5] = sec
#endasm
}

// Build predef_date and predef_time from the current DOS clock.
// Called once at the end of initPreprocessor().
// Both strings include surrounding double-quote characters so that
// tryExpandPredefined() can emit them directly into the pline token stream;
// the C parser then sees a string literal token.
void initPredefined() {
    int dt[6];
    char ybuf[6];
    getDosDateTime(dt);
    // dt[0]=year, dt[1]=month, dt[2]=day, dt[3]=hour, dt[4]=min, dt[5]=sec.
    // Build predef_date: "\"Mmm DD YYYY\""
    // Layout: [0]='"' [1..3]=month [4]=' ' [5..6]=day [7]=' ' [8..11]=year [12]='"' [13]=NUL
    predef_date[ 0] = '"';
    predef_date[ 1] = monnam[(dt[1] - 1) * 3    ];
    predef_date[ 2] = monnam[(dt[1] - 1) * 3 + 1];
    predef_date[ 3] = monnam[(dt[1] - 1) * 3 + 2];
    predef_date[ 4] = ' ';
    predef_date[ 5] = (dt[2] < 10) ? ' ' : '0' + dt[2] / 10;
    predef_date[ 6] = '0' + dt[2] % 10;
    predef_date[ 7] = ' ';
    lineToStr(dt[0], ybuf);   // year, e.g. "2026"
    predef_date[ 8] = ybuf[0];
    predef_date[ 9] = ybuf[1];
    predef_date[10] = ybuf[2];
    predef_date[11] = ybuf[3];
    predef_date[12] = '"';
    predef_date[13] = NULL;
    // Build predef_time: "\"HH:MM:SS\""
    // Layout: [0]='"' [1..2]=hour [3]=':' [4..5]=min [6]=':' [7..8]=sec [9]='"' [10]=NUL
    predef_time[0] = '"';
    twoDigit(dt[3], &predef_time[1]);
    predef_time[3] = ':';
    twoDigit(dt[4], &predef_time[4]);
    predef_time[6] = ':';
    twoDigit(dt[5], &predef_time[7]);
    predef_time[9]  = '"';
    predef_time[10] = NULL;
}



// === macro management =======================================================

// Wrapper for the repeated 7-argument macro-table lookup.
// Sets cptr to the found/insertion slot; returns 1 on hit, 0 on miss.
int findMacro() {
    return findSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX);
}

// Append one character to the macro string buffer.
int putmac(char c) {
    macq[macptr] = c;
    if (macptr < MACMAX) {
        ++macptr;
    }
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
    if (findMacro() == 0) {
        if (cptr2 = cptr) {
            k = 0;
            while (*cptr2++ = msname[k++]) {
                // copy the macro name into the name buffer; cptr2 advances
            }
        }
        else {
            error("macro name table full");
            return;
        }
    }
    // Each name-table slot is NAMESIZE+2 bytes: NAMESIZE bytes for the null-
    // terminated name, then 2 little-endian bytes holding the offset into macq
    // where the macro's replacement-text string starts.
    putint(macptr, cptr + NAMESIZE, 2);
    while (white()) {
        gch();
    }
    // Collect macro value chars up to end-of-line or null.
    // Must test for LF/CR explicitly because dodefine() is called from
    // ifline() on the raw mline buffer which still carries the trailing \n.
    // In the old flow it was called on the preprocessed pline which had
    // already had newlines stripped, so gch() reaching null was sufficient.
    while (ch && ch != LF && ch != CR) {
        putmac(gch());
    }
    putmac(0);   // always null-terminate the macro value
    // putmac() guards the write so no byte is lost; this check fires as a
    // fatal diagnostic rather than a memory overwrite.
    // Use exit() not abort(): abort() in this runtime is a bare jmp into
    // exit() past the argument copy and does not reliably convey the exit code.
    if (macptr >= MACMAX) {
        error("macro string queue full");
        exit(ERRCODE);
    }
}

// Open an include file and push the current include state onto the stack.
// Nesting is limited to MAXINCLUDE levels; a deeper attempt is an error.
void doinclude() {
    int i;
    char str[30];
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
    if (incldepth >= MAXINCLUDE) {
        error("includes nested too deep");
        kill();
        return;
    }
    // Push the current include state before switching to the new file.
    // At depth 0, input2==EOF and inclfn/inclno are irrelevant but saved
    // anyway so the pop is uniform at all levels.
    inclfds[incldepth]  = input2;
    inclnostk[incldepth] = inclno;
    strcpy(inclstk + incldepth * 30, inclfn);
    if ((input2 = fopen(str, "r")) == NULL) {
        input2 = inclfds[incldepth];   // restore before reporting error
        error("open failure on include file");
    }
    else {
        ++incldepth;
        strcpy(inclfn, str);
        inclno = 0;
    }
    kill();   // make next read come from new file (if open)
}

// === line buffer management =================================================

// Append character c to the preprocessed line buffer (pline).
// pptr is initialised to -1 at the top of preprocess(), so the first call
// arrives with pptr == -1 and the pre-increment places c at pline[0].
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
    int unit;
    poll(1);           // allow operator interruption
    if (input == EOF) {
        openfile();
    }
    if (eof) {
        return;
    }
    if ((unit = input2) == EOF) {
        unit = input;
    }
    line[LINELAST] = '\n';  // sentinel: fgets overwrites only if buffer fills
    if (fgets(line, LINEMAX, unit) == NULL) {
        fclose(unit);
        if (input2 != EOF) {
            // End of an include file: pop the include stack.
            --incldepth;
            input2 = inclfds[incldepth];
            inclno = inclnostk[incldepth];
            strcpy(inclfn, inclstk + incldepth * 30);
        }
        else {
            input = EOF;
        }
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
        // than LINEMAX-1. Drain the remainder so the next read starts on a
        // fresh line, then report the error exactly once.
        if (line[LINELAST] != '\n' && line[LINELAST] != '\0') {
            int c;
            while ((c = fgetc(unit)) != '\n' && c != EOF) {
                // drain the rest of the line
            }
            error("line too long");
        }
        if (listfp) {
            if (listfp == output) {
                fputc(';', output);
            }
            fputs(line, listfp);
        }
    }
    bump(0);
}

// === conditional compilation ================================================

// Emit a user-defined compile error from a #error directive.
// Reads the remainder of the current line as the message text.
// Called only when skiplevel == 0 (the directive is not inside a skipped block).
// Always terminates compilation after reporting -- #error is a hard stop.
// Uses exit() rather than abort(): abort() in this runtime is a bare jmp into
// exit() past the argument copy, so it does not reliably convey the exit code.
void doerror() {
    char msg[LINESIZE];
    char *p;
    int i;
    // Strip the trailing CR/LF that fgets left in mline.  error() prints
    // the line via lout() which appends its own newline; without this strip
    // lout() produces a blank line between the source echo and the /\ caret.
    p = line;
    while (*p && *p != LF && *p != CR) {
        ++p;
    }
    *p = NULL;
    while (white()) {
        gch();
    }
    i = 0;
    while (ch && ch != LF && ch != CR && i < LINESIZE - 1) {
        msg[i++] = gch();
    }
    msg[i] = NULL;
    if (i == 0) {
        error("#error");
    }
    else {
        error(msg);
    }
    exit(ERRCODE);
}

// Remove a macro definition via #undef.
// Marks the slot with TOMBSTONE so the open-addressing probe chain is
// preserved: findSymbol will skip the slot when probing for other macros
// and will hand it back as the insertion point for a later #define
// (tombstone reclamation).  The macq replacement-text bytes are not freed:
// macq is an append-only buffer (macptr is a monotone write cursor) with no
// free list; compacting freed ranges would require rewriting the 2-byte offset
// field in every live macn slot that points past the reclaimed block.
// Silently succeeds when the macro is not defined -- C89/90 permits #undef
// of a name that was never defined.
void doundef() {
    if (symname(msname) == 0) {
        illname();
        kill();
        return;
    }
    if (findMacro()) {
        *cptr = TOMBSTONE;
    }
}

// Override the current line number (and optionally filename) via #line.
// Syntax:  #line <decimal>  |  #line <decimal> "<filename>"
// Sets lineno (or inclno when inside an #include) to N-1 so that the next
// doInline() increment lands on N.  The optional filename replaces infn or
// inclfn and takes effect immediately for __FILE__ expansion.
void doline() {
    int n;
    int i;
    // Parse the required decimal number.
    while (white()) {
        gch();
    }
    if (ch < '0' || ch > '9') {
        error("#line: line number expected");
        kill();
        return;
    }
    n = 0;
    while (ch >= '0' && ch <= '9') {
        n = n * 10 + (gch() - '0');
    }
    // Store N-1: doInline() will increment before the next line is parsed.
    if (input2 != EOF) {
        inclno = n - 1;
    }
    else {
        lineno = n - 1;
    }
    // Parse the optional "<filename>".
    while (white()) {
        gch();
    }
    if (ch == '"') {
        gch();   // consume opening quote
        i = 0;
        while (ch && ch != '"' && ch != LF && ch != CR && i < 29) {
            msname[i++] = gch();
        }
        msname[i] = NULL;
        if (ch == '"') {
            gch();   // consume closing quote
        }
        if (input2 != EOF) {
            strcpy(inclfn, msname);
        }
        else {
            strcpy(infn, msname);
        }
    }
}

// Read physical lines, processing ALL preprocessor directives
// (#ifdef, #ifndef, #else, #endif, #error, #undef, #define, #include) until a
// non-directive, non-skipped line is ready in mline with ch/nch/lptr set
// to its first char.
void ifline() {
    // State machine invariant:
    //   iflevel   -- nesting depth; incremented by #ifdef/#ifndef, decremented by #endif.
    //   skiplevel -- the iflevel at which skipping began, or 0 when not skipping.
    //   skiplevel == 0          -> not skipping; emit lines normally.
    //   skiplevel == iflevel    -> this level opened the skip; #else/#endif can close it.
    //   skiplevel  < iflevel    -> skipping from an outer level; count but don't act.
    while (1) {
        doInline();
        if (eof) {
            return;
        }
        if (isMatch("#ifdef")) {
            ++iflevel;
            if (skiplevel) {
                continue;
            }
            symname(msname);
            if (findMacro() == 0) {
                skiplevel = iflevel;
            }
            continue;
        }
        if (isMatch("#ifndef")) {
            ++iflevel;
            if (skiplevel) {
                continue;
            }
            symname(msname);
            if (findMacro()) {
                skiplevel = iflevel;
            }
            continue;
        }
        if (isMatch("#else")) {
            if (iflevel) {
                if (skiplevel == iflevel) {
                    skiplevel = 0;
                }
                else if (skiplevel == 0) {
                    skiplevel = iflevel;
                }
            }
            else {
                noiferr();
            }
            continue;
        }
        if (isMatch("#endif")) {
            if (iflevel) {
                if (skiplevel == iflevel) {
                    skiplevel = 0;
                }
                --iflevel;
            }
            else {
                noiferr();
            }
            continue;
        }
        if (isMatch("#error")) {
            // Only fire when not inside a skipped conditional block.
            if (!skiplevel) {
                doerror();
            }
            continue;
        }
        if (isMatch("#undef")) {
            if (!skiplevel) {
                doundef();
            }
            continue;
        }
        if (isMatch("#line")) {
            if (!skiplevel) {
                doline();
            }
            continue;
        }
        if (skiplevel) {
            continue;
        }
        if (ch == 0) {
            continue;
        }
        break;
    }
}

// === preprocess helpers =====================================================

// Scan a string or character literal delimited by delim ('"' or '\''),
// emitting all characters including both delimiters into pline via keepch().
// Handles backslash-escaped occurrences of the delimiter.
void scanLiteral(char delim) {
    keepch(delim);
    gch();
    while (ch != delim || (*(lptr - 1) == '\\' && *(lptr - 2) != '\\')) {
        if (ch == NULL) {
            error(delim == '"' ? "no quote" : "no apostrophe");
            break;
        }
        keepch(gch());
    }
    gch();
    keepch(delim);
}

// Consume a comment: C-style (/* ... */) or C99-style (// ...).
// On entry ch == '/' and nch reveals the comment style.
// For /* comments, reads across line boundaries via ifline().
// For // comments, consumes to end of the current line only.
void scanComment() {
    if (nch == '*') {
        bump(2);
        while ((ch == '*' && nch == '/') == 0) {
            if (ch) {
                bump(1);
            }
            else {
                ifline();
                if (eof) {
                    break;
                }
            }
        }
        bump(2);
    }
    else {   // nch == '/'
        bump(2);
        while (ch && ch != LF && ch != CR) {
            bump(1);
        }
        bump(1);
        if (ch == LF) {
            bump(1);
        }
    }
}

// Emit every character of NUL-terminated string s into pline via keepch().
void keepStr(char *s) {
    while (*s) {
        keepch(*s++);
    }
}

// Check whether name is one of the four C89 predefined macro identifiers
// supported by this compiler (__FILE__, __LINE__, __DATE__, __TIME__).
// If so, emit the expanded token text into pline via keepch() and return 1.
// If not, return 0 (caller falls through to user-defined macro lookup).
// __STDC__ is intentionally NOT defined: it signals a fully conforming ANSI C
// implementation, which Small-C is not (subset language, no float, etc.).
// Fast pre-filter: only names beginning with __ can match.
// Uses astreq(name, literal, NAMEMAX) for exact identifier matching --
// astreq returns 0 if an alphanumeric follows, so __LINE__X does not match.
int tryExpandPredefined(char *name) {
    char buf[6];
    int lineval;
    if (name[0] != '_' || name[1] != '_') {
        return 0;
    }
    if (astreq(name, "__FILE__", NAMEMAX)) {
        // Emit as a string literal including quotes.
        // Use inclfn when inside an #include file, infn otherwise.
        keepch('"');
        keepStr(input2 != EOF ? inclfn : infn);
        keepch('"');
        return 1;
    }
    if (astreq(name, "__LINE__", NAMEMAX)) {
        // Use inclno when inside an #include file, lineno otherwise.
        lineval = (input2 != EOF) ? inclno : lineno;
        lineToStr(lineval, buf);
        keepStr(buf);
        return 1;
    }
    if (astreq(name, "__DATE__", NAMEMAX)) {
        keepStr(predef_date);
        return 1;
    }
    if (astreq(name, "__TIME__", NAMEMAX)) {
        keepStr(predef_time);
        return 1;
    }
    return 0;
}

// === macro expansion ========================================================

// Preprocess the next source line: read it (via ifline or doInline),
// expand macros, strip comments. On return, line == pline and
// ch/nch/lptr are positioned at the first character of the expanded line.
// Loops internally to consume #define and #include directives so that
// callers always receive a non-directive line (or eof). This preserves
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
            if (eof) {
                return;
            }
        }
        else {
            doInline();
            return;
        }
        pptr = -1;
        while (ch != LF && ch != CR && ch) {
            // Collapse any run of whitespace to a single space,
            // normalising token spacing in the expanded output.
            if (white()) {
                keepch(' ');
                while (white()) {
                    gch();
                }
            }
            else if (ch == '"') {
                scanLiteral('"');
            }
            else if (ch == '\'') {
                scanLiteral('\'');
            }
            // Strip both /* block comments and // line comments.
            else if (ch == '/' && (nch == '*' || nch == '/')) {
                scanComment();
            }
            else if (an(ch)) {
                k = 0;
                while (an(ch) && k < NAMEMAX) {
                    msname[k++] = ch;
                    gch();
                }
                msname[k] = NULL;
                // Check predefined macros before the user-defined table.
                // The __ prefix filter makes this a near-zero cost for most tokens.
                if (msname[0] == '_' && msname[1] == '_'
                        && tryExpandPredefined(msname)) {
                    // predefined expanded; continue scanning the line
                }
                else if (findMacro()) {
                    k = getint(cptr + NAMESIZE, 2);
                    while (c = macq[k++]) {
                        keepch(c);
                    }
                    // Drain any extra alphanumerics that were truncated when
                    // the token's length hit NAMEMAX: the accumulation loop
                    // above stops at NAMEMAX chars, so ch may still be
                    // alphanumeric.  They are not part of the matched token
                    // and are silently dropped.
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
            else {
                keepch(gch());
            }
        }
        if (pptr >= LINEMAX) {
            error("line too long");
        }
        keepch(NULL);
        line = pline;
        bump(0);
        // Handle #define and #include on the expanded pline before returning.
        // Guard with ch=='#': avoids calling isMatch (which calls blanks) when
        // the expanded line is empty (ch==0). blanks() on an empty pline would
        // recurse back into preprocess(), causing one extra nested call per blank
        // or comment line -- significant overhead on large headers like cc.h.
        if (ch == '#') {
            if (isMatch("#define"))  { 
                dodefine();
                continue;
            }
            if (isMatch("#include")) {
                doinclude();
                continue;
            }
        }
        return;
    }
}

// === character delivery =====================================================

// Return the next preprocessed character, calling preprocess() as needed
// when the current line is exhausted (ch == 0).
int inbyte() {
    while (ch == 0) {
        if (eof) {
            return 0;
        }
        preprocess();
    }
    return gch();
}
