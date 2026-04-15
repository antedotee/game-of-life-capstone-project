#include "keyboard.h"
#include <termios.h>
#include <unistd.h>

static struct termios g_saved_termios;
static int g_tty_saved;

void keyboard_init(void)
{
    struct termios raw;
    if (tcgetattr(STDIN_FILENO, &g_saved_termios) != 0) {
        return;
    }
    g_tty_saved = 1;
    raw = g_saved_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void keyboard_shutdown(void)
{
    if (g_tty_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_saved_termios);
        g_tty_saved = 0;
    }
}

int keyboard_key_pressed(char *out)
{
    unsigned char b;
    ssize_t n;
    if (!out) {
        return 0;
    }
    n = read(STDIN_FILENO, &b, 1);
    if (n == 1) {
        *out = (char)b;
        return 1;
    }
    return 0;
}
