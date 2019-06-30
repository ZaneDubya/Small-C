           Small C Compiler / Small Assembler
                       User Notes


COPYRIGHT NOTICES

Compiler:  Copyright 1982, 1983, 1985, 1988 J. E. Hendrix
Library:   Copyright 1984 J. E. Hendrix, L. E. Payne
Assembler: Copyright 1988 J. E. Hendrix

All rights reserved.


COMPILER VERSION 

This diskette contains version 2.2 (revision level 117) of the Small C
compiler as implemented under DOS 2.1 (and later versions) by James E.
Hendrix.  This version differs from 2.1 primarily in the code generating
and optimizing parts.  They have been extensively rewritten, making them
much more lucid and noticably improving the quality of the code generated.
The source files have been reorganized into a more logical arrangement.  No
new features have been added.

Compared to the previous release (version 2.1, revision level 75), this 
compiler generates programs with EXE files that are between 4% and 13%
(typically 10%) smaller and twice as fast.  Although much more
optimizing is done, the compiler itself is about 6% smaller and runs
slightly faster than the previous version.  About one fourth of its time
is spent optimizing.

The main reason for the doubling of the execution speed is a revision to
the library function iscons() so that it calls DOS only for the first
refererence to a file, thereafter it returns a value based on its memory of
DOS's original answer.  It turned out that this was a very very slow DOS
call, and since iscons() is called in fgets(), fgetc(), and _read(), which
form a thread, the penalty was high.  So, not surprisingly, avoiding this
DOS call on a byte-by-byte basis yielded spectacular improvements in read
times.

Another reason for the speed improvement is a rewrite of the character
classification functions -- isalpha(), isdigit(), etc.  They used to test a
character's value with "if" statements, but now they simply reference an
array with the questionable character as a subscript and return a bit
indicating whether or not the character is of the specified class.  This
sped up these functions by three times.  Since these functions are now so
small, they have been grouped into a single source module IS.C.

Finally, there is the general improvement in speed produced by the
additional code optimizing, which goes far beyond what was being done.
While not as good as the major C compilers, the code Small C now
generates is quite respectable.  For example, AR.EXE is 1.13 times the
size of the same program compiled with Microsoft 4.0.

For those interested in studying the compiler itself, perhaps the nicest
thing about version 2.2 is the rewrite of the code generation and
optimizing logic, the reorganization of the source files, and the
revised p-code names.  These changes have made the compiler much more
understandable.  First, the p-code names now follow a system so that it
is possible to tell exactly what a code does without looking at the
assembly text it generates.  The regrouping of the source code clearly
differentiates between front end, back end, and parsing functions.  All
of the p-codes appear alphabetically in the function setcodes() together
with the assembly strings they generate, making it unnecessary to forage
around for the exact effect of a p-code.  Finally, the peephole
optimizer has been generalized to take both the p-code sequence to be
matched and the optimizing instructions from integer arrays to which it
is directed.  A single array contains both of these for a given
optimization case.  One can easily understand the optimizations by
studying these arrays -- which are initialized with p-code symbols and
other symbols that form a kind of language.  There is no need to study
the optimizer itself to read these optimization actions.

This version fixes a number of problems with the earlier version -- most
notably, a tendency of scanf() to drop characters.  Read the file
HISTORY for a complete listing of the revisions since 2.1.


ASSEMBLER VERSION

This diskette contains an executable copy of Small Assembler.  This is an
assembler written in Small C primarily for use in processing Small C
output.  The assembler runs slowly compared to Microsoft's latest
assembler.  But it is adequate for use with Small C, making it unnecessary
to purchase an expensive assembler.  Small Assembler is available as a
product in its own right -- complete with source code and a user's manual.

This assembler is essentially compatible with the Microsoft product.
Either may be used with the Small C compiler.  Differences are primarily:
(1) many Microsoft directives are not supported (most notably the
conditional assembly directives), (2) macros employ positional rather than
named parameters, (3) not all of the Microsoft expression operators are
supported, and (4) most of the expression operators in the C language are
supported.


THE DISTRIBUTION DISKETTE

