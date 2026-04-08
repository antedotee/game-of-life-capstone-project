#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void);
void keyboard_shutdown(void);
int keyboard_key_pressed(char *out);
void keyboard_read_line(char *buf, unsigned long cap);

#endif
