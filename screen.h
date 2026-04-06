#ifndef SCREEN_H
#define SCREEN_H

void screen_clear(void);
void screen_cursor(int col, int row);
void screen_putc(char c);
void screen_puts(const char *s);
void screen_flush(void);

#endif
