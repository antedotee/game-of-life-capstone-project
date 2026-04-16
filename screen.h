#ifndef SCREEN_H
#define SCREEN_H

void screen_clear(void);
void screen_cursor(int col, int row);
void screen_putc(char c);
void screen_puts(const char *s);
void screen_flush(void);

void screen_enter_app(void);
void screen_leave_app(void);
void screen_hide_cursor(void);
void screen_show_cursor(void);

/* ANSI SGR helpers (truecolor + reset) for richer terminal UI */
void screen_reset(void);
void screen_style_banner(void);
void screen_style_hint(void);
void screen_style_dim(void);
void screen_style_title(void);
void screen_style_panel(void);
void screen_style_alive(void);
void screen_style_alive_alt(void);
void screen_style_alive_hot(void);
void screen_style_dead(void);
void screen_style_dead_alt(void);
void screen_style_cursor_cell(void);
void screen_style_cursor_blink(void);
void screen_style_border(void);
void screen_style_border_hot(void);
void screen_style_run(void);
void screen_style_paused(void);
void screen_style_warning(void);

#endif
