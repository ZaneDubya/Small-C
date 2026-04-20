// tenvvars.h -- Sentinel header for the tenvvars include-path test.
//
// This file must live alongside tenvvars.exe (C:\ETC\TENVVARS\).
// doinclude() in the new ccpre.c would find it via:
//
//   dirpart(infn, srcdir)             -- e.g. "C:\ETC\TENVVARS\"
//   compose_path(srcdir, "tenvvars.h", path)
//   fopen(path, "r")                  -- succeeds
//
// When the test runs from C:\ETC (CWD), a bare fopen("tenvvars.h") fails,
// proving the new source-relative lookup is required.

#define TENVVARS_H_FOUND  1
