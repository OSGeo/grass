
/* barscale types */
#define STYLE_NONE           0
#define STYLE_CLASSIC_BAR    1
#define STYLE_PART_CHECKER   2
#define STYLE_FULL_CHECKER   3
#define STYLE_THIN_WITH_ENDS 4
#define STYLE_SOLID_BAR      5
#define STYLE_HOLLOW_BAR     6
#define STYLE_TICKS_UP       7
#define STYLE_TICKS_DOWN     8
#define STYLE_TICKS_BOTH     9
#define STYLE_ARROW_ENDS     10

/* text placement */
#define TEXT_UNDER 0
#define TEXT_OVER  1
#define TEXT_LEFT  2
#define TEXT_RIGHT 3

/* globals */
extern int fg_color;
extern int bg_color;
extern int use_feet;
extern int do_background;

/* draw_scale.c */
int draw_scale(double, double, int, int, double, char *);
