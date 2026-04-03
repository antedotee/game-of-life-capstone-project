#ifndef STRING_H
#define STRING_H

unsigned long str_len(const char *s);
void str_cpy(char *dst, const char *src);
int str_cmp(const char *a, const char *b);
int str_split_inplace(char *line, char delim, char **tokens, int max_tokens);
void str_from_int(int value, char *buf, unsigned long cap);

#endif
