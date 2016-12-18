# SmallC
Working copy of Small-C Compiler for 16-bit MS-DOS.
Based on Small C, Version 2.2, Revision Level 117
Copyright 1982, 1983, 1985, 1988 J. E. Hendrix

Delta from Small-C Version 2.2:
* Replaced Quote[] with inline string literal.
* Consolidated Type recognition in dodeclare(), dofunction(), and statement().
* C99 style comments are allowed (//)
* C89/C90 argument list types.

My eventual goal is to port the compiler and assembler to the YCPU architecture,
write a new linker to create exes for that architecture, and then port Unix V6.
A lofty goal, but hey, that's what hobbies are for, right?