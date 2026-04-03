#include "string.h"

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

void str_cpy(char *dst, const char *src)
{
    unsigned long i = 0;
    if (!dst || !src) {
        return;
    }
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int str_cmp(const char *a, const char *b)
{
    unsigned long i = 0;
    if (!a || !b) {
        return 0;
    }
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return (int)((unsigned char)a[i] - (unsigned char)b[i]);
        }
        i++;
    }
    return (int)((unsigned char)a[i] - (unsigned char)b[i]);
}

int str_split_inplace(char *line, char delim, char **tokens, int max_tokens)
{
    int n = 0;
    int i = 0;
    if (!line || !tokens || max_tokens <= 0) {
        return 0;
    }
    tokens[n++] = line;
    for (i = 0; line[i] != '\0' && n < max_tokens; i++) {
        if (line[i] == delim) {
            line[i] = '\0';
            if (n < max_tokens) {
                tokens[n++] = line + i + 1;
            }
        }
    }
    return n;
}

void str_from_int(int value, char *buf, unsigned long cap)
{
    unsigned long pos = 0;
    int neg = 0;
    unsigned int u = 0;
    char tmp[32];
    int t = 0;
    if (!buf || cap == 0) {
        return;
    }
    if (value < 0) {
        neg = 1;
        u = (unsigned int)(-(value + 1)) + 1u;
    } else {
        u = (unsigned int)value;
    }
    if (u == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (u > 0 && t < 31) {
        tmp[t++] = (char)('0' + (u % 10u));
        u /= 10u;
    }
    if (neg && pos + 1 < cap) {
        buf[pos++] = '-';
    }
    while (t > 0 && pos + 1 < cap) {
        buf[pos++] = tmp[--t];
    }
    buf[pos] = '\0';
}
