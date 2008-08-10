#include <grass/glocale.h>
#include "cairodriver.h"

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

static cairo_matrix_t mat;

static char *convert(const char *in)
{
    size_t ilen, olen;
    char *out;

    ilen = strlen(in);
    olen = 3 * ilen + 1;

    out = G_malloc(olen);

#ifdef HAVE_ICONV_H
    {
	char *p1 = (char *) in;
	char *p2 = out;
	size_t ret;
	iconv_t cd;

	if (!encoding)
	    encoding = G_store("US-ASCII");

	if ((cd = iconv_open("UTF-8", encoding)) < 0)
	    G_fatal_error(_("Unable to convert from <%s> to UTF-8"));

	ret = iconv(cd, &p1, &ilen, &p2, &olen);

	iconv_close(cd);

	*p2++ = '\0';

	if (ret > 0)
	    G_warning(_("Some characters could not be converted to UTF-8"));
    }
#else
    {
	const unsigned char *p1 = (const unsigned char *) in;
	unsigned char *p2 = (unsigned char *) out;
	int i, j;

	for (i = j = 0; i < len; i++) {
	    int c = p1[i];
	    if (c < 0x80)
		p2[j++] = c;
	    else {
		p2[j++] = 0xC0 + (c >> 6);
		p2[j++] = 0x80 + (c & 0x3F);
	    }
	}

	p2[j++] = '\0';
    }
#endif

    return out;
}

static void set_matrix(void)
{
    if (matrix_valid)
	return;

    cairo_matrix_init_identity(&mat);
    cairo_matrix_scale(&mat, text_size_x * 25, text_size_y * 25);
    cairo_matrix_rotate(&mat, -text_rotation * M_PI / 180);

    cairo_set_font_matrix(cairo, &mat);

    matrix_valid = 1;
}

void Cairo_draw_text(const char *str)
{
    char *utf8 = convert(str);

    if (!utf8)
	return;

    set_matrix();
    cairo_move_to(cairo, cur_x, cur_y);
    cairo_show_text(cairo, utf8);

    G_free(utf8);

    modified = 1;
}

void Cairo_text_box(const char *str, double *t, double *b, double *l, double *r)
{
    char *utf8 = convert(str);
    cairo_text_extents_t ext;

    if (!utf8)
	return;

    set_matrix();

    cairo_text_extents(cairo, utf8, &ext);

    G_free(utf8);

    *l = cur_x + ext.x_bearing;
    *r = cur_x + ext.x_bearing + ext.width;
    *t = cur_x + ext.y_bearing;
    *b = cur_x + ext.y_bearing + ext.height;
}

void Cairo_set_font(const char *name)
{
    char *font = G_store(name);
    cairo_font_weight_t weight = CAIRO_FONT_WEIGHT_NORMAL;
    cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;

    for (;;) {
	char *p = strrchr(font, '-');
	if (!p)
	    break;

	if (G_strcasecmp(p, "-bold") == 0)
	    weight = CAIRO_FONT_WEIGHT_BOLD;
	else if (strcasecmp(p, "-italic") == 0)
	    slant = CAIRO_FONT_SLANT_ITALIC;
	else if (G_strcasecmp(p, "-oblique") == 0)
	    slant = CAIRO_FONT_SLANT_OBLIQUE;
	else
	    break;

	*p = '\0';
    }

    cairo_select_font_face(cairo, font, slant, weight);

    G_free(font);
}

void Cairo_set_encoding(const char *enc)
{
    if (encoding)
	G_free(encoding);

    encoding = G_store(enc);
}

void Cairo_text_size(double width, double height)
{
    text_size_x = width;
    text_size_y = height;
    matrix_valid = 0;
}

void Cairo_text_rotation(double angle)
{
    text_rotation = angle;
    matrix_valid = 0;
}

void Cairo_font_list(char ***list, int *count)
{
    char **fonts;
    int num_fonts;

    num_fonts = 1;

    fonts = G_malloc(num_fonts * sizeof(char *));
    fonts[0] = G_store("serif");

    *list = fonts;
    *count = num_fonts;
}

void Cairo_font_info(char ***list, int *count)
{
    char **fonts;
    int num_fonts;

    num_fonts = 1;

    fonts = G_malloc(num_fonts * sizeof(char *));
    fonts[0] = G_store("serif|serif|1||0|utf-8|");

    *list = fonts;
    *count = num_fonts;
}

