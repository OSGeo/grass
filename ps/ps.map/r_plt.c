#include <string.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define KEY(x)(strcmp(key,x)==0)

int read_point(double e, double n)
{
    char buf[1024], symb[1024];
    int r, g, b;
    int color_R, color_G, color_B;
    int fcolor_R, fcolor_G, fcolor_B;
    int ret;
    double size, width, rotate;
    /* int have_icon; */ /* unused */
    char ch;
    char *key, *data;
    int masked;

    static char *help[] = {
	"color  color",
	"fcolor fill color",
	"symbol group/symbol",
	"size   #",
	"width  #",
	"rotate #",
	"masked [y|n]",
	""
    };

    size = 6.0;
    width = -1.0;  /* default is proportionate to symbol size */
    rotate = 0.0;
    masked = 0;
    color_R = color_G = color_B = 0;
    fcolor_R = fcolor_G = fcolor_B = 128;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("masked")) {
	    masked = yesno(key, data);
	    if (masked)
		PS.mask_needed = 1;
	    continue;
	}
	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		color_R = r;
		color_G = g;
		color_B = b;
	    }
	    else if (ret == 2)	/* i.e. "none" */
		color_R = color_G = color_B = -1;
	    else
		error(key, data, "illegal color request");

	    continue;
	}
	if (KEY("fcolor")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		fcolor_R = r;
		fcolor_G = g;
		fcolor_B = b;
	    }
	    else if (ret == 2)	/* i.e. "none" */
		fcolor_R = fcolor_G = fcolor_B = -1;
	    else
		error(key, data, "illegal color request");

	    continue;
	}
	if (KEY("symbol")) {
	    /* TODO: check if exists */
	    strcpy(symb, data);
	    continue;
	}

	if (KEY("size")) {
	    if (sscanf(data, "%lf", &size) != 1 || size <= 0.0) {
		size = 1.0;
		error(key, data, "illegal size request");
	    }
	    continue;
	}

	if (KEY("width")) {
	   ch = ' ';
	   if (sscanf(data, "%lf%c", &width, &ch) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal width request");
	   }
	   if (ch == 'i')
	       width = width * 72.;
	   continue;
	}

	if (KEY("rotate")) {
	    if (sscanf(data, "%lf", &rotate) != 1) {
		rotate = 0.0;
		error(key, data, "illegal rotate request");
	    }
	    continue;
	}

	error(key, data, "illegal point request");
    }

    sprintf(buf, "P %d %f %f %d %d %d %d %d %d %f %f %s %.2f", masked, e, n,
	    color_R, color_G, color_B, fcolor_R, fcolor_G, fcolor_B, size,
	    rotate, symb, width);

    add_to_plfile(buf);

    return 0;
}

int read_eps(double e, double n)
{
    char buf[1024], eps_file[GPATH_MAX];
    char *eps;
    double scale, rotate;
    int have_eps;
    char *key, *data;
    int masked;
    FILE *fp;

    static char *help[] = {
	"epsfile EPS file",
	"scale   #",
	"rotate   #",
	"masked [y|n]",
	""
    };

    scale = 1.0;
    rotate = 0.0;
    have_eps = 0;
    masked = 0;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("masked")) {
	    masked = yesno(key, data);
	    if (masked)
		PS.mask_needed = 1;
	    continue;
	}

	if (KEY("epsfile")) {
	    G_chop(data);

	    /* expand "$GISBASE" if present */
	    if (strncmp(data, "$GISBASE", 8) != 0)
		strcpy(eps_file, data);
	    else {
		strcpy(eps_file, G_gisbase());
		data += 8;
		strcat(eps_file, data);
	    }

	    eps = G_store(eps_file);

	    /* test if file is accessible */
	    if ((fp = fopen(eps, "r")) == NULL)
		error(key, data, _("Can't open eps file"));

	    have_eps = 1;
	    fclose(fp);
	    continue;
	}

	if (KEY("scale")) {
	    if (sscanf(data, "%lf", &scale) != 1 || scale <= 0.0) {
		scale = 1.0;
		error(key, data, "illegal scale request");
	    }
	    continue;
	}

	if (KEY("rotate")) {
	    if (sscanf(data, "%lf", &rotate) != 1) {
		rotate = 0.0;
		error(key, data, "illegal rotate request");
	    }
	    continue;
	}

	error(key, data, "illegal eps request");
    }
    if (have_eps) {
	sprintf(buf, "E %d %f %f %f %f %s", masked, e, n, scale, rotate, eps);
    }
    add_to_plfile(buf);

    return 0;
}

