/*
* $Id$
*
****************************************************************************
*
* MODULE:       d.text.freetype
*
* AUTHOR(S):    Huidae Cho <grass4u@gmail.com>
*
* PURPOSE:      d.text with FreeType2 support
*               http://www.freetype.org
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>

#include <grass/config.h>
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/colors.h>
#include <grass/glocale.h>

#define	DEFAULT_CHARSET		"UTF-8"
#define	DEFAULT_SIZE		"5"
#define	DEFAULT_COLOR		"gray"

#define	DEFAULT_ALIGN		"ll"
#define	DEFAULT_ROTATION	"0"
#define	DEFAULT_LINESPACING	"1.1"

/* font size conversion */
#define	cnv(x)			((int)(0.8*64*(x)))

#define	deinit()		{if(face)				\
					FT_Done_Face(face);		\
				if(library)				\
					FT_Done_FreeType(library);	\
				if(driver)				\
					R_close_driver();}

#define	error(msg)		{deinit(); G_fatal_error(msg);}


typedef struct	{
	char	*font, *path, *charset;
} capinfo;

typedef	struct	{
	int	t, b, l, r;
} rectinfo;

static int	read_capfile(char *, capinfo **, int *, int *, char **);
static int	find_font(capinfo *, int, char *);
static char	*transform_string(char *, int (*)(int));
static int	convert_text(char *, char *, unsigned char **);
static int	get_coordinates(rectinfo, char **, char, char, double *,
			double *, int *, int *);
static void	set_color(const char *);

static int	set_font(FT_Library, FT_Face *, char *);
static void	get_dimension(FT_Face, unsigned char *, int, FT_Vector *);
static void	get_ll_coordinates(FT_Face, unsigned char *, int, char *,
			double, FT_Vector *);
static void	set_matrix(FT_Matrix *, double);
static int	draw_character(rectinfo, FT_Face, FT_Matrix *, FT_Vector *, 
			int);
static void	draw_text(rectinfo, FT_Face, FT_Vector *, unsigned char *, int,
			double);


