/* text draw truetypefont
 *
 * 2004/01/30
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/config.h>
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

/*#define DEBUG_LOG(S) {FILE *fp = fopen("debug.TXT","a");fputs(S,fp);fclose(fp);} */
/*#define DEBUG_LOG_INT(D) {FILE *fp = fopen("debug.TXT","a");fprintf(fp,"%d",D);fclose(fp);} */
/*#define DEBUG_LOG_DOUBLE(D) {FILE *fp = fopen("debug.TXT","a");fprintf(fp,"%f",D);fclose(fp);} */

struct rectangle
{
    double t, b, l, r;
};

#ifdef HAVE_FT2BUILD_H
static int convert_str(const char *, const char *, unsigned char **);
static void release_convert_str(unsigned char *);
static void set_matrix(FT_Matrix *);
static void draw_text(FT_Face, FT_Vector *, FT_Matrix *,
		      const unsigned char *, int, int, struct rectangle *);
static void draw_bitmap(FT_Bitmap *, FT_Int, FT_Int);
static void set_text_box(FT_Bitmap *, FT_Int, FT_Int, struct rectangle *);
#endif

static void draw_main(double x, double y, const char *string,
		      struct rectangle *box)
{
#ifdef HAVE_FT2BUILD_H
    FT_Library library;
    FT_Face face;
    FT_Matrix matrix;

    /*FT_UInt glyph_index; */
    FT_Vector pen;
    FT_Error ans;
    const char *filename;
    const char *encoding;
    int font_index;
    unsigned char *out;
    int outlen;

    /* get file name */
    filename = font_get_freetype_name();
    encoding = font_get_encoding();
    font_index = font_get_index();

    /* set freetype */
    ans = FT_Init_FreeType(&library);
    if (ans) {
	/* DEBUG_LOG("Text3 error: ft init\n"); */
	return;
    }
    ans = FT_New_Face(library, filename, font_index, &face);
    if (ans == FT_Err_Unknown_File_Format) {
	/* DEBUG_LOG("Text3 error: ft new face 1\n"); */
	FT_Done_FreeType(library);
	return;
    }
    else if (ans) {
	/* DEBUG_LOG("Text3 error: ft new face 2\n"); */
	FT_Done_FreeType(library);
	return;
    }

    /* ans = FT_Set_Pixel_Sizes(face,10,10); */
    /* ans = FT_Set_Char_Size(face,text_size_x*64,text_size_y*64,0,0); */
    /* ans = FT_Set_Char_Size(face,10*64,0,72,0); */
    /* ans = FT_Set_Char_Size(face,text_size_x*64,text_size_y*64,72,72); */
    ans = FT_Set_Char_Size(face,
			   (int)(text_size_x * 64),
			   (int)(text_size_y * 64),
			   100, 100);

    if (ans) {
	/* DEBUG_LOG("Text3 error: ft set size\n"); */
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return;
    }

    /* init point */
    pen.x = x * 64;
    /* pen.y = 0; */
    pen.y = (screen_height - y) * 64;

    /* convert string to:shift-jis from:encoding */
    outlen = convert_str(encoding, string, &out);

    /* set matrix */
    set_matrix(&matrix);
    /* draw */
    draw_text(face, &pen, &matrix, out, outlen, 0, box);

    /* release */
    release_convert_str(out);

    /* FT_done */
    FT_Done_Face(face);
    FT_Done_FreeType(library);
#endif
}

#ifdef HAVE_FT2BUILD_H
static void set_matrix(FT_Matrix * matrix)
{
    /* rotation is in radians */
    matrix->xx = (FT_Fixed) ( text_cosrot * 0x10000);
    matrix->xy = (FT_Fixed) (-text_sinrot * 0x10000);
    matrix->yx = (FT_Fixed) ( text_sinrot * 0x10000);
    matrix->yy = (FT_Fixed) ( text_cosrot * 0x10000);
}

