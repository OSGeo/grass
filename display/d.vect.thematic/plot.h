/* TODO: should use 24bit instead of 16 colors, maybe implement
   predefined color tables? */
struct rgb_color
{
    unsigned char R, G, B;
};

extern int palette_ncolors;
extern struct rgb_color palette[16];

#define DISP_SHAPE 0x01
#define DISP_CAT   0x02
#define DISP_TOPO  0x04
#define DISP_DIR   0x08
#define DISP_ATTR  0x10
#define DISP_ZCOOR 0x20

#define RENDER_DP	2
#define RENDER_DPC	3
#define RENDER_DPL	4

extern int render;