int
main(int argc, char **argv)
{
	struct	GModule	*module;
	struct
	{
		struct	Option	*text;
		struct	Option	*at;
		struct	Option	*font;
		struct	Option	*path;
		struct	Option	*charset;
		struct	Option	*color;
		struct	Option	*size;
		struct	Option	*align;
		struct	Option	*rotation;
		struct	Option	*linespacing;
	} param;

	struct
	{
		struct	Flag	*b;
		struct	Flag	*r;
		struct	Flag	*p;
		struct	Flag	*g;
		struct	Flag	*s;
		struct	Flag	*c;
	} flag;

	capinfo	*fonts;
	int	fonts_count;
	int	cur_font;
	char	*font_names;

	FT_Library	library = NULL;
	FT_Face		face = NULL;
	FT_Vector	pen, pen2;

	int	driver = 0;
	char	win_name[64];
	rectinfo	win;
	char	*text, *path, *charset, *tcolor;
	int	bold;
	double	east, north, size, rotation, linespacing;
	int	i, l, ol, x, y;
	unsigned char	*out;
	char	buf[512];


	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("display");
    module->description =
	    _("Draws text in the graphics monitor's active display frame using TrueType fonts.");

	param.text = G_define_option();
	param.text->key         = "text";
	param.text->type        = TYPE_STRING;
	param.text->required    = NO;
	param.text->description = _("Text to display");

	param.at = G_define_option();
	param.at->key         = "at";
	param.at->type        = TYPE_DOUBLE;
	param.at->required    = NO;
	param.at->key_desc    = "x,y";
	param.at->description = _("Screen position (percentage, [0,0] is bottom left)");

	read_capfile(getenv("GRASS_FREETYPECAP"), &fonts, &fonts_count,
			&cur_font, &font_names);

	param.font = NULL;
	if(fonts_count)
	{
		param.font = G_define_option();
		param.font->key         = "font";
		param.font->type        = TYPE_STRING;
		param.font->required    = NO;
		if(cur_font >= 0)
			param.font->answer      = fonts[cur_font].font;
		param.font->options     = font_names;
		param.font->description = _("Font name");
	}

	param.path = G_define_option();
	param.path->key         = "path";
	param.path->type        = TYPE_STRING;
	param.path->required    = NO;
	param.path->description = _("Path to TrueType font (including file name)");
	param.path->gisprompt   = "old_file,file,font";

	param.charset = G_define_option();
	param.charset->key         = "charset";
	param.charset->type        = TYPE_STRING;
	param.charset->required    = NO;
	param.charset->description = "Character encoding (default: "DEFAULT_CHARSET")";

	param.color = G_define_option();
	param.color->key         = "color";
	param.color->type        = TYPE_STRING;
	param.color->required    = NO;
	param.color->answer      = DEFAULT_COLOR;
	param.color->description =
	    _("Text color, either a standard GRASS color or R:G:B triplet");
	param.color->gisprompt   = GISPROMPT_COLOR;

	param.size = G_define_option();
	param.size->key         = "size";
	param.size->type        = TYPE_DOUBLE;
	param.size->required    = NO;
	param.size->answer      = DEFAULT_SIZE;
	param.size->description =
	    _("Height of letters (in percent of available frame height)");

	param.align = G_define_option();
	param.align->key         = "align";
	param.align->type        = TYPE_STRING;
	param.align->required    = NO;
	param.align->answer      = DEFAULT_ALIGN;
	param.align->options     = "ll,lc,lr,cl,cc,cr,ul,uc,ur";
	param.align->description = _("Text alignment");

	param.rotation = G_define_option();
	param.rotation->key         = "rotation";
	param.rotation->type        = TYPE_DOUBLE;
	param.rotation->required    = NO;
	param.rotation->answer      = DEFAULT_ROTATION;
	param.rotation->description = _("Rotation angle in degrees (counter-clockwise)");

	param.linespacing = G_define_option();
	param.linespacing->key         = "linespacing";
	param.linespacing->type        = TYPE_DOUBLE;
	param.linespacing->required    = NO;
	param.linespacing->answer      = DEFAULT_LINESPACING;
	param.linespacing->description = _("Line spacing");

	flag.p = G_define_flag();
	flag.p->key         = 'p';
	flag.p->description = _("Screen position in pixels ([0,0] is top left)");

	flag.g = G_define_flag();
	flag.g->key         = 'g';
	flag.g->description =_( "Screen position in geographic coordinates");

	flag.b = G_define_flag();
	flag.b->key         = 'b';
	flag.b->description = _("Use bold text");

	flag.r = G_define_flag();
	flag.r->key         = 'r';
	flag.r->description = _("Use radians instead of degrees for rotation");

	flag.s = G_define_flag();
	flag.s->key         = 's';
	flag.s->description = _("Font size is height in pixels");

	flag.c = G_define_flag();
	flag.c->key         = 'c';
	flag.c->description = _("Command mode (Compatibility with d.text)");


	if(G_parser(argc, argv))
		exit(1);

	if(!param.text->answer && !flag.c->answer)
		G_fatal_error(_("Either text or -c should be given"));

	text = param.text->answer;

	if(flag.p->answer && flag.g->answer)
		G_fatal_error(_("Choose only one coordinate system for placement"));

	if(!flag.c->answer && !param.path->answer) {
	    if(fonts_count) {
		if(!param.font->answer)
		    G_fatal_error(_("No font selected"));
	    }
	    else G_fatal_error(_("No font selected"));
	}

	path = NULL;
	charset = NULL;

	if(param.font && param.font->answer)
	{
		cur_font = find_font(fonts, fonts_count, param.font->answer);
		if(cur_font < 0)
			G_fatal_error(_("Invalid font: %s"), param.font->answer);

		path = fonts[cur_font].path;
		charset = transform_string(fonts[cur_font].charset, toupper);
	}

	if(param.path->answer)
		path = param.path->answer;
	if(param.charset->answer)
		charset = transform_string(param.charset->answer, toupper);

	tcolor = transform_string(param.color->answer, tolower);
	size = atof(param.size->answer);

	if(!charset)
		charset = DEFAULT_CHARSET;

	bold = flag.b->answer;

	if(!flag.c->answer)
	    G_debug(1, "Font=<%s:%s>", path, charset);

	rotation = atof(param.rotation->answer);
	if(!flag.r->answer)
		rotation *= M_PI / 180.0;

	rotation = fmod(rotation, 2 * M_PI);
	if(rotation < 0.0)
		rotation += 2 * M_PI;

	linespacing = atof(param.linespacing->answer);

	if(R_open_driver() != 0)
		error(_("No graphics device selected"));
	driver = 1;

	D_setup(0);

	if(D_get_cur_wind(win_name))
		error(_("No current window"));
	if(D_set_cur_wind(win_name))
		error(_("Current window not available"));

	D_get_screen_window(&win.t, &win.b, &win.l, &win.r);
	R_set_window(win.t, win.b, win.l, win.r);

	if(!flag.s->answer)
		size *= (double)(win.b-win.t)/100.0;

	if(FT_Init_FreeType(&library))
		error(_("Unable to initialise FreeType"));

	if(path)
	{
		if(set_font(library, &face, path))
			error(_("Unable to create face"));

		if(FT_Set_Char_Size(face, cnv(size), cnv(size), 100, 100))
			error(_("Unable to set size"));
	}

	set_color(tcolor);

	if(!flag.c->answer)
	{
		if(get_coordinates(win, param.at->answers,
					flag.p->answer, flag.g->answer,
					&east, &north, &x, &y))
		{
			deinit();
			exit(0);
		}

		ol = convert_text(charset, text, &out);
		if(ol == -1)
			error(_("Unable to create text conversion context"));
		if(ol == -2)
			error(_("Text conversion error"));

		pen.x = x;
		pen.y = y;
		get_ll_coordinates(face, out, ol,
				param.align->answer, rotation, &pen);
		pen2 = pen;
		draw_text(win, face, &pen, out, ol, rotation);

		if(bold){
			pen.x = pen2.x + 64 * cos(rotation);
			pen.y = pen2.y - 64 * sin(rotation);
			pen2 = pen;
			draw_text(win, face, &pen, out, ol, rotation);

			pen.x = pen2.x - 64 * sin(rotation);
			pen.y = pen2.y - 64 * cos(rotation);
			draw_text(win, face, &pen, out, ol, rotation);
		}

		if(param.at->answer)
			D_add_to_list(G_recreate_command());
		else{
			sprintf(buf, "%s at=%f,%f",
					G_recreate_command(), east, north);
			D_add_to_list(buf);
		}
	}
	else
	{
		char	*tmpfile, *p, *c, align[3], linefeed,
			setx, sety, setl;
		FILE	*fp;
		int	sx, sy, px, py;
		double	d, dd;

		sx = win.l;
		sy = win.t;

		linefeed = 1;
		setx = sety = setl = 0;

		if(param.at->answer)
		{
			if(get_coordinates(win, param.at->answers,
					flag.p->answer, flag.g->answer,
					&east, &north, &x, &y))
			{
				deinit();
				exit(0);
			}
			sx = x;
			sy = y;
		}
		else
		{
			east  = D_d_to_u_col((double)sx);
			north = D_d_to_u_row((double)sy);
		}
		strncpy(align, param.align->answer, 2);

		x = px = sx;
		y = py = sy;

		if(isatty(0))
			fprintf(stdout, "\nPlease enter text instructions.  Enter EOF (ctrl-d) on last line to quit\n");

		tmpfile = G_tempfile();
		if(!(fp = fopen(tmpfile, "w")))
			error(_("Unable to write the temporary file"));

		while(fgets(buf, 512, stdin))
		{
			fprintf(fp, "%s", buf);

			l = strlen(buf);
			buf[l-1] = 0;

			if(buf[0] == '.' && buf[1] != '.')
			{
				G_squeeze(buf);
				if(!buf[1])
					continue;
				for(p = buf + 2; *p == ' '; p++);
				l = strlen(p);
				switch(buf[1])
				{
					case 'F':
						if((c = strchr(p, ':')))
							*c = 0;
						if(*p != '/')
						{
							if(!fonts_count)
							{
								G_warning(_("No predefined font"));
								break;
							}
							cur_font = find_font(fonts, fonts_count, p);
							if(cur_font < 0)
							{
								G_warning(_("Invalid font: %s"), p);
								break;
							}
							path = fonts[cur_font].path;
							charset = transform_string(fonts[cur_font].charset, toupper);
						}
						else
						{
							path = p;
							if(access(path, R_OK))
							{
								G_warning(_("%s: Unable to read font"), p);
								break;
							}
						}
						if(c)
							charset = transform_string(c+1, toupper);
						if(set_font(library, &face, path))
							error(_("Unable to create face"));
						if(FT_Set_Char_Size(face, cnv(size), cnv(size), 100, 100))
							error(_("Unable to set size"));
						break;
					case 'C':
						tcolor = transform_string(p, tolower);
						set_color(tcolor);
						break;
					case 'S':
						i = 0;
						if(strchr("+-", p[0]))
							i = 1;
						d = atof(p);
						if(p[l-1] != 'p')
							d *= (double)(win.b-win.t)/100.0;
						size = d + (i ? size : 0);
						if(face && FT_Set_Char_Size(face, cnv(size), cnv(size), 100, 100))
							error(_("Unable to set size"));
						break;
					case 'B':
						bold = (atoi(p) ? 1 : 0);
						break;
					case 'A':
						strncpy(align, p, 2);
						break;
					case 'R':
						i = 0;
						if(strchr("+-", p[0]))
							i = 1;
						dd = atof(p);
						if(p[l-1] != 'r')
							dd *= M_PI / 180.0;
						dd += (i ? rotation : 0.0);
						rotation = fmod(dd, 2 * M_PI);
						if(rotation < 0.0)
							rotation += 2 * M_PI;
						break;
					case 'I':
						linespacing = atof(p);
						break;
					case 'X':
						setx = 1;
						i = 0;
						if(strchr("+-", p[0]))
							i = 1;
						d = atof(p);
						if(p[l-1] == '%')
							d = d/100.0*(double)(win.r-win.l);
						else
						if(p[l-1] != 'p')
						{
							setx = 0;
							d *= size;
						}
						x = d + (i ? x : sx);
						if(!setx)
							px = x;
						break;
					case 'Y':
						sety = 1;
						i = 0;
						if(strchr("+-", p[0]))
							i = 1;
						d = atof(p);
						if(p[l-1] == '%')
							d *= (double)(win.b-win.t)/100.0;
						else
						if(p[l-1] != 'p')
						{
							sety = 0;
							d *= size;
						}
						y = d + (i ? y : sy);
						if(!sety)
							py = y;
						break;
					case 'L':
						setl = 1;
						linefeed = (atoi(p) ? 1 : 0);
						break;
					case 'E':
						i = 0;
						if(strchr("+-", p[0]))
							i = 1;
						dd = atof(p);
						if(p[l-1] == '%')
							dd *= dd/100.0 * (double)(win.r-win.l);
						else
						if(p[l-1] != 'p')
							dd = D_u_to_d_col(dd);
						x = px = sx = (int)dd + (i ? sx : win.l);
						break;
					case 'N':
						i = 0;
						if(strchr("+-", p[0]))
							i = 1;
						dd = atof(p);
						if(p[l-1] == '%')
							dd *= (double)(win.b-win.t)/100.0;
						else
						if(p[l-1] != 'p')
							dd = D_u_to_d_row(dd);
						y = py = sy = (int)dd + (i ? sy : win.t);
						break;
				}
			}
			else
			if(face)
			{
				i = 0;
				if(buf[0] == '.' && buf[1] == '.')
					i = 1;

				ol = convert_text(charset, buf + i, &out);
				if(ol == -1)
					error(_("Unable to create text conversion context"));
				if(ol == -2)
					error(_("Text conversion error"));

				if(linefeed || setl)
				{
					if(!setx)
						x = px + size * linespacing * sin(rotation);
					if(!sety)
						y = py + size * linespacing * cos(rotation);
					px = x;
					py = y;
				}

				pen.x = x;
				pen.y = y;
				get_ll_coordinates(face, out, ol,
						align, rotation, &pen);
				pen2 = pen;
				draw_text(win, face, &pen, out, ol, rotation);

				if(bold){
					pen.x = pen2.x + 64 * cos(rotation);
					pen.y = pen2.y - 64 * sin(rotation);
					pen2 = pen;
					draw_text(win, face, &pen, out, ol, rotation);

					pen.x = pen2.x - 64 * sin(rotation);
					pen.y = pen2.y - 64 * cos(rotation);
					draw_text(win, face, &pen, out, ol, rotation);
				}

				if(!linefeed)
				{
					x = pen.x / 64;
					y = - pen.y / 64;
				}

				setx = sety = setl = 0;
			}
			else
				G_warning(_("No font selected"));
		}

		fclose(fp);

		l = strlen(G_recreate_command()) + 4 + strlen(tmpfile);
		p = (char *) G_malloc(l);

		sprintf(p, "%s < %s", G_recreate_command(), tmpfile);

		D_add_to_list(p);
		G_free(p);
	}

	deinit();

	exit(0);
}

static int
read_capfile(char *capfile, capinfo **fonts, int *fonts_count, int *cur_font,
		char **font_names)
{
	char	file[4096], *ptr;
	int	i, font_names_size = 0;
	char	buf[4096], ifont[128], ipath[4096], icharset[32];
	FILE	*fp;

	*fonts = NULL;
	*fonts_count = 0;
	if(cur_font)
		*cur_font = -1;
	if(font_names)
		*font_names = NULL;

	ptr = file;
	sprintf(file, "%s/etc/freetypecap", G_gisbase());
	if(capfile)
	{
		if(access(capfile, R_OK))
			G_warning(_("%s: Unable to read FreeType definition file; use the default"), capfile);
		else
			ptr = capfile;
	}
	if(ptr == file && access(ptr, R_OK))
	{
		G_warning(_("%s: No FreeType definition file"), ptr);
		return -1;
	}
	if(!(fp = fopen(ptr, "r")))
	{
		G_warning(_("%s: Unable to read FreeType definition file"), ptr);
		return -1;
	}

	while(fgets(buf, sizeof(buf), fp) && !feof(fp))
	{
		capinfo *font;
		int offset;
		char *p;

		p = strchr(buf, '#');
		if(p)
			*p = 0;

		if(sscanf(buf, "%[^:]:%[^:]:%[^:]", ifont,ipath,icharset) != 3)
			continue;

		if(access(ipath, R_OK))
			continue;

		*fonts = (capinfo *)
			G_realloc(*fonts, (*fonts_count + 1) * sizeof(capinfo));

		font = &((*fonts)[*fonts_count]);

		offset = (ifont[0] == '*') ? 1 : 0;

		if(cur_font && offset > 0 && *cur_font < 0)
			*cur_font = *fonts_count;

		font->font    = G_store(ifont + offset);
		font->path    = G_store(ipath);
		font->charset = G_store(icharset);

		(*fonts_count)++;
	}

	fclose(fp);

	if(!font_names)
		return 0;

	font_names_size = 0;
	for(i = 0; i < *fonts_count; i++)
		font_names_size += strlen((*fonts)[i].font) + 1;

	*font_names = (char *) G_malloc(font_names_size);
	(*font_names)[0] = '\0';
	for(i = 0; i < *fonts_count; i++)
	{
		if(i > 0)
			strcat(*font_names, ",");
		strcat(*font_names, (*fonts)[i].font);
	}

	return 0;
}