int read_line(double e1, double n1, double e2, double n2)
{
    char buf[300];
    int r, g, b;
    int color_R, color_G, color_B;
    int ret;
    double width;
    int masked;
    char ch, *key, *data;

    static char *help[] = {
	"color  color",
	"width  #",
	"masked [y|n]",
	""
    };

    width = 1.;
    color_R = color_G = color_B = 0;
    masked = 0;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("masked")) {
	    masked = yesno(key, data);
	    if (masked)
		PS.mask_needed = 1;
	    continue;
	}

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		color_R = r;
		color_G = g;
		color_B = b;
	    }
	    else if (ret == 2)	/* i.e. "none" */
		color_R = color_G = color_B = -1;
	    else
		error(key, data, "illegal color request");

	    continue;
	}

	if (KEY("width")) {
	    ch = ' ';
	    if (sscanf(data, "%lf%c", &width, &ch) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal width request");
	    }
	    if (ch == 'i')
		width = width * 72.;
	    continue;
	}

	error(key, data, "illegal line request");
    }

    sprintf(buf, "L %d %f %f %f %f %d %d %d %.2f",
	    masked, e1, n1, e2, n2, color_R, color_G, color_B, width);

    add_to_plfile(buf);

    return 0;
}

int read_rectangle(double e1, double n1, double e2, double n2)
{
    char buf[300];
    int r, g, b;
    int color_R, color_G, color_B;
    int fcolor_R, fcolor_G, fcolor_B;
    int ret;
    double width;
    int masked;
    char ch, *key, *data;

    static char *help[] = {
	"color  color",
	"fcolor fill color",
	"width  #",
	"masked [y|n]",
	""
    };

    width = 1.;
    masked = 0;
    color_R = color_G = color_B = 0;
    fcolor_R = fcolor_G = fcolor_B = -1;  /* not filled by default */

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("masked")) {
	    masked = yesno(key, data);
	    if (masked)
		PS.mask_needed = 1;
	    continue;
	}

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		color_R = r;
		color_G = g;
		color_B = b;
	    }
	    else if (ret == 2)	/* i.e. "none" */
		color_R = color_G = color_B = -1;
	    else
		error(key, data, "illegal color request");

	    continue;
	}

	if (KEY("fcolor")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		fcolor_R = r;
		fcolor_G = g;
		fcolor_B = b;
	    }
	    else if (ret == 2)	/* i.e. "none" */
		fcolor_R = fcolor_G = fcolor_B = -1;
	    else
		error(key, data, "illegal color request");

	    continue;
	}

	if (KEY("width")) {
	    ch = ' ';
	    if (sscanf(data, "%lf%c", &width, &ch) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal width request");
	    }
	    if (ch == 'i')
		width = width * 72.;
	    continue;
	}

	error(key, data, "illegal rectangle request");
    }

    sprintf(buf, "R %d %f %f %f %f %d %d %d %d %d %d %.2f",
	    masked, e1, n1, e2, n2, color_R, color_G, color_B,
	    fcolor_R, fcolor_G, fcolor_B, width);

    add_to_plfile(buf);

    return 0;
}

int add_to_plfile(char *buf)
{
    FILE *fd;

    if (PS.plfile == NULL) {
	PS.plfile = G_tempfile();
	fd = fopen(PS.plfile, "w");
    }
    else
	fd = fopen(PS.plfile, "a");
    if (fd != NULL) {
	fprintf(fd, "%s\n", buf);
	fclose(fd);
    }
    else
	error("point/line file", "", "can't open");

    return 0;
}
