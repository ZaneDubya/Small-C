# Small-C and YLink
This is a full development environment with source code for 16-bit MS-DOS.
The repository includes source code and compiled executables for the
Small-C compiler, Small-Assembler, Small-C and Small-Assembler standard
libraries, and YLink, which links assembled object files into MS-DOS
executables.

The Small-C Compiler, Assembler, and Libraries were written by Jim Hendrix,
building on on initial draft of the compiler by Ron Cain. These works were
initially published in Dr. Dobbs Journal in the early 1980s. Jim Hendrix's
website is [deturbulator.org](http://www.deturbulator.org/Jim.asp).

YLink was written by Zane Wagner in 2017.

## Small-C Compiler
Version 2.5
Copyright 1982, 1983, 1985, 1988 J. E. Hendrix

Enhancements from Small-C Version 2.2 Revision 117:
* 32-bit long and unsigned long integer types.
* C90 function prototypes: typed parameter lists, f(void), and variadic f(...).
* cdecl (right-to-left) calling convention: first argument always at [BP+4], which may be compatible with standard C ABI.
* Structs and unions, including nested declarations, pointer-to-struct, and bitfield members.
* C90 enum support: named integer constants with optional explicit values.
* typedef declarations.
* Arbitrary pointer depth: int **, char ***, etc.
* Multi-dimensional arrays of ints, chars, longs, and structs, with initialization.
* Pointer arrays (char *arr[]) with initialization.
* Void pointers (void *): pointer arithmetic and dereferencing behave as char *.
* Static local variables: function-scoped names with permanent (data-segment) lifetime.
* Local variable initialization, including structs and arrays.
* C90 type qualifiers: const (compiler-enforced), signed, short.
* Adjacent string literal concatenation.
* C99 style // comments.
* C90 argument list types.
* Longer variable names (8 > 12 characters).
* Reserved keyword checks.
* Preprocessor: nested #include (up to 4 levels), #undef, #error, and #line.
* Predefined macros: __FILE__, __LINE__, __DATE__, __TIME__.
* Optimizer improvements: per-function second pass with short-branch substitution, frame pointer omission, epilogue consolidation, boolean-boxing elimination, and constant-shift folding, many new peephole optimization rules; output 8086 assembly is 23% smaller than Revision 117.

## Small-Assembler
Version 1.2, Revision Level 14
Copyright 1984 J. E. Hendrix, L. E. Payne

## Small-C and Small-Assembler Libraries
Copyright 1988 J. E. Hendrix

## YLink
Copyright 2017 Zane Wagner

## Notice of Public Domain Status
The source code for the Small-C Compiler and runtime libraries (CP/M & DOS),
Small-Mac Assembler (CP/M), Small-Assembler (DOS), Small-Tools programs and
Small-Windows library to which I hold copyrights are hereby available for
royalty free use in private or commerical endeavors. The only obligation being
that the users retain the original copyright notices and credit all prior
authors (Ron Cain, James Hendrix, etc.) in derivative versions.

- James E. Hendrix Jr.

The source code for the YLink object file linker is hereby available for
royalty free use in private or commerical endeavors. The only obligation being
that the users retain the original copyright notices and credit all prior
authors in derivative versions.
- Zane Wagner