The following files are included on this disk:

   READ.ME       This documentation.
   HISTORY       History of Small C rivisions.
   CCC.BAT       Procedure to compile the compiler.
   CC.EXE        Executable Small C compiler.
   CC.H          Header file for compiling Small C.
   CC1.C         Small C source (part 1).
   CC2.C         Small C source (part 2).
   CC3.C         Small C source (part 3).
   CC4.C         Small C source (part 4).
   NOTICE.H      Copyright header for compiling Small C.
   STDIO.H       Standard header file for all compiles.
   CLIB.LIB      Small C object library for use with LINK.
   CLIB.ARC      Small C library source archive.
   CLIBARC.LST   List supplied to AR for building CLIB.ARC.
   AR.EXE        Executable archive maintainer.
   AR.C          Archive maintainer source.
   ASM.EXE       Executable Small Assembler.


DOCUMENTATION

The Small C compiler is fully described in "A Small C Compiler:
Language, Usage, Theory, and Design"  by James E. Hendrix.  Copies are
available on a "Small C" CD-ROM from:

                     Dr. Dobb's CD-ROM Library
                     1601 West 23rd Street, Suite 200
                     Lawrence, KS  66046-9905
                     Phone: 1 (800) 992-0549

In this book, you will find a full treatment of the Small-C language, 
the use of the Small-C compiler, and the theory of operation of the 
compiler.  It documents revision level 112 of the compiler.  Any 
additional revisions to the compiler on this diskette are described in 
the file named HISTORY.  


USING THE COMPILER

The Small-C compiler takes in a subset of the full C language, and
generates Microsoft assembly language output.  It supports only integer
and character data types.  Arrays are limited to one dimension.  It does
not support arrays of pointers, structures, or unions.  Also lacking are
sizeof, casts, #if expr, #undef, and #line. External functions are
automatically declared, but external variables (defined in another
source file) must be declared explicitly.  Functions always return
integer values. Globals may be initialized using the = syntax, but
locals cannot be initialized. Locals are always automatic, and the
specifiers auto, static, extern, register, and typedef are not accepted
at the local level. Only extern is accepted at the global level.
Character variables are expanded with sign-extension when they are
referenced; character constants are not sign extended, however.

The steps involved in compiling a program are documented in the file
C.BAT.

Examples of invoking the compiler follow:

  CC                       compile console input
                           giving console output

  CC <FILE1 -L1 -P         compile FILE1 giving console
                           output, list the source as
                           comments in the output, and
                           pause on errors

  CC <FILE1 >FILE2 -M      compile FILE1 giving FILE2
                           and monitor progress by
                           listing function headers on
                           the console

  CC FILE1                 compile FILE1.C giving FILE1.ASM

  CC FILE1 FILE2           compile FILE1.C then FILE2.C
                           giving FILE1.ASM

  CC FILE1 FILE2 >FILE3    compile FILE1.C then FILE2.C into
                           a single program in FILE3

  CC FILE1 >FILE2 -NO -A   compile FILE1.C giving FILE2,
                           negate optimizing, and
                           sound the alarm on errors

Any number of files may be concatenated as input by listing them in the
command line; in that case stdin is not used. Standard DOS file
specifications, including logical devices, are accepted. The listing
switch has two forms:

          -L1   lists on stdout (with output) as comments
          -L2   lists on stderr (always the console)

If the compiler aborts with an exit code of 1, there is insufficient
memory to run it.  Pressing control-S makes the compiler pause until
another key is pressed, and control-C aborts the run with an exit code
of 2.  Press ENTER to resume execution after a pause because of a
compile error. If input is from the keyboard,  control-Z  indicates end-
of-file, control-X rubs out the pending line, and Backspace rubs out the
previous character.


USING THE ASSEMBLER

The Small Assembler reads an ASCII source file and writes an object file
in the standard .OBJ file format.  This makes the Small Assembler
compatible with LINK which comes with PC/MS-DOS.  In fact, Small
Assembler has no linker of its own; it must be used with the DOS linker.

