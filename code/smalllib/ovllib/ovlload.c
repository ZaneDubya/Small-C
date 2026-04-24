// ovlload.c -- _ovl_load: read an overlay module from the EXE into the window.
// TOC format (8 bytes per module, appended to EXE):
//   uint win_base    -- CS offset of the window area for this module
//   uint code_size   -- byte count of the module's code
//   uint exe_off_lo  -- low  word of module block's file offset in EXE
//   uint exe_off_hi  -- high word of module block's file offset in EXE
// Compiled as part of ovllib.lib.
#include <stdio.h>

extern long _ovl_toc_off;
extern int  _ovl_exe_fd;
extern int  _ovl_cs_seg;
extern void _ovl_copy(unsigned dest_off, char *src, unsigned count);

// Read one little-endian uint (2 bytes) from fd.
static int ovl_get16(int fd) {
  int lo, hi;
  lo = fgetc(fd);
  hi = fgetc(fd);
  return (hi << 8) | (lo & 0xFF);
}

void _ovl_load(int win_idx, int grp_idx) {
  long toc_entry;
  int win_base, code_size, exe_lo, exe_hi;
  long exe_off;
  char buf[128];
  int remaining, chunk;

  // Seek to the TOC entry for grp_idx
  // Each entry is 8 bytes; _ovl_toc_off is the start of the TOC.
  toc_entry = _ovl_toc_off + (long)grp_idx * 8L;
  bseek(_ovl_exe_fd, &toc_entry, 0);

  // Read the 8-byte TOC entry
  win_base  = ovl_get16(_ovl_exe_fd);
  code_size = ovl_get16(_ovl_exe_fd);
  exe_lo    = ovl_get16(_ovl_exe_fd);
  exe_hi    = ovl_get16(_ovl_exe_fd);

  // Build 32-bit file offset and seek to the module's block
  exe_off = ((long)exe_hi << 16) | ((long)exe_lo & 0xFFFFL);
  bseek(_ovl_exe_fd, &exe_off, 0);

  // Copy code_size bytes from EXE into CS:win_base in chunks
  remaining = code_size;
  while (remaining > 0) {
    chunk = (remaining > 128) ? 128 : remaining;
    fread(buf, 1, chunk, _ovl_exe_fd);
    _ovl_copy(win_base, buf, chunk);
    win_base  += chunk;
    remaining -= chunk;
  }
}
