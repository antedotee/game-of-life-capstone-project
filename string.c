#include "string.h"
#include "math.h"

unsigned long str_len(const char *s)
{
    unsigned long n = 0;
    if (!s) {
        return 0;
    }
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

unsigned long str_copy(char *dst, unsigned long cap, const char *src)
{
    unsigned long i = 0;
    if (!dst || cap == 0) {
        return 0;
    }
    if (!src) {
        dst[0] = '\0';
        return 0;
    }
    while (src[i] != '\0' && i + 1u < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    while (src[i] != '\0') {
        i++;
    }
    return i;
}

int str_compare(const char *a, const char *b)
{
    unsigned long i = 0;
    unsigned char ca;
    unsigned char cb;

    if (a == b) {
        return 0;
    }
    if (!a) {
        return -1;
    }
    if (!b) {
        return 1;
    }
    while (a[i] != '\0' || b[i] != '\0') {
        ca = (unsigned char)a[i];
        cb = (unsigned char)b[i];
        if (ca < cb) {
            return -1;
        }
        if (ca > cb) {
            return 1;
        }
        i++;
    }
    return 0;
}

int str_token_next(const char **cursor, char delim, char *out, unsigned long cap)
{
    const char *p;
    unsigned long pos = 0;

    if (!cursor || !*cursor || !out || cap == 0) {
        return 0;
    }
    p = *cursor;
    while (*p == delim) {
        p++;
    }
    if (*p == '\0') {
        out[0] = '\0';
        *cursor = p;
        return 0;
    }
    while (*p != '\0' && *p != delim) {
        if (pos + 1u < cap) {
            out[pos++] = *p;
        }
        p++;
    }
    out[pos] = '\0';
    if (*p == delim) {
        p++;
    }
    *cursor = p;
    return 1;
}

void str_from_int(int value, char *buf, unsigned long cap)
{
    unsigned long pos = 0;
    int neg = 0;
    int u = 0;
    char tmp[32];
    int t = 0;
    if (!buf || cap == 0) {
        return;
    }
    if (value < 0) {
        neg = 1;
        u = -(value + 1);
        u++;
    } else {
        u = value;
    }
    if (u == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (u > 0 && t < 31) {
        tmp[t++] = (char)('0' + math_imod(u, 10));
        u = math_idiv(u, 10);
    }
    if (neg && pos + 1 < cap) {
        buf[pos++] = '-';
    }
    while (t > 0 && pos + 1 < cap) {
        buf[pos++] = tmp[--t];
    }
    buf[pos] = '\0';
}
