#include <curses.h>
#include <grass/gis.h>
#include <grass/colors.h>


/*               " <------------ Room for text --------------> " */
#define LINE01	 "  R/r  increase/decrease RED"
#define LINE02	 "  G/g  increase/decrease GREEN"
#define LINE03	 "  B/b  increase/decrease BLUE"
#define LINE04	 "  I/i  increase/decrease increment"
#define LINE05	 "  h    highlight current category"
#define LINE06	 ""
#define LINE07	 "  D/d  down; move to next category"
#define LINE08	 "  U/u  up  ; move to previous category"
#define LINE09	 "  +/-  shift entire color table"
#define LINE10	 "  c    save color table"
#define LINE11	 "  t    toggle different color table"
#define LINE12	 "  Q    quit"
#define LINE13	 "  *  Replot screen"

WINDOW *newwin();
WINDOW *CAT_WIN;
WINDOW *COLR_WIN;
WINDOW *MENU_WIN;
WINDOW *MESG_WIN;


int Initialize_curses(void)
{
    initscr();
    raw();
    crmode();
    noecho();
    nonl();

    /*         newwin(NROWS, NCOLS, BEGROW, BEGCOL) ; */
    CAT_WIN = newwin(9, 80, 0, 0);
    COLR_WIN = newwin(9, 30, 8, 0);
    MENU_WIN = newwin(15, 51, 8, 29);
    MESG_WIN = newwin(5, 30, 16, 0);

    werase(CAT_WIN);
    werase(COLR_WIN);
    werase(MENU_WIN);
    werase(MESG_WIN);

    return 0;
}

int Close_curses(void)
{
    clear();
    refresh();
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();

    return 0;
}

int Write_cats(struct Categories *pcats, int current_cat)
{
    char buffer[128];
    CELL tmp = current_cat;
    int start_cat, end_cat, at_cat, at_line;

    if (G_is_c_null_value(&tmp))
	current_cat = 0;
    else
	current_cat++;
    G_set_c_null_value(&tmp, 1);
    start_cat = current_cat - 2;
    start_cat = start_cat > 0 ? start_cat : 0;
    end_cat = start_cat + 4;
    end_cat = end_cat < pcats->num + 1 ? end_cat : pcats->num + 1;

    werase(CAT_WIN);
    wmove(CAT_WIN, 1, 20);
    sprintf(buffer, "CATEGORIES:   %3d to %3d of %3d",
	    start_cat, end_cat, pcats->num);
    waddstr(CAT_WIN, buffer);

    at_line = 3;

    for (at_cat = start_cat; at_cat <= pcats->num + 1; at_cat++) {
	if (at_cat == current_cat) {
	    if (!at_cat)
		sprintf(buffer, "-> %3s %s", "N",
			G_get_c_raster_cat(&tmp, pcats));
	    else
		sprintf(buffer, "-> %3d %s", at_cat - 1,
			G_get_cat(at_cat - 1, pcats));
	}
	else {
	    if (!at_cat)
		sprintf(buffer, "   %3s %s", "N",
			G_get_c_raster_cat(&tmp, pcats));
	    else
		sprintf(buffer, "   %3d %s", at_cat - 1,
			G_get_cat(at_cat - 1, pcats));
	}
	wmove(CAT_WIN, at_line++, 1);
	waddstr(CAT_WIN, buffer);
    }

    box(CAT_WIN, '|', '-');
    wrefresh(CAT_WIN);
    return (0);
}

int Write_menu(void)
{
    werase(MENU_WIN);
    wmove(MENU_WIN, 1, 1);
    waddstr(MENU_WIN, LINE01);
    wmove(MENU_WIN, 2, 1);
    waddstr(MENU_WIN, LINE02);
    wmove(MENU_WIN, 3, 1);
    waddstr(MENU_WIN, LINE03);
    wmove(MENU_WIN, 4, 1);
    waddstr(MENU_WIN, LINE04);
    wmove(MENU_WIN, 5, 1);
    waddstr(MENU_WIN, LINE05);
    wmove(MENU_WIN, 6, 1);
    waddstr(MENU_WIN, LINE06);
    wmove(MENU_WIN, 7, 1);
    waddstr(MENU_WIN, LINE07);
    wmove(MENU_WIN, 8, 1);
    waddstr(MENU_WIN, LINE08);
    wmove(MENU_WIN, 9, 1);
    waddstr(MENU_WIN, LINE09);
    wmove(MENU_WIN, 10, 1);
    waddstr(MENU_WIN, LINE10);
    wmove(MENU_WIN, 11, 1);
    waddstr(MENU_WIN, LINE11);
    wmove(MENU_WIN, 12, 1);
    waddstr(MENU_WIN, LINE12);
    wmove(MENU_WIN, 13, 1);
    waddstr(MENU_WIN, LINE13);

    box(MENU_WIN, '|', '-');
    wrefresh(MENU_WIN);

    return 0;
}

