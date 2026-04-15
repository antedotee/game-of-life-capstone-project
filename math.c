#include "math.h"

int math_imul(int a, int b)
{
    int neg = 0;
    int r = 0;
    if (a < 0) {
        neg = !neg;
        a = -a;
    }
    if (b < 0) {
        neg = !neg;
        b = -b;
    }
    while (b > 0) {
        if (b & 1) {
            r += a;
        }
        a <<= 1;
        b >>= 1;
    }
    return neg ? -r : r;
}

int math_idiv(int a, int b)
{
    int neg = 0;
    int q = 0;
    if (b == 0) {
        return 0;
    }
    if (a < 0) {
        neg = !neg;
        a = -a;
    }
    if (b < 0) {
        neg = !neg;
        b = -b;
    }
    while (a >= b) {
        a -= b;
        q++;
    }
    return neg ? -q : q;
}

int math_imod(int a, int m)
{
    if (m == 0) {
        return 0;
    }
    int q = math_idiv(a, m);
    return a - math_imul(q, m);
}

int math_clamp(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

int math_wrap_index(int idx, int len)
{
    if (len <= 0) {
        return 0;
    }
    int r = math_imod(idx, len);
    if (r < 0) {
        r += len;
    }
    return r;
}

int math_in_bounds(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) {
        return 0;
    }
    if (x < 0 || y < 0) {
        return 0;
    }
    if (x >= w || y >= h) {
        return 0;
    }
    return 1;
}