static int convert_str(const char *from, const char *in, unsigned char **out)
{
    size_t len, i, res;
    const unsigned char *p1;
    unsigned char *p2;

    len = strlen(in);
    res = 2 * (len + 1);

    *out = G_calloc(1, res);
    p1 = (const unsigned char *)in;
    p2 = *out;

#ifdef HAVE_ICONV_H
    {
	iconv_t cd;

	i = res;
        cd = iconv_open("UCS-2BE", from);
	if (cd == (iconv_t) -1)
	    return -1;
	if (iconv(cd, (char **)&p1, &len, (char **)&p2, &i) == (size_t) -1)
            return -1;
	iconv_close(cd);

	res -= i;
    }
#else
    for (i = 0; i <= len; i++)
	/* Pad each character out to 2 bytes, i.e. UCS-2 Big Endian encoding
	 * (note low byte has already been zeroed by G_calloc() call) */
	p2[2 * i + 1] = p1[i];

    res = 2 * len;
#endif

    return res;
}

static void release_convert_str(unsigned char *out)
{
    G_free(out);
}

static void draw_text(FT_Face face, FT_Vector * pen, FT_Matrix * matrix,
		      const unsigned char *out, int len, int color,
		      struct rectangle *box)
{
    FT_ULong ch;
    FT_Error ans;
    FT_GlyphSlot slot = face->glyph;
    int i;

    for (i = 0; i < len; i += 2) {
	ch = (out[i] << 8) | out[i + 1];
	if (ch == 10)
	    continue;
	/* transform */
	FT_Set_Transform(face, matrix, pen);
	/* get glyph image */
	ans = FT_Load_Char(face, ch, FT_LOAD_NO_BITMAP);
	if (ans)
	    continue;
	ans = FT_Render_Glyph(face->glyph, ft_render_mode_normal);
	if (ans)
	    continue;
	/* draw bitmap */
	if (!box)
	    draw_bitmap(&slot->bitmap, slot->bitmap_left,
			screen_height - slot->bitmap_top);
	else
	    set_text_box(&slot->bitmap, slot->bitmap_left,
			 screen_height - slot->bitmap_top, box);

	/* increment pen position */
	pen->x += slot->advance.x;
	pen->y += slot->advance.y;
    }
}

static void set_text_box(FT_Bitmap *bitmap, FT_Int x, FT_Int y, struct rectangle *box)
{
    FT_Int xMax = x + bitmap->width;
    FT_Int yMax = y + bitmap->rows;

    if ((x == xMax) || (y == yMax))
	return;
    if (x < box->l)
	box->l = x;
    if (xMax > box->r)
	box->r = xMax;
    if (y < box->t)
	box->t = y;
    if (yMax > box->b)
	box->b = yMax;
}

static void draw_bitmap(FT_Bitmap * bitmap, FT_Int x, FT_Int y)
{
    static unsigned char *buf;
    static int nalloc;
    int w, h;
    int bw = bitmap->width;
    int bh = bitmap->rows;
    const unsigned char *sbuf = bitmap->buffer;
    int offset, i, j;
    double x1, y1, x2, y2;

    x1 = x;
    y1 = y;
    x2 = x1 + bw;
    y2 = y1 + bh;

    w = x2 - x1;
    h = y2 - y1;
    if (w <= 0 || h <= 0)
	return;

    offset = ((int)y1 - y) * bw + (int)x1 - x;

    if (nalloc < w * h) {
	nalloc = w * h;
	buf = G_realloc(buf, nalloc);
    }

    for (j = 0; j < h; j++)
	for (i = 0; i < w; i++)
	    buf[j * w + i] = sbuf[offset + j * bw + i];

    COM_Pos_abs(x1, y1);
    COM_Bitmap(w, h, 128, buf);
}
#endif

void soft_text_freetype(const char *string)
{
    draw_main(cur_x, cur_y, string, NULL);
}

void get_text_ext_freetype(const char *string, double *top, double *bot, double *left, double *rite)
{
    struct rectangle box;

    box.t = 1e300;
    box.b = -1e300;
    box.l = 1e300;
    box.r = -1e300;

    draw_main(cur_x, cur_y, string, &box);

    *top = box.t;
    *bot = box.b;
    *left = box.l;
    *rite = box.r;
}

