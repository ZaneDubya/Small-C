// ovlmgr.c -- Overlay manager global state.
// Compiled as part of ovllib.lib.
#include <stdio.h>
#include "ovllib.h"

// Patched by the linker to the 32-bit file offset of the TOC.
long _ovl_toc_off;

// DOS file handle for the open EXE; set by ovl_init.
int _ovl_exe_fd;

// CS segment register value captured at startup.
int _ovl_cs_seg;

// Per-window currently-loaded module index (-1 = nothing loaded).
// Indexed by 0-based window index matching the overlay directive file.
int _ovl_win_grp[8];
