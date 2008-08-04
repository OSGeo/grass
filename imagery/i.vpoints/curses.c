#include <stdlib.h>
#include <unistd.h>
#include "globals.h"

static int inited = 0;

static WINDOW *save;
WINDOW *newwin();

static Window *make_window(int top, int bottom, int left, int right)
{
    Window *window;

    if (top < 0 || bottom >= LINES || left < 0 || right >= COLS
	|| bottom - top <= 1 || right - left <= 1) {
	End_curses();
	fprintf(stderr, "make_window(%d,%d,%d,%d): illegal screen values\n",
		top, bottom, left, right);
	G_sleep(3);
	exit(1);
    }
    window = (Window *) G_malloc(sizeof(Window));
    window->top = top;
    window->bottom = bottom;
    window->left = left;
    window->right = right;
    Curses_clear_window(window);
    return window;
}

int Begin_curses(void)
{
    /* should only be called once at program outset */

    initscr();			/* initialize curses standard screens   */
    raw();			/* set tty modes via curses calls       */
    noecho();
    nonl();

    inited = 1;

    /* make a window to save stdscr */
    save = newwin(LINES, COLS, 0, 0);

    /* make_window (nrows, ncols, start_row, start_col) */
    INFO_WINDOW = make_window(0, LINES - 4, COLS / 2, COLS - 1);
    MENU_WINDOW = make_window(0, LINES - 4, 0, COLS / 2);
    PROMPT_WINDOW = make_window(LINES - 4, LINES - 1, 0, COLS - 1);
    refresh();

    return 0;
}

int End_curses(void)
{
    /* should only be called upon program exit */

    clear();			/* clear the screen */
    refresh();
    endwin();			/* let curses reset the tty now */
    return 0;
}

int Suspend_curses(void)
{
    overwrite(stdscr, save);
    clear();
    refresh();
    endwin();

    return 0;
}

int Resume_curses(void)
{
    clear();
    refresh();
    overwrite(save, stdscr);
    refresh();

    return 0;
}

int Curses_allow_interrupts(int ok)
{
    refresh();
    if (ok)
	noraw();
    else
	raw();

    return 0;
}

int Curses_clear_window(Window * window)
{
    int y, x;

    if (!inited)
	return 1;
    for (y = window->top + 1; y < window->bottom; y++) {
	move(y, x = window->left + 1);
	while (x++ < window->right)
	    addch(' ');
    }
    Curses_outline_window(window);
    refresh();

    return 0;
}

int Curses_outline_window(Window * window)
{
    int x, y;

    move(window->top, x = window->left + 1);
    while (x++ < window->right)
	addch('-');
    move(window->bottom, x = window->left + 1);
    while (x++ < window->right)
	addch('-');
    for (y = window->top + 1; y < window->bottom; y++) {
	move(y, window->left);
	addch('|');
	move(y, window->right);
	addch('|');
    }
    move(window->top, window->left);
    addch('+');
    move(window->top, window->right);
    addch('+');
    move(window->bottom, window->left);
    addch('+');
    if (window->bottom < LINES - 1 || window->right < COLS - 1) {
	move(window->bottom, window->right);
	addch('+');
    }

    return 0;
}

int Curses_write_window(Window * window, int line, int col, char *message)
{
    int y, x, i;

    if (!inited) {
	fprintf(stderr, "%s\n", message);
	return 1;
    }
    if (line <= 0 || line >= window->bottom - window->top)
	return 1;
    if (col <= 0 || col >= window->right - window->left)
	return 1;
    move(y = window->top + line, x = window->left + col);
    while (*message != 0 && *message != '\n' && x < window->right) {
	addch(*message);
	message++;
	x++;
    }
    if (*message == '\n')
	for (i = x; i < window->right; i++)
	    addch(' ');
    move(y, x);
    refresh();

    return 0;
}


int Curses_replot_screen(void)
{
    int x, y;

    getyx(stdscr, y, x);
    wrefresh(curscr);
    move(y, x);
    refresh();

    return 0;
}

int Curses_prompt_gets(char *prompt, char *answer)
{
    char c;
    int n;
    int y, x;

    *answer = 0;
    n = 0;

    Curses_write_window(PROMPT_WINDOW, 1, 1, "\n");
    Curses_write_window(PROMPT_WINDOW, 1, 1, prompt);

    for (;;) {
	refresh();
	c = Curses_getch(0);

	if (c == '\n' || c == '\r')
	    break;

	getyx(stdscr, y, x);

	if (c > '\037' && c < '\177') {	/* octal codes: accept space to '~' */
	    if (x < PROMPT_WINDOW->right) {
		*answer++ = c;
		*answer = 0;
		addch(c);
		n++;
	    }
	    continue;
	}

	if (c == '\b' || c == '\177') {	/* backspace or DEL (decimal 8,127) */
	    if (n > 0) {
		answer--;
		*answer = 0;
		move(y, x - 1);
		addch(' ');
		move(y, x - 1);
		n--;
	    }
	    continue;
	}
	Beep();
    }

    return 0;
}

int Beep(void)
{
    putchar('\7');
    fflush(stdout);

    return 0;
}

int Curses_getch(int with_echo)
{
    char achar;
    int c;
    int kill;

    if (!inited)
	return 0;
    kill = 0;
    while (1) {
	c = getch() & 0177;
	if (c == interrupt_char) {
	    if (kill++ >= 3) {
		End_curses();
		exit(0);
	    }
	    continue;
	}
	kill = 0;
	if (c != 18)
	    break;
	Curses_replot_screen();
    }
    if (with_echo) {
	achar = c;
	addch(achar);
	refresh();
    }
    return c;
}