static int
find_font(capinfo *fonts, int fonts_count, char *name)
{
	int i;

	for(i = 0; i < fonts_count; i++)
		if(strcasecmp(fonts[i].font, name) == 0)
			return i;

	return -1;
}

static char *
transform_string(char *str, int (*func)(int))
{
	int i;
	char *result;

	result = G_store(str);

	for(i = 0; result[i]; i++)
		result[i] = (*func)(result[i]);

	return result;
}

static int
convert_text(char *charset, char *text, unsigned char **out)
{
	size_t	l, i, ol;
	char	*p1;
	unsigned char	*p2;
#ifdef HAVE_ICONV_H
	iconv_t	cd;
#endif

	l = strlen(text);

	ol = 4 * (l + 1);
	*out = G_malloc(ol);

#ifdef HAVE_ICONV_H
	p1 = text;
	p2 = *out;
	i = ol;
	if((cd = iconv_open("UCS-4", charset)) < 0)
		return -1;

	if(iconv(cd, (char **)&p1, &l, (char **)&p2, &i) < 0)
		return -2;

	iconv_close(cd);
	ol -= i;
#else
	p2 = *out;
	for(i = 0; i <= l; i++)
	{
		p2[4*i+0] = 0;
		p2[4*i+1] = 0;
		p2[4*i+2] = 0;
		p2[4*i+3] = text[i];
	}
	ol = l * 4;
#endif

	return (int) ol;
}

