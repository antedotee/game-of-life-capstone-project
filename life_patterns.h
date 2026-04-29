#ifndef LIFE_PATTERNS_H
#define LIFE_PATTERNS_H

typedef struct LifePoint {
    int x;
    int y;
} LifePoint;

typedef struct LifePattern {
    const char *name;
    const char *title;
    int width;
    int height;
    int cell_count;
    const LifePoint *cells;
} LifePattern;

int life_pattern_count(void);
const LifePattern *life_pattern_at(int index);
const LifePattern *life_find_pattern(const char *name);
void life_stamp_pattern(char *grid, int w, int h, const LifePattern *pattern, int origin_x, int origin_y);

#endif
