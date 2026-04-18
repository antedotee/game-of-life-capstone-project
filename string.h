#ifndef STRING_H
#define STRING_H

unsigned long str_len(const char *s);
unsigned long str_copy(char *dst, unsigned long cap, const char *src);
int str_compare(const char *a, const char *b);
int str_token_next(const char **cursor, char delim, char *out, unsigned long cap);
void str_from_int(int value, char *buf, unsigned long cap);

#endif
