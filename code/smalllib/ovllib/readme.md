# Small-C Overlay Library (ovllib)

## What it provides

`ovllib.lib` implements a transparent overlay system for 16-bit DOS programs built with the Small-C v2.5 toolchain. Overlay modules are demand-loaded from the EXE file at runtime; the linker replaces cross-overlay `CALL` instructions with 11-byte stubs that invoke the loader automatically before jumping to the real function.

## Terminology

**Window** — a fixed-size region reserved in the resident CODE segment. All modules assigned to the same window share that address space; only one module occupies it at any moment. The window is sized to the largest of its member modules.

**Module** — one or more OBJ files that are loaded and evicted together. Every module belongs to exactly one window. When a function in module M is called and M is not the currently-loaded occupant of its window, the loader reads M's code block from the EXE and copies it into the window before the call proceeds.

**Stub** — an 11-byte trampoline emitted by the linker in the resident CODE segment for every cross-module call site. The stub checks whether the target module is already loaded, loads it if not, then jumps to the target function.

## EXE file layout

```
Offset 0x0000 : MZ header (0x40 bytes, 4 paragraphs)
Offset 0x0040 : CODE segment (resident code + stubs + window areas)
              :   [resident functions]
              :   [stub area -- 11 bytes per unique cross-module entry point]
              :   [window 0 -- max code size among its member modules]
              :   [window 1 -- max code size among its member modules]
              :   ...
              : (padded to a paragraph boundary so DATA starts on an even paragraph)
Offset ...    : DATA segment (all globals, resident and overlay alike)
Offset ...    : Stack (64 bytes by default)
              :
              : --- appended after the MZ image ---
              :
Offset ...    : [module 0 code block]  (snapshotted at link time)
Offset ...    : [module 1 code block]
Offset ...    : [module N code block]
Offset ...    : TOC -- 8 bytes per module, in module-index order
```

### TOC entry format (8 bytes each)

| Field       | Size | Description |
|-------------|------|-------------|
| `win_base`  | 2    | CS offset of the window where this module loads |
| `code_size` | 2    | Byte count of this module's code block |
| `exe_off_lo`| 2    | Low word of the module block's file offset in the EXE |
| `exe_off_hi`| 2    | High word of the module block's file offset in the EXE |

`_ovl_toc_off` (a `long` in the DATA segment) is patched by the linker to the file offset of the first TOC entry.

## Stub layout (11 bytes)

```asm
68 lo hi   PUSH imm16   ; encoded = (win_idx << 8) | module_idx
E8 lo hi   CALL rel16   ; near call to _ovl_check
58         POP AX       ; discard encoded from stack
68 lo hi   PUSH imm16   ; CS offset of the real overlay function
C3         RET          ; indirect jump to the function
```

`_ovl_check` decodes `win_idx` and `module_idx` from the pushed word. If `_ovl_win_mod[win_idx] != module_idx` it calls `_ovl_load` to copy the module's code block from the EXE into the window, then updates `_ovl_win_mod[win_idx]`. Control then returns through the stub's `PUSH target / RET` sequence.

## Runtime load sequence

1. `_ovl_check(encoded)` is called via the stub.
2. It extracts `win_idx = encoded >> 8` and `module_idx = encoded & 0xFF`.
3. If `_ovl_win_mod[win_idx] == module_idx` the module is already in the window; `_ovl_check` returns immediately.
4. Otherwise `_ovl_load(win_idx, module_idx)` is called:
   - Seeks to `_ovl_toc_off + module_idx * 8` in the open EXE file handle.
   - Reads `win_base`, `code_size`, and the 32-bit `exe_off`.
   - Seeks to `exe_off` and copies `code_size` bytes into CS:`win_base` via `_ovl_copy` (REP MOVSB into CS).
5. `_ovl_win_mod[win_idx]` is updated to `module_idx`.
6. The stub pushes the target function's CS offset and executes `RET`, jumping to the now-loaded function.

## Designing your program

**Resident code** (main `.c` file; linked without an overlay directive):
- `main()`, startup/init, and `ovl_init()` itself
- Any function called *from* overlay modules (overlays call resident code as direct near calls; no stub is needed)
- Any function whose address is taken at runtime

**Overlay modules** (one `.c` file per module; listed under a `MODULE` directive):
- Functions that are not needed simultaneously with other modules in the same window
- Each module is its own compiled `.obj` file
- Globals defined inside an overlay module live in the resident DATA segment permanently; only the code is swapped in and out

## Including the header

```c
#include "..\..\smalllib\ovllib\ovllib.h"
```

Call `ovl_init` once at startup, before any overlay function is invoked:

```c
int main(int argc, char **argv) {
    ovl_init(argv[0]);   /* open EXE, capture CS, init window state */
    ...
}
```

`argv[0]` must be the full path to the running EXE. When linked against `clib.lib`, the Small-C startup code in `CSYSLIB.C` reads the real EXE path from the DOS environment block and passes it as `argv[0]`.

## Overlay directive file

Create a plain-text `.ovl` file describing your windows and modules:

```
; myprog.ovl -- overlay layout
WINDOW 1
WINDOW 2

MODULE name 1
win1mod1.OBJ

MODULE name 1
win1mod2.OBJ

MODULE name 1
win1mod3.OBJ

MODULE name 2
win2mod1.OBJ

MODULE name 2
win2mod2.OBJ

MODULE name 2
win2mod3.OBJ
```

Rules:
- `WINDOW n` — declare a window with user ID `n` (positive integer, 1-based)
- `MODULE name winnum` — start a module assigned to the window with ID `winnum`
- A line ending in `.OBJ` immediately following a `MODULE` line — assign that object file to the current module (case-insensitive basename match against linker inputs)
- Lines starting with `;` and blank lines are ignored

## Linking

Pass the directive file with `-od=`:

```
ylink main.obj draw_cga.obj draw_ega.obj draw_vga.obj ^
      snd_c4.obj snd_e4.obj snd_g4.obj ^
      ..\smalllib\clib.lib ..\smalllib\ovllib.lib ^
      -e=myprog.exe -od=myprog.ovl
```

`ovllib.lib` must appear **after** your own object files so that `_ovl_check`, `_ovl_toc_off`, etc. are resolved and patched correctly by the linker.

## Rules and limitations

| Rule | Detail |
|---|---|
| All code is near | One 64 KB CS segment; all calls and stubs use 16-bit offsets |
| Resident -> overlay | Always safe; the linker inserts a stub automatically |
| Overlay -> resident | Always safe; direct near call, no stub needed |
| Overlay -> same module | Safe; the module is already loaded |
| Overlay -> different module, same window | The called module is loaded, evicting the caller — only safe if the caller does not return and access overlay-local state afterwards; the linker emits a warning |
| Overlay -> different window | Safe; each window is independently managed |
| Maximum windows | 8 |
| Maximum modules | 32 |
| Maximum stubs | 256 |

## Worked example

See `code/etc/ovltest/` for a complete program with 3 video-mode drawing modules (window 1) and 3 OPL2 note modules (window 2).