static int
get_coordinates(rectinfo win, char **ans, char pixel, char geocoor,
		double *east, double *north, int *x, int *y)
{
	int	i;
	double	e, n;

	if(ans)
	{
		e = atof(ans[0]);
		n = atof(ans[1]);
		if(pixel)
		{
			*x = e + win.l;
			*y = n + win.t;
			e = D_d_to_u_col((double)*x);
			n = D_d_to_u_row((double)*y);
		}
		else if(geocoor)
		{
			*x = (int)D_u_to_d_col(e);
			*y = (int)D_u_to_d_row(n);
		}
		else
		{
			*x = win.l+(int)((win.r-win.l)*e/100.);
			*y = win.t+(int)((win.b-win.t)*(100.-n)/100.);
			e = D_d_to_u_col((double)*x);
			n = D_d_to_u_row((double)*y);
		}
		
	}
	else
	{
		fprintf(stderr, "Click!\n");
		fprintf(stderr, " Left:    Place text here\n");
		fprintf(stderr, " Right:   Quit\n");

		R_get_location_with_pointer(x, y, &i);
		i &= 0x0f;
		if(i != 1)
			return 1;
		e = D_d_to_u_col((double)*x);
		n = D_d_to_u_row((double)*y);
	}

	if(east)
		*east = e;
	if(north)
		*north = n;

	return 0;
}

