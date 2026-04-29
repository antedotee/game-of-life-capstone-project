#include "life_patterns.h"
#include "math.h"
#include "string.h"

#define PATTERN_CELLS(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const LifePoint k_block[] = {{0,0},{1,0},{0,1},{1,1}};
static const LifePoint k_beehive[] = {{1,0},{2,0},{0,1},{3,1},{1,2},{2,2}};
static const LifePoint k_loaf[] = {{1,0},{2,0},{0,1},{3,1},{1,2},{3,2},{2,3}};
static const LifePoint k_boat[] = {{0,0},{1,0},{0,1},{2,1},{1,2}};
static const LifePoint k_tub[] = {{1,0},{0,1},{2,1},{1,2}};
static const LifePoint k_blinker[] = {{0,0},{1,0},{2,0}};
static const LifePoint k_toad[] = {{1,0},{2,0},{3,0},{0,1},{1,1},{2,1}};
static const LifePoint k_beacon[] = {{0,0},{1,0},{0,1},{3,2},{2,3},{3,3}};
static const LifePoint k_pulsar[] = {
    {2,0},{3,0},{4,0},{8,0},{9,0},{10,0},
    {0,2},{5,2},{7,2},{12,2},
    {0,3},{5,3},{7,3},{12,3},
    {0,4},{5,4},{7,4},{12,4},
    {2,5},{3,5},{4,5},{8,5},{9,5},{10,5},
    {2,7},{3,7},{4,7},{8,7},{9,7},{10,7},
    {0,8},{5,8},{7,8},{12,8},
    {0,9},{5,9},{7,9},{12,9},
    {0,10},{5,10},{7,10},{12,10},
    {2,12},{3,12},{4,12},{8,12},{9,12},{10,12}
};
static const LifePoint k_pentadecathlon[] = {
    {1,0},{1,1},{0,2},{2,2},{1,3},{1,4},
    {1,5},{1,6},{0,7},{2,7},{1,8},{1,9}
};
static const LifePoint k_glider[] = {{1,0},{2,1},{0,2},{1,2},{2,2}};
static const LifePoint k_lwss[] = {{1,0},{4,0},{0,1},{0,2},{4,2},{0,3},{1,3},{2,3},{3,3}};
static const LifePoint k_mwss[] = {
    {3,0},{1,1},{5,1},{0,2},{0,3},{5,3},{0,4},{1,4},{2,4},{3,4},{4,4}
};
static const LifePoint k_hwss[] = {
    {3,0},{4,0},{1,1},{6,1},{0,2},{0,3},{6,3},{0,4},{1,4},{2,4},{3,4},{4,4},{5,4}
};
static const LifePoint k_rpentomino[] = {{1,0},{2,0},{0,1},{1,1},{1,2}};
static const LifePoint k_diehard[] = {{6,0},{0,1},{1,1},{1,2},{5,2},{6,2},{7,2}};
static const LifePoint k_acorn[] = {{1,0},{3,1},{0,2},{1,2},{4,2},{5,2},{6,2}};
static const LifePoint k_gosper_gun[] = {
    {24,0},{22,1},{24,1},{12,2},{13,2},{20,2},{21,2},{34,2},{35,2},
    {11,3},{15,3},{20,3},{21,3},{34,3},{35,3},
    {0,4},{1,4},{10,4},{16,4},{20,4},{21,4},
    {0,5},{1,5},{10,5},{14,5},{16,5},{17,5},{22,5},{24,5},
    {10,6},{16,6},{24,6},{11,7},{15,7},{12,8},{13,8}
};
static const LifePoint k_small_exploder[] = {{1,0},{0,1},{1,1},{2,1},{0,2},{2,2},{1,3}};
static const LifePoint k_exploder[] = {
    {0,0},{2,0},{4,0},{0,1},{4,1},{0,2},{4,2},{0,3},{4,3},{0,4},{2,4},{4,4}
};
static const LifePoint k_ten_cell_row[] = {{0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0}};

static const LifePattern k_patterns[] = {
    {"block", "Block", 2, 2, PATTERN_CELLS(k_block), k_block},
    {"beehive", "Beehive", 4, 3, PATTERN_CELLS(k_beehive), k_beehive},
    {"loaf", "Loaf", 4, 4, PATTERN_CELLS(k_loaf), k_loaf},
    {"boat", "Boat", 3, 3, PATTERN_CELLS(k_boat), k_boat},
    {"tub", "Tub", 3, 3, PATTERN_CELLS(k_tub), k_tub},
    {"blinker", "Blinker", 3, 1, PATTERN_CELLS(k_blinker), k_blinker},
    {"toad", "Toad", 4, 2, PATTERN_CELLS(k_toad), k_toad},
    {"beacon", "Beacon", 4, 4, PATTERN_CELLS(k_beacon), k_beacon},
    {"pulsar", "Pulsar", 13, 13, PATTERN_CELLS(k_pulsar), k_pulsar},
    {"pentadecathlon", "Pentadecathlon", 3, 10, PATTERN_CELLS(k_pentadecathlon), k_pentadecathlon},
    {"glider", "Glider", 3, 3, PATTERN_CELLS(k_glider), k_glider},
    {"lwss", "Lightweight Spaceship", 5, 4, PATTERN_CELLS(k_lwss), k_lwss},
    {"mwss", "Middleweight Spaceship", 6, 5, PATTERN_CELLS(k_mwss), k_mwss},
    {"hwss", "Heavyweight Spaceship", 7, 5, PATTERN_CELLS(k_hwss), k_hwss},
    {"r-pentomino", "R-pentomino", 3, 3, PATTERN_CELLS(k_rpentomino), k_rpentomino},
    {"diehard", "Diehard", 8, 3, PATTERN_CELLS(k_diehard), k_diehard},
    {"acorn", "Acorn", 7, 3, PATTERN_CELLS(k_acorn), k_acorn},
    {"gosper-gun", "Gosper Glider Gun", 36, 9, PATTERN_CELLS(k_gosper_gun), k_gosper_gun},
    {"small-exploder", "Small Exploder", 3, 4, PATTERN_CELLS(k_small_exploder), k_small_exploder},
    {"exploder", "Exploder", 5, 5, PATTERN_CELLS(k_exploder), k_exploder},
    {"ten-cell-row", "Ten Cell Row", 10, 1, PATTERN_CELLS(k_ten_cell_row), k_ten_cell_row}
};

int life_pattern_count(void)
{
    return PATTERN_CELLS(k_patterns);
}

const LifePattern *life_pattern_at(int index)
{
    if (index < 0 || index >= life_pattern_count()) {
        return 0;
    }
    return &k_patterns[index];
}

const LifePattern *life_find_pattern(const char *name)
{
    int i;
    if (!name) {
        return 0;
    }
    for (i = 0; i < life_pattern_count(); i++) {
        if (str_compare(k_patterns[i].name, name) == 0) {
            return &k_patterns[i];
        }
    }
    return 0;
}

void life_stamp_pattern(char *grid, int w, int h, const LifePattern *pattern, int origin_x, int origin_y)
{
    int i;
    if (!grid || !pattern || w <= 0 || h <= 0) {
        return;
    }
    for (i = 0; i < pattern->cell_count; i++) {
        int x = origin_x + pattern->cells[i].x;
        int y = origin_y + pattern->cells[i].y;
        if (math_in_bounds(x, y, w, h)) {
            grid[math_imul(y, w) + x] = 1;
        }
    }
}
