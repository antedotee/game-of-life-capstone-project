# C Capstone: Phase 1 — Library integration demo

This folder is a **Phase 1**-style self-contained **C** demo: the five course libraries drive a terminal **editor** plus Conway’s Game of Life on a **torus**. **Enter** toggles **auto-run** (live simulation); **n** advances **one** generation while paused; **Ctrl+C** stops and restores the terminal.

**Full documentation** (zero-prerequisite guide, file-by-file explanation, and **before vs after** comparison with the earlier full Game of Life build): [docs/PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md).

Course rules still apply where relevant: core logic avoids `<string.h>`, `<math.h>`, and the default `malloc`/`free` pair. A **single global memory pool** in `memory.c` backs `memory_alloc` / `memory_dealloc`. `<stdlib.h>` is used for `exit` and `atexit` only.

## Custom libraries (pipeline)

| Library    | Role |
|-----------|------|
| `keyboard.c` | Non-blocking `keyboard_key_pressed` for the main loop. |
| `string.c`   | Integer-to-string for the status line (no libc string functions in app logic paths). |
| `memory.c`   | Global byte pool and a small first-fit allocator; the cell grid uses two allocations (visible + staging for each Conway step). |
| `math.c`     | Integer multiply, divide, clamp, toroidal wrap for stamping, in-bounds checks for the cursor. |
| `screen.c`   | ANSI clear, cursor move, truecolor SGR styles, bordered grid drawing, flush. |

Example flow: **keyboard** polls keys → when **auto-run** is on, each tick runs one toroidal Conway step (read `g_grid`, write staging, swap) → **math** counts neighbors and checks cursor bounds → **memory** holds the two planes → **string** formats generation/cursor → **screen** draws a colored, bordered frame and a small spinner.

## What you see (Phase 1)

- A **static glider** is stamped on the grid at startup (torus-aware placement via `math_wrap_index` inside `stamp_live`).
- **W A S D** moves an edit cursor (bounded with `math_in_bounds`).
- **Space** toggles the cell under the cursor.
- **B** sets a **3×3 block** around the cursor to **alive** (edges clip at the grid border).
- **K** clears that **3×3 block** to **dead**.
- **Enter** toggles **auto-run** vs **paused**. While running, generations advance automatically (same toroidal **B3/S23** rules). **n** runs **one** step when paused. **`[`** / **`]`** slow down / speed up the simulation (delay per step, shown as `ms/step` on the status line).
- The board is **square** (`width == height`): the usable terminal area yields a max width and max height in *cells*; the smaller value becomes both dimensions so the playfield is compact and even.
- The board **refits when you resize the terminal**: each frame reads `ioctl(TIOCGWINSZ)` and reallocates the two grids when derived size changes, copying the overlapping region so the pattern is preserved where it still fits.
- Cells are UTF-8 **block** glyphs (█ / ░ / ▓) inside a **light box-drawing grid**: horizontal **─** rules and vertical **│** between every cell so the lattice is obvious. Layout is **2×logical_width + 1** columns per row so the board still fits after a resize.
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
