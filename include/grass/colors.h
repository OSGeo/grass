#ifndef GRASS_COLORS_H
#define GRASS_COLORS_H

/* Don't add more colors here.
   These colors are the preallocated colors used in displays for efficiency.
   Adding colors here reduces the number of colors it is possible for a user to display.
   If support for named colors is needed it should go in G_str_to_color. */

#define BLACK		1
#define RED		2
#define GREEN		3
#define BLUE		4
#define YELLOW		5
#define CYAN		6
#define MAGENTA		7
#define WHITE		8
#define GRAY		9
#define ORANGE		10
#define AQUA		11
#define INDIGO		12
#define VIOLET		13
#define BROWN		14

#define GREY            GRAY
#define PURPLE          VIOLET

/* These can be in any order. They must match the lookup strings in the table below. */
#define D_COLOR_LIST "red,orange,yellow,green,blue,indigo,violet,white,black,gray,brown,magenta,aqua,grey,cyan,purple"

/* max length of color string */
#define MAX_COLOR_LEN 32

struct color_rgb
{
    unsigned char r, g, b;
};

struct color_name
{
    const char *name;
    int number;
};

#include <grass/defs/colors.h>

#endif