int
Write_status(int red, int grn, int blu, int shift_incr, int at_cat,
	     int hi_mode)
{
    char buffer[40];
    CELL tmp = at_cat;

    werase(COLR_WIN);

    if (hi_mode)
	sprintf(buffer, "HIGHLIGHT COLOR");
    else {
	if (!G_is_c_null_value(&tmp))
	    sprintf(buffer, "CATEGORY NUMBER: %d", at_cat);
	else
	    sprintf(buffer, "CATEGORY NUMBER: N");
    }
    wmove(COLR_WIN, 1, 3);
    waddstr(COLR_WIN, buffer);

    sprintf(buffer, "       RED: %3d   %3d%%", red, (int)((float)red / 2.56));
    wmove(COLR_WIN, 3, 3);
    waddstr(COLR_WIN, buffer);

    sprintf(buffer, "     GREEN: %3d   %3d%%", grn, (int)((float)grn / 2.56));
    wmove(COLR_WIN, 4, 3);
    waddstr(COLR_WIN, buffer);

    sprintf(buffer, "      BLUE: %3d   %3d%%", blu, (int)((float)blu / 2.56));
    wmove(COLR_WIN, 5, 3);
    waddstr(COLR_WIN, buffer);

    sprintf(buffer, "SHIFT INCR: %3d   %3d%%", shift_incr,
	    (int)((float)shift_incr / 2.56));
    wmove(COLR_WIN, 7, 3);
    waddstr(COLR_WIN, buffer);

    box(COLR_WIN, '|', '-');
    wmove(COLR_WIN, 0, 0);
    wrefresh(COLR_WIN);

    return 0;
}

int Write_message(int line, char *message)
{
    wmove(MESG_WIN, line, 1);
    waddstr(MESG_WIN, "                            ");
    wmove(MESG_WIN, line, 1);
    waddstr(MESG_WIN, message);
    wmove(MESG_WIN, 0, 0);
    wrefresh(MESG_WIN);

    return 0;
}

int Clear_message(void)
{
    werase(MESG_WIN);
    wrefresh(MESG_WIN);

    return 0;
}

int Clear_menu(void)
{
    werase(MENU_WIN);
    box(MENU_WIN, '|', '-');

    return 0;
}

int Write_menu_line(int line, char *message)
{
    wmove(MENU_WIN, line, 1);
    wclrtoeol(MENU_WIN);
    wmove(MENU_WIN, line, 1);
    waddstr(MENU_WIN, message);
    wrefresh(MENU_WIN);

    return 0;
}

int Replot_screen(void)
{
    wrefresh(curscr);

    return 0;
}

int Get_curses_text(char answer[])
{
    char newchar;
    char *pointer;
    int curx, cury;

    pointer = answer;

    for (;;) {
	newchar = wgetch(MENU_WIN) & 0177;

	if ((newchar > '\037') && (newchar < '\177')) {	/* octal codes: accept space to '~' */
	    *(pointer++) = newchar;
	    *pointer = 000;
	    waddch(MENU_WIN, newchar);
	    wrefresh(MENU_WIN);
	}
	else if (newchar == '\b' || newchar == '\177') {	/* backspace or DEL */
	    if (pointer > answer) {
		*(pointer--) = 000;
		getyx(MENU_WIN, cury, curx);
		wmove(MENU_WIN, cury, curx - 1);
		waddch(MENU_WIN, ' ');
		wmove(MENU_WIN, cury, curx - 1);
		wrefresh(MENU_WIN);
	    }
	}
	else
	    break;
    }

    return 0;
}
