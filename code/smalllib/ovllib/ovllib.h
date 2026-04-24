// ovllib.h -- Overlay manager interface for Small-C DOS programs.
// Include this header in programs that use overlays.
// Link with ovllib.lib (built by ovlmake.bat) and use ylink -od= option.

#ifndef OVLLIB_H
#define OVLLIB_H

// _ovl_toc_off: 32-bit file offset of the overlay TOC appended to the EXE.
// This value is patched by the linker; do not modify it at runtime.
// extern long _ovl_toc_off;

// ovl_init: open the EXE file and prepare the overlay system.
// Call once at program startup before any overlay function is invoked.
// exepath must be the full path to the running EXE (argv[0]).
void ovl_init(char *exepath);

#endif