The command line for invoking the assembler has the following format:

          ASM source [object] [-C] [-L] [-NM] [-P] [-S#]

Command-line arguments may be given in any order.  Switches may be
introduced by a hyphen or a slash.  The brackets indicate optional
arguments; do not enter the brackets themselves.

 source   Name of the source file to be assembled.  The default, and
          only acceptable, extension is ASM.  Drive and path may be
          specified.

 object   Name of the object file to be output.  It must have a OBJ
          extension to be recognized as the output file.  Drive and path
          may be specified.  If no object file is specified, output will
          go to a file bearing the same name and path as the source
          file, but with a OBJ extension.

 -C       Use case sensitivity.  Without this switch, all symbols are
          converted to upper-case.  With this switch, they are taken as
          is; upper- and lower-case variants of the same letter are
          considered to be different.  With or without this switch,
          segment, group, and class names are always converted to upper-
          case in the output OBJ file.

              Note: The .CASE directive may be included at the
              beginning of the source file to produce the same
              effect as this switch.

 -L       Generate an assembly listing on the standard output file.  If
          you do not want the listing to go to the screen, then provide
          a redirection specification (e.g., >prn or >abc.lst) to send
          it whereever you wish.

 -NM      No macro processing.  This speeds up the assembler somewhat.
          Macro processing is NOT needed for Small-C output files.

 -P       Pause on errors waiting for an ENTER keystroke.

 -S#      Set symbol table size to accept # symbols.  Default is 1000.

If an illegal switch is given, the assembler aborts with the message

        Usage: ASM source [object] [-C] [-L] [-NM] [-P] [-S#]

The null switch (- or /) can be used to force this message.

If the assembler aborts with an exit code of 1, there is insufficient
memory to run it.  Pressing control-S makes the assembler pause until
another key is pressed, and control-C aborts the run with an exit code
of 2.  Press ENTER to resume execution after a pause because of an
error.

Symbols (names of labels, etc.) may be up to 31 characters long.  Every
character has significance.  Symbols may contain the following
characters:

        1. alphabetic
        2. numeric
        3. the special characters _, $, ?, and @.

Labels must be terminated by a colon.  All other names which begin a
line must not have a colon.

Comments begin with a semicolon (;) and continue to the end of the line.

ASCII strings are must be delimited by quotes (") or apostrophies (').
The backslash (\) may be used to escape the delimiter when it appears
within the string.

Only integer numbers are recognized by the assembler.  Numeric constants
must begin with a numeric (decimal) digit.  The default base (radix) for
numeric constants is decimal.  Bases may be specified explicitely by
attaching a letter to the right end of the number as follows:

             b or B    binary
             o or O    octal
             q or Q    octal
             d or D    decimal
             h or H    hexadecimal

Expressions may produce the Boolean values 1 for "true" or 0 for
"false".  This differs from Microsoft which yields FFFFh for "true".

Expression evaluation is done with 32-bit precision.  The low order bits
are then used according to the needs of the instruciton being generated.
Expressions may contain the following operators:

        (highest precedence)
        -------------------------------------  unary operators
        BYTE  PTR      take following operand as a byte
        WORD  PTR      take following operand as a word (2 bytes)
        DWORD PTR      take following operand as a double word (4 bytes)
        FWORD PTR      take following operand as a far word (6 bytes)
        QWORD PTR      take following operand as a quad word (8 bytes)
        NEAR  PTR      consider the following label near
        FAR   PTR      consider the following label far
        OFFSET         the offset to the follownig operand
        SEG            the segment address of the following operand
        xS:            segment register override (e.g. ES:)
        [  ]           indirect memory reference
        !              Logical not
        -              minus
        ~              one's complement
        NOT            one's complement
        -------------------------------------
        *              multiplication
        /              division
        %              modulo
        MOD            modulo
        -------------------------------------
        .              addition of indirect reference displacement
        +              addition
        -              subtraction
        -------------------------------------
        <<             shift left
        >>             shift right
        -------------------------------------
        !=             not equal
        NE             not equal
        <=             less than or equal
        LE             less than or equal
        <              less than
        LT             less than
        >=             greater than or equal
        GE             greater than or equal
        >              greater than
        GT             greater than
        ==             equal
        EQ             equal
        ------------------------------------- 
        &              bitwise AND
        AND            bitwise AND
        ------------------------------------- 
        ^              bitwise exclusive OR
        XOR            bitwise exclusive OR
        ------------------------------------- 
        |              bitwise inclusive OR
        OR             bitwise inclusive OR
        -------------------------------------
        (lowest precedence)

Notice that these precedence levels agree with the C language and 
disagree with the Microsoft assembler.  This should only rarely be a 
problem, however, since most expressions are very simple.  Nevertheless, 
parentheses may be used to override the default precedences.  

The dollar sign ($) represents the value of the location counter at the 
beginning of the current instruction.  

This version of the assembler knows only the instruction set of the 8086 
processor.  It does not know the 8087 instructions.  Future editions 
will know all of the 80x86 processors and their coprocessors.  

The following directives are recognized by this verson of the assembler.  
Only the parameter values shown are supported, however.  

     -------------------------------------------------
            .CASE          (see the -C switch above)
     -------------------------------------------------
     name   EQU constantexpression
     alias  EQU symbol
     -------------------------------------------------
     name   =  constantexpression
     -------------------------------------------------
     name   MACRO
            ...
            ENDM
     -------------------------------------------------
     name   SEGMENT  [align]  [combine]  [class]
                      BYTE     PUBLIC     'name'
                      WORD     STACK
                      PARA     COMMON
                      PAGE
            ...
            ENDS
     -------------------------------------------------
     name   GROUP segmentname,,,
     -------------------------------------------------
     name   PROC [distance]
                  NEAR
                  FAR
            ...
            ENDP
     -------------------------------------------------
            ASSUME NOTHING
            ASSUME register:name,,,
                   CS       segmentname
                   SS       groupname
                   DS       NOTHING
                   ES
                   FS
                   GS
     -------------------------------------------------
            PUBLIC name,,,
     -------------------------------------------------
            EXTRN name:type,,,
                       BYTE
                       WORD
                       DWORD
                       FWORD
                       QWORD
                       NEAR
                       FAR
     -------------------------------------------------
     [name] DB expression,,,
     [name] DW expression,,,
     [name] DD expression,,,
     [name] DF expression,,,
     [name] DQ expression,,,
     -------------------------------------------------
            ORG expression
     -------------------------------------------------
            END [start]
     -------------------------------------------------
     name   MACRO
     -------------------------------------------------
            ENDM
     -------------------------------------------------

Macro definitions begin with the MACRO directive and end with the ENDM 
directive as shown above.  Macro definitions cannot be nested; however, 
macro expansions (or calls) can.  Since macro arguments are not named, 
none appear with the MACRO directive.  Within the body of the macro, the 
symbol ?1 indicates places where the first argument of the macro call is to 
go.  Likewise, ?2 designates locations where the second argument goes, 
and so forth through ?0 for the 10th argument.  Macro argument 
substitution is done without regard to context, so you are not limited 
to replacing whole symbols with an argument.  For instance, you might 
write J?0 for the mnemonic of a conditional jump.  The first argument of 
the call might be LE, resulting in a mnemonic of JLE.  

To avoid "Redundant Definition" errors when a macro containing labels is 
called more than once, you may write labels within the body of a macro 
as @n, where n is a decimal digit.  This allows ten such labels per 
macro.  The assembler maintains a running count of such labels as 
assembly progresses.  Whenever such a label is found in a macro 
expansion, it is replaced by a label of the form @x, where x is the 
running count.  

Small Assembler makes no distinction between warnings and hard errors.  
If it issues any error messages in a run, then it finishes the run with 
an exit code of 10 which can be tested by means of an IF statement in a 
batch file.  In that case, it also deletes the OBJ file so that you 
cannot attempt to link and execute an erroneous program.  The following 
error messages may be issued by the assembler: 

   Per Run

   - No Source File        no input file is specified or it doesn't exist
   - Error in Object File  an error occurred wirting the OBJ file
   - Missing ENDM          the program ended within a macro definition
   - Missing END           the program ended without an END directive
   - Deleted Object File   the OBJ file was deleted because of errors

   Per Segment

   - CS Not Assumed for    
     this Segment          there is no ASSUME for CS and this segment

   Per Line

   - Phase Error           the address of a label differs between passes
   - Bad Expression        an expression is written improperly
   - Invalid Instruction   this mnemonic/operand combination is not defined
   - Redundant Definition  the same symbol is defined more than once
   - Bad Symbol            a symbol is improperly formed
   - Relocation Error      an relocatable address is written as absolute
   - Undefined Symbol      an undefined symbol is referenced
   - Bad Parameter         a macro call does not give a required parameter
   - Range Error           a self-relative reference is too distant
   - Syntax Error          an instruction or directive is improperly formed
   - Not Addressable       "END xxx" has something other than an address
   - Segment Error         a segment is not defined or referenced properly
   - Procedure Error       a procedure is not defined properly


USING THE ARCHIVE MAINTAINER

Comments in the front of AR.C describe the operation of the archive 
maintainer AR.EXE.  To extract all of the modules out of the library 
archive, enter the command: 

                          AR -X CLIB.ARC