static void
set_color(const char *tcolor)
{
	int r, g, b;

	if(sscanf(tcolor, "%d:%d:%d", &r, &g, &b) == 3)
	{
		if (r>=0 && r<256 && g>=0 && g<256 && b>=0 && b<256)
			R_RGB_color(r, g, b);
	}
#define BACKWARDS_COMPATIBLE
#ifdef BACKWARDS_COMPATIBLE
	else if(sscanf(tcolor, "0x%02x%02x%02x", &r, &g, &b) == 3)
		R_RGB_color(r, g, b);
#endif
	else
	{
		int n = D_translate_color(tcolor);

		if (!n)
		{
			G_warning(_("[%s]: No such color"), tcolor);
			n = D_translate_color(DEFAULT_COLOR);
		}

		R_standard_color(n);
	}
}

static int
set_font(FT_Library library, FT_Face *face, char *path)
{
	if(*face)
		FT_Done_Face(*face);
	if(FT_New_Face(library, path, 0, face))
		return -1;

	return 0;
}

static void
get_dimension(FT_Face face, unsigned char *out, int l, FT_Vector *dim)
{
	int	i, first = 1, minx, maxx, miny, maxy, ch;
	FT_Matrix	matrix;
	FT_Vector	pen;
	FT_GlyphSlot	slot = face->glyph;

	set_matrix(&matrix, 0);

	pen.x = 0;
	pen.y = 0;

	for(i = 0; i < l; i += 4)
	{
		ch = (out[i+2] << 8) | out[i+3];

		FT_Set_Transform(face, &matrix, &pen);

		if(FT_Load_Char(face, ch, FT_LOAD_NO_BITMAP))
			continue;
		if(FT_Render_Glyph(slot, ft_render_mode_mono))
		{
			/* FT_Render_Glyph fails for spaces */
			pen.x += slot->advance.x;
			pen.y += slot->advance.y;
			continue;
		}

		if(first)
		{
			first = 0;
			minx = slot->bitmap_left;
			maxx = minx + slot->bitmap.width;
			miny = - slot->bitmap_top;
			maxy = miny + slot->bitmap.rows;
		}
		else
		{
			if(minx > slot->bitmap_left)
				minx = slot->bitmap_left;
			if(maxx < slot->bitmap_left +
					slot->bitmap.width)
				maxx = slot->bitmap_left +
					slot->bitmap.width;
			if(miny > - slot->bitmap_top)
				miny = - slot->bitmap_top;
			if(maxy < - slot->bitmap_top +
					slot->bitmap.rows)
				maxy = - slot->bitmap_top +
					slot->bitmap.rows;
		}

		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	dim->x = maxx - minx;
	dim->y = maxy - miny;

	return;
}

static void
get_ll_coordinates(FT_Face face, unsigned char *out, int l,
		char *align, double rotation, FT_Vector *pen)
{
	FT_Vector	dim;

	if(strcmp(align, "ll"))
	{
		get_dimension(face, out, l, &dim);

		switch(align[0])
		{
			case 'l':
				break;
			case 'c':
				pen->x += dim.y / 2.0 * sin(rotation);
				pen->y += dim.y / 2.0 * cos(rotation);
				break;
			case 'u':
				pen->x += dim.y * sin(rotation);
				pen->y += dim.y * cos(rotation);
				break;
		}
	
		switch(align[1])
		{
			case 'l':
				break;
			case 'c':
				pen->x -= dim.x / 2.0 * cos(rotation);
				pen->y += dim.x / 2.0 * sin(rotation);
				break;
			case 'r':
				pen->x -= dim.x * cos(rotation);
				pen->y += dim.x * sin(rotation);
				break;
		}
	}

	pen->x *= 64;
	pen->y = - pen->y * 64;

	return;
}

static void
set_matrix(FT_Matrix *matrix, double rotation)
{
	matrix->xx = (FT_Fixed)( cos(rotation)*0x10000);
	matrix->xy = (FT_Fixed)(-sin(rotation)*0x10000);
	matrix->yx = (FT_Fixed)( sin(rotation)*0x10000);
	matrix->yy = (FT_Fixed)( cos(rotation)*0x10000);

	return;
}

static int
draw_character(rectinfo win, FT_Face face, FT_Matrix *matrix, FT_Vector *pen,
		int ch)
{
	static char *buffer;
	static int nalloc;
	int	i, j, l, start_row, start_col, rows, width, w, h;
	rectinfo	rect;
	FT_GlyphSlot	slot = face->glyph;

	FT_Set_Transform(face, matrix, pen);

	if(FT_Load_Char(face, ch, FT_LOAD_NO_BITMAP))
		return -1;
	if(FT_Render_Glyph(slot, ft_render_mode_normal))
	{
		/* FT_Render_Glyph fails for spaces */
		pen->x += slot->advance.x;
		pen->y += slot->advance.y;
		return -2;
	}

	rows  = slot->bitmap.rows;
	width = slot->bitmap.width;

	rect.t = - slot->bitmap_top;
	rect.b = rect.t + rows;
	rect.l = slot->bitmap_left;
	rect.r = rect.l + width;

	pen->x += slot->advance.x;
	pen->y += slot->advance.y;

	if (rows <= 0 || width <= 0)
		return 0;

	if (rect.t > win.b || rect.b < win.t || rect.l > win.r || rect.r < win.l)
		return 0;

	start_row = 0;
	start_col = 0;
	w = width;
	h = rows;

	if (rect.t < win.t)
	{
		start_row = win.t - rect.t;
		h -= start_row;
	}
	if (rect.b > win.b)
		h -= rect.b - win.b;

	if (rect.l < win.l)
	{
		start_col = win.l - rect.l;
		w -= start_col;
	}
	if (rect.r > win.r)
		w -= rect.r - win.r;

	R_move_abs(rect.l + start_col, rect.t + start_row);

	l = w * h;
	if (nalloc <= l)
	{
		nalloc = l;
		buffer = G_realloc(buffer, nalloc);
	}

	for (j = 0; j < h; j++)
	{
		int y = start_row + j;
		for (i = 0; i < w; i++)
		{
			int x = start_col + i;
			buffer[j * w + i] = slot->bitmap.buffer[y * slot->bitmap.pitch + x];
		}
	}

	R_bitmap(w, h, 128, buffer);

	return 0;
}

static void
draw_text(rectinfo win, FT_Face face, FT_Vector *pen,
		unsigned char *out, int l, double rotation)
{
	int	i, ch;
	FT_Matrix	matrix;

	set_matrix(&matrix, rotation);

	for(i = 0; i < l; i += 4)
	{
		ch = (out[i+2] << 8) | out[i+3];
		draw_character(win, face, &matrix, pen, ch);
	}

	return;
}

