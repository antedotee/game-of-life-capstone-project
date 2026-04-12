# C Capstone: Terminal Game of Life

This folder is a self-contained **C** implementation of Conway’s Game of Life for a terminal. It follows the course rules: core logic avoids `<string.h>`, `<math.h>`, and the default `malloc`/`free` pair. A **single global memory pool** in `memory.c` backs `memory_alloc` / `memory_dealloc`. `<stdio.h>` is used only for terminal output and line input where required; `<stdlib.h>` is used for `exit` and `atexit` only.

## Custom libraries (pipeline)

| Library    | Role |
|-----------|------|
| `keyboard.c` | Non-blocking `keyboard_key_pressed`, blocking `keyboard_read_line` (via `stdio` when in cooked mode). |
| `string.c`   | Length, copy, compare, split, int-to-string (no libc string functions). |
| `memory.c`   | Global byte pool and a small first-fit allocator. |
| `math.c`     | Integer multiply/divide/modulo, absolute value, clamp, toroidal wrap, in-bounds checks. |
| `screen.c`   | ANSI clear, cursor move, character/string output, flush. |

`modes.c` (application layer, not a sixth course library) holds **argv handling** (`mode=gun`) and **classic Life patterns** as coordinate lists and **RLE** strings; stamping uses `math_wrap_index` for toroidal placement. It is `#include`d from `main.c` so the **Makefile** stays a single `main.o` plus the five libraries.

Example flow: **keyboard** reads `:` → **string** splits a command line → **memory** holds the two cell planes → **math** wraps neighbor indices and checks edit bounds → **screen** draws the grid and status.

## Built-in demos (keys **0**–**9**, edit mode only)

- **Default** (no arguments): the board starts with a **random** soup (same as the original capstone behavior).
- **Named modes** (each can be written as `mode=name` or `--mode=name`):

| Mode | Argument | What you see |
|------|-----------|----------------|
| **gun** | `mode=gun` | **Gosper glider gun** — period-30 stream of gliders (classic “infinite growth” demo). |
| **pulse** | `mode=pulse` | **Tumbler** — a compact period-**14** oscillator (rhythmic flipping motion). |
| **gates** | `mode=gates` | **Eater + glider** — still-life “circuit” piece meets a moving signal; shows the kind of **collision** logic used in real Life circuitry (not a full gate net, but the same idea in miniature). |
| **cpu** | `mode=cpu` | **Acorn** methuselah — tiny seed, very long, messy evolution (a common metaphor for **complex behavior from simple rules**, as in “Life is Turing-complete”). |

If you pass several `mode=…` arguments, the **last** one wins.

Press **P** to pause, then a digit **0**–**9** to load another preset.

| Key | Pattern | Why it is interesting |
|-----|---------|------------------------|
| **0** | **Gosper glider gun** | Period **30** gun; emits gliders — classic proof of unbounded growth from finite starting soup (see [LifeWiki](https://conwaylife.com/wiki/Gosper_glider_gun)). |
| **1** | **Glider** | Smallest spaceship; shows diagonal travel on a torus. |
| **2** | **Blinker** | Simplest period-**2** oscillator. |
| **3** | **Toad** | Another period-**2** oscillator (different shape). |
| **4** | **Beacon** | Two blocks “blinking” together. |
| **5** | **LWSS** | Lightweight spaceship (orthogonal). |
| **6** | **MWSS** | Middleweight spaceship (orthogonal). |
| **7** | **Pentadecathlon** | Period-**15** oscillator (long period in a small footprint). |
| **8** | **R-pentomino** | Five cells that behave chaotically for a long time before stabilizing. |
| **9** | **Acorn** | Famous **methuselah** (tiny seed, very long evolution); good on a **large** terminal. |

These names match what people use on [Conway Life](https://conwaylife.com/wiki/Main_Page) and common tutorials. **Logic gates** in Life are usually built from glider **collisions** and **synchronizing** streams; this build does not ship a full gate library, but the **glider gun + gliders** are the usual starting point for that kind of construction.

## Controls

- **Enter**: start running generations.
- **P**: pause (return to edit mode).
- **Space**: toggle cell under the cursor (edit mode); in run mode, pauses.
- **W A S D**: move the edit cursor (bounded with `math_in_bounds`).
- **R**: randomize the field (custom PRNG, not `rand`).
- **C**: clear all cells.
- **0**–**9** (while **paused**): load a **preset pattern** (see table above).
- **F**: faster updates (shorter delay).
- **Z**: slower updates (longer delay).
- **+ / -**: grow or shrink the grid (re-allocates both planes with `memory_alloc` / `memory_dealloc`).
- **:** then type a command and press Enter: `help`, `quit`, `random`, `clear`, `faster`, `slower`.
- **Q**: quit.

Grid width and height are derived from the terminal size (`ioctl`); they are clamped with `math_clamp`.

## Build and run

From this directory:

```bash
make
./game_of_life
./game_of_life mode=gun
./game_of_life mode=pulse
./game_of_life mode=gates
./game_of_life mode=cpu
```

The binary name is `game_of_life` (underscore). Each **`mode=…`** or **`--mode=…`** is one argv string; omit them for the normal random start.

Requires a POSIX-like terminal (macOS Terminal, Linux console, etc.). Resize the terminal to change the default grid bounds; use `+` / `-` to adjust further.

**Note:** If you edit `modes.c` without touching `main.c`, run `make clean && make` once so `main.o` is rebuilt (the Makefile does not list `modes.c` as a dependency).

## Known issues / limits

- **Terminal variance**: Arrow keys and some escape sequences are not handled; use **WASD** for movement.
- **Resize**: Very small terminals may clamp to the minimum grid size (12×8); huge sizes are capped to keep the pool allocation reasonable.
- **Performance**: The allocator does not coalesce freed blocks; heavy `+`/`-` resizing could fragment the pool over a long session (unlikely for normal play).
- **Line input**: Command mode (`:`) briefly switches the TTY to cooked mode for `read_line`; rapid input during that window can behave oddly on some systems.

## Demo

Record or screenshot the running binary for your submission; this README does not bundle images.
