# C Capstone: Phase 1 — Library integration demo

This folder is a **Phase 1**-style self-contained **C** demo: the five course libraries drive a terminal **editor** plus **manual** Conway’s Game of Life steps (**Enter** advances one generation on a **torus**). There is no auto-run loop or pattern catalog.

**Full documentation** (zero-prerequisite guide, file-by-file explanation, and **before vs after** comparison with the earlier full Game of Life build): [docs/PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md).

Course rules still apply where relevant: core logic avoids `<string.h>`, `<math.h>`, and the default `malloc`/`free` pair. A **single global memory pool** in `memory.c` backs `memory_alloc` / `memory_dealloc`. `<stdlib.h>` is used for `exit` and `atexit` only.

## Custom libraries (pipeline)

| Library    | Role |
|-----------|------|
| `keyboard.c` | Non-blocking `keyboard_key_pressed` for the main loop. |
| `string.c`   | Integer-to-string for the status line (no libc string functions in app logic paths). |
| `memory.c`   | Global byte pool and a small first-fit allocator; the cell grid uses two allocations (visible + staging for each **Enter** Conway step). |
| `math.c`     | Integer multiply, divide, clamp, toroidal wrap for stamping, in-bounds checks for the cursor. |
| `screen.c`   | ANSI clear, cursor move, character/string output, flush. |

Example flow: **keyboard** polls keys → **Enter** runs one toroidal Conway step (read `g_grid`, write staging, swap) → **math** counts neighbors and checks cursor bounds → **memory** holds the two planes → **string** formats coordinates and generation → **screen** draws the frame.

## What you see (Phase 1)

- A **static glider** is stamped on the grid at startup (torus-aware placement via `math_wrap_index` inside `stamp_live`).
- **W A S D** moves an edit cursor (bounded with `math_in_bounds`).
- **Space** toggles the cell under the cursor.
- **B** sets a **3×3 block** around the cursor to **alive** (edges clip at the grid border).
- **K** clears that **3×3 block** to **dead**.
- **Enter** advances **one Conway generation** (toroidal 8-neighbor counts): a **live** cell stays alive only with **2 or 3** live neighbors; a **dead** cell becomes alive with **exactly 3** live neighbors; otherwise the cell is dead. The status line shows **gen** (number of Enter steps since last **C**).
- **C** clears the **entire** grid (both internal buffers).
- **Q** quits (frees the grid allocation then exits).

## Build and run

From this directory:

```bash
make
./game_of_life
```

The binary name is `game_of_life` (underscore).

Requires a POSIX-like terminal (macOS Terminal, Linux console, etc.). Grid size follows the terminal window (`ioctl`), clamped with `math_clamp`.

## Known issues / limits

- **Terminal variance**: Arrow keys and escape sequences are not handled; use **WASD** for movement.
- **Allocator**: The pool does not coalesce freed blocks; this build allocates the grid pair once per run (plus pool bookkeeping).
