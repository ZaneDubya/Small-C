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
Version 2.2, Revision Level 117
Copyright 1982, 1983, 1985, 1988 J. E. Hendrix

Enhancements by Zane Wagner:
* Replaced Quote[] with inline string literal.
* Consolidated Type recognition in dodeclare(), dofunction(), and statement().
* C99 style comments are allowed (//)
* C89/C90 argument list types.
* Support for 'static' access modifier for functions and globals.
* Support for longer variable names.

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
