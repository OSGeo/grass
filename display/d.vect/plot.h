
#ifndef PLOT_H_
#define PLOT_H_
/* TODO: should use 24bit instead of 16 colors, maybe implement
   predefined color tables? */

struct rgb_color
{
    unsigned char R, G, B;
};

extern int palette_ncolors;
extern struct rgb_color palette[16];

typedef struct
{
    int field;
    int has_bgcolor;
    int has_bcolor;
    struct rgb_color color, bgcolor, bcolor;
    int size;
    const char *font;
    const char *enc;
    int xref, yref;
} LATTR;

#define LCENTER  0
#define LLEFT    1
#define LRIGHT   2
#define LBOTTOM  3
#define LTOP     4


#define DISP_SHAPE 0x01
#define DISP_CAT   0x02
#define DISP_TOPO  0x04
#define DISP_VERT  0x08
#define DISP_DIR   0x10
#define DISP_ATTR  0x20
#define DISP_ZCOOR 0x40

#define RENDER_DP	2
#define RENDER_DPC	3
#define RENDER_DPL	4

extern int render;

#endif

