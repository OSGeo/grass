#include <grass/display.h>
#include "globals.h"

/*
   static int set_blue(void);
   static int set_gray(void);
   static int set_green(void);
   static int set_red(void);
   static int set_white(void);
   static int set_yellow(void);
   static int set_cur_clr(int);
 */
static int setmap_blue(void);
static int setmap_gray(void);
static int setmap_green(void);
static int setmap_red(void);
static int setmap_white(void);
static int setmap_yellow(void);

/* static int done(void); */

int set_colors(struct Colors *colors)
{
    return 0;
}

#ifdef UNUSED
int set_menu_colors(struct Colors *colors)
{

    /* SCREEN OUTLINE and CURSOR */
    Rast_add_c_color_rule((CELL) 241, 180, 180, 180, (CELL) 241, 180, 180, 180,
		     colors);
    /* RED */
    Rast_add_c_color_rule((CELL) 242, 200, 90, 90, (CELL) 242, 200, 90, 90,
		     colors);
    /* ORANGE */
    Rast_add_c_color_rule((CELL) 243, 150, 100, 50, (CELL) 243, 150, 100, 50,
		     colors);
    /* YELLOW */
    Rast_add_c_color_rule((CELL) 244, 200, 200, 10, (CELL) 244, 200, 200, 10,
		     colors);
    /* GREEN */
    Rast_add_c_color_rule((CELL) 245, 90, 200, 90, (CELL) 245, 90, 200, 90,
		     colors);
    /* BLUE */
    Rast_add_c_color_rule((CELL) 246, 90, 90, 200, (CELL) 246, 90, 90, 200,
		     colors);
    /* INDIGO */
    Rast_add_c_color_rule((CELL) 247, 100, 100, 10, (CELL) 247, 100, 100, 10,
		     colors);
    /* VIOLET */
    Rast_add_c_color_rule((CELL) 248, 150, 150, 10, (CELL) 248, 150, 150, 10,
		     colors);
    /* WHITE */
    Rast_add_c_color_rule((CELL) 249, 250, 250, 250, (CELL) 249, 250, 250, 250,
		     colors);
    /* BLACK */
    Rast_add_c_color_rule((CELL) 250, 0, 0, 0, (CELL) 250, 0, 0, 0, colors);
    /* GRAY */
    Rast_add_c_color_rule((CELL) 251, 180, 180, 180, (CELL) 251, 180, 180, 180,
		     colors);
    /* BROWN */
    Rast_add_c_color_rule((CELL) 252, 100, 100, 30, (CELL) 252, 100, 100, 30,
		     colors);
    /* MAGENTA */
    Rast_add_c_color_rule((CELL) 253, 150, 90, 150, (CELL) 253, 150, 90, 150,
		     colors);
    /* AQUA */
    Rast_add_c_color_rule((CELL) 254, 50, 120, 120, (CELL) 254, 50, 120, 120,
		     colors);
    /*      */
    Rast_add_c_color_rule((CELL) 255, 250, 0, 0, (CELL) 255, 250, 0, 0, colors);

    set_colors(colors);

    return 0;
}
#endif /* set_menu_colors */

#ifdef UNUSED
int cursor_color(void)
{

    static int use = 1;

    static Objects objects[] = {
	MENU("DONE", done, &use),
	INFO("Pick a Color ->", &use),
	MENU("BLUE", set_blue, &use),
	MENU("GRAY", set_gray, &use),
	MENU("GREEN", set_green, &use),
	MENU("RED", set_red, &use),
	MENU("WHITE", set_white, &use),
	MENU("YELLOW", set_yellow, &use),
	{0}
    };

    Input_pointer(objects);
    return 0;			/* return but don't quit */
}

static int set_blue(void)
{
    set_cur_clr(BLUE);
    return 0;
}

static int set_gray(void)
{
    set_cur_clr(GREY);
    return 0;
}

static int set_green(void)
{
    set_cur_clr(GREEN);
    return 0;
}

static int set_red(void)
{
    set_cur_clr(RED);
    return 0;
}

static int set_white(void)
{
    set_cur_clr(WHITE);
    return 0;
}

static int set_yellow(void)
{
    set_cur_clr(YELLOW);
    return 0;
}

static int set_cur_clr(int curs_color)
{
    struct Colors *colors;

    colors = &VIEW_MAP1->cell.colors;

    switch (curs_color) {
    case BLUE:
	Rast_add_c_color_rule((CELL) 241, 90, 90, 200, (CELL) 241, 90, 90, 200,
			 colors);
	break;

    case GRAY:
	Rast_add_c_color_rule((CELL) 241, 180, 180, 180, (CELL) 241, 180, 180, 180,
			 colors);
	break;

    case GREEN:
	Rast_add_c_color_rule((CELL) 241, 90, 200, 90, (CELL) 241, 90, 200, 90,
			 colors);
	break;

    case RED:
	Rast_add_c_color_rule((CELL) 241, 200, 90, 90, (CELL) 241, 200, 90, 90,
			 colors);
	break;

    case WHITE:
	Rast_add_c_color_rule((CELL) 241, 250, 250, 250, (CELL) 241, 250, 250, 250,
			 colors);
	break;

    case YELLOW:
	Rast_add_c_color_rule((CELL) 241, 200, 200, 10, (CELL) 241, 200, 200, 10,
			 colors);
	break;
    }

    set_colors(colors);
    return 0;
}
#endif /* unused cursor color */

int get_vector_color(void)
{
    static int use = 1;

    static Objects objects[] = {
	INFO("Pick color for vectors ->", &use),
	MENU("BLUE", setmap_blue, &use),
	MENU("GRAY", setmap_gray, &use),
	MENU("GREEN", setmap_green, &use),
	MENU("RED", setmap_red, &use),
	MENU("WHITE", setmap_white, &use),
	MENU("YELLOW", setmap_yellow, &use),
	{0}
    };

    Input_pointer(objects);
    return 0;			/* return but don't quit */
}

static int setmap_blue(void)
{
    return line_color = BLUE;
}

static int setmap_gray(void)
{
    return line_color = GREY;
}

static int setmap_green(void)
{
    return line_color = GREEN;
}

static int setmap_red(void)
{
    return line_color = RED;
}

static int setmap_white(void)
{
    return line_color = WHITE;
}

static int setmap_yellow(void)
{
    return line_color = YELLOW;
}

#ifdef UNUSED
static int done(void)
{
    return -1;
}
#endif /* unused cursor color */
