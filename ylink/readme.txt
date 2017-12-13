YLINK is meant to be a replacement linker that can be used to create executable
files. It is the next step in porting SmallC to the YCPU platform.

Expected milestones are:
    0.1     Read in simple "HELLO.OBJ" file.
            Display contents.
    0.2     Read in complex "CC.OBJ" file.
            Display contents.
    0.3     Read in library "LIBC.LIB" file.
            Display contents in LINK.TXT
    0.4     Dry run of linking: Calculate all necessary offsets, etc required to
            link and output HELLO.EXE from HELLO.OBJ and CLIBC.LIB.
            Write up process to be followed by ylink.
    0.5     Able to link and output HELLO.EXE, which works. DONE.
    0.6     Same as 0.5, but now it is (as close as I can manage) an exact copy
            of HELLO.EXE as linked by Microsoft Overlay Linker (link.exe).
            This means that I will only include library modules when they are
            necessary to resolve a missing extdef.
            Verify new copy with checksum of new and old HELLO.EXE.
    0.7     Able to link and output CC.EXE, which works. DONE in v5.
    0.8     Same as 0.7, but now it is an exact copy of CC.EXE as linked
            by Microsoft Overlay Linker (link.exe).
            Verify new copy with checksum of new and old CC.EXE.
    0.9     Able to create new library file CLIB.LIB - an exact copy of CLIB.LIB
            that is packaged with SmallC22 (or as close as possible) from
            included archived clib file.
            If possible, verify new copy of checksum of new and old CLIB.LIB.
    1.0     Release with batch file that automates creating CLIB.LIB, HELLO.EXE,
            and CC.EXE from source.
            If it was possible to verify new CLIB.LIB, verify new CLIB.LIB,
            HELLO.EXE, and CC.EXE
    Future  Optimizations to reduce size of output EXE files. Rewrite of clib?