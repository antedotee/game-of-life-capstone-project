#ifndef MATH_H
#define MATH_H

int math_imul(int a, int b);
int math_idiv(int a, int b);
int math_imod(int a, int m);
int math_clamp(int v, int lo, int hi);
int math_wrap_index(int idx, int len);
int math_in_bounds(int x, int y, int w, int h);

#endif
