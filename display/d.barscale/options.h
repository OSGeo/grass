
/* barscale types */
#define STYLE_NONE           0
#define STYLE_CLASSIC_BAR    1
#define STYLE_PART_CHECKER   2
#define STYLE_FULL_CHECKER   3
#define STYLE_MIXED_CHECKER  4
#define STYLE_TAIL_CHECKER   5
#define STYLE_THIN_WITH_ENDS 6
#define STYLE_SOLID_BAR      7
#define STYLE_HOLLOW_BAR     8
#define STYLE_TICKS_UP       9
#define STYLE_TICKS_DOWN     10
#define STYLE_TICKS_BOTH     11
#define STYLE_ARROW_ENDS     12

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
int draw_scale(double, double, int, int, double);

/* draw_n_arrow.c */
int draw_n_arrow(double, double, double, char *);
