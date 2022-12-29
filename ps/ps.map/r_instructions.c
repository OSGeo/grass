#include <string.h>

#include <grass/colors.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

extern FILE *inputfd;
extern int do_mapinfo;
extern int do_vlegend;
extern int ps_copies;

static char *help[] = {
    "cell       rastermap             rast       rastermap",
    "raster     rastermap             group      imagery group",
    "greyrast   greyscale rastermap   grayrast   grayscale rastermap",
    "rgb        3 rastermaps for RGB  setcolor   val_range(s) color",
    "vpoints    vector points map     scalebar   [f|s]",
    "vlines     vector lines map      paper      [a4|a3|us-letter|...]",
    "vareas     vector areas map      maploc     x y [width height]",
    "labels     labelfile             text       east north text",
    "region     regionfile            line       east north east north",
    "grid       spacing               point      east north",
    "geogrid    spacing               header     header text",
    "colortable [y|n]                 vlegend    vector legend",
    "comments   [unix-file]           psfile     PostScript include file",
    "read       unix-file             eps        Encapsulated PostScript file",
    "border     [y|n]                 mapinfo    map information",
    "window     region definition     region     region definition",
    "maskcolor  MASK color",
    "rectangle  east north east north",
    "scale      1:#|# inches|# panels|1 inch = # miles",
    "outline    map composition outline",
    "copies     number of copies",
    ""
};

void read_instructions(int copies_set, int can_reset_scale)
{
    int i;
    int iflag;
    /* name can be fully qualified */
    char name[GNAME_MAX + GMAPSET_MAX], mapset[GMAPSET_MAX];
    char buf[1024];

    iflag = 0;
    
    while (1) {
	char *key;
	char *data;

	if (!input(1, buf, help)) {
	    if (!iflag) {
		/* TODO: check also if instructions are piped in 
		 * through stdin, not interactive */
		if (inputfd != stdin) {
		    while (G_getl2(buf, 12, inputfd)) {
			/* empty lines and comments are fine */
			if (key_data(buf, &key, &data))
			    G_warning(_("Data exist after final 'end' instruction!"));
		    }
		    fclose(inputfd);
		    inputfd = stdin;
		}
		break;
	    }
	    iflag = 0;
	    continue;
	}
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("read")) {
	    if (inputfd != stdin)
		fclose(inputfd);

	    if (sscanf(data, "%s", name) != 1) {
		error(key, data, _("no file specified"));
		inputfd = stdin;
	    }
	    else if ((inputfd = fopen(name, "r")) == NULL) {
		error(key, data, _("unable to open"));
		inputfd = stdin;
	    }
	    else
		iflag = 1;
	    continue;
	}

	if (KEY("paper")) {
	    if (strlen(data) > 0) {
		set_paper(data);
	    }
	    read_paper();

	    continue;
	}
	
	if (KEY("maploc")) {
	    int n;
	    double x, y, w, h;

	    n = sscanf(data, "%lf %lf %lf %lf", &x, &y, &w, &h);
	    if (n == 2 || n == 4) {
		PS.map_x_orig = x;
		PS.map_y_loc = y;
		if (n == 4) {
		    PS.map_width = w;
		    PS.map_height = h;
		}
	    }
	    else {
		error(key, data, _("illegal maploc request"));
		gobble_input();
	    }
	    continue;
	}

	if (KEY("copies")) {
	    int n, copies;

	    if (copies_set)
		continue;
	    n = sscanf(data, "%d", &copies);
	    if (n != 1 || copies < 1 || copies > 20) {
		ps_copies = 1;
		error(key, data, _("illegal copies request"));
	    }
	    ps_copies = copies;
	    continue;
	}

	if (KEY("setcolor")) {
	    int ret, r, g, b;
	    int count;
	    DCELL *val_list;
	    DCELL dmin, dmax;
	    char colorbuf[100];
	    char catsbuf[100];

	    if (PS.cell_fd < 0)
		error(key, data, _("no raster map selected yet"));

	    if (sscanf(data, "%s %[^\n]", catsbuf, colorbuf) == 2) {
		ret = G_str_to_color(colorbuf, &r, &g, &b);
		if (ret != 1)
		    error(key, colorbuf, _("illegal color request")); 

		if (strncmp(catsbuf, "null", 4) == 0) {
		    Rast_set_null_value_color(r, g, b, &(PS.colors));
		    continue;
		}
		if (strncmp(catsbuf, "default", 7) == 0) {
		    Rast_set_default_color(r, g, b, &(PS.colors));
		    continue;
		}
		if ((count = parse_val_list(catsbuf, &val_list)) < 0)
		    error(key, data, _("illegal value list"));

		for (i = 0; i < count; i += 2) {
		    dmin = val_list[i];
		    dmax = val_list[i + 1];
		    Rast_add_d_color_rule(&dmin, r, g, b, &dmax, r, g, b,
					  &(PS.colors));
		}
		G_free(val_list);
	    }
	    continue;
	}

	if (KEY("colortable")) {
	    PS.do_colortable = 0;
	    /*
	       if (PS.cell_fd < 0)
	       error(key, data, "no raster map selected yet");
	       else
	     */
	    PS.do_colortable = yesno(key, data);
	    if (PS.do_colortable)
		read_colortable();
	    continue;
	}

	if (KEY("border")) {
	    PS.do_border = yesno(key, data);
	    if (PS.do_border)
		read_border();
	    continue;
	}

	if (KEY("scalebar")) {
	    if (G_projection() == PROJECTION_LL) {
		error(key, data,
		      _("scalebar is not appropriate for this projection"));
		gobble_input();
	    }
	    PS.do_scalebar = 1;
	    if (sscanf(data, "%s", sb.type) != 1)
		strcpy(sb.type, "f");	/* default to fancy scalebar */
	    read_scalebar();
	    if (sb.length <= 0.) {
		error(key, data, _("Bad scalebar length"));
		gobble_input();
	    }
	    continue;
	}

	if (KEY("text")) {
	    double e, n;
	    char east[50], north[50];
	    char text[1024];

	    if (sscanf(data, "%s %s %[^\n]", east, north, text) == 3
		&& (scan_easting(east, &e) && scan_northing(north, &n)))
		read_text(east, north, text);
	    else {
		gobble_input();
		error(key, data, _("illegal text request"));
	    }
	    continue;
	}

	if (KEY("point")) {
	    double e, n;
	    char east[50], north[50];

	    if (sscanf(data, "%s %s", east, north) == 2
		&& (scan_easting(east, &e) && scan_northing(north, &n)))
		read_point(e, n);
	    else {
		gobble_input();
		error(key, data, _("illegal point request"));
	    }
	    continue;
	}

	if (KEY("eps")) {
	    double e, n;
	    char east[50], north[50];

	    if (sscanf(data, "%s %s", east, north) == 2
		&& (scan_easting(east, &e) && scan_northing(north, &n)))
		read_eps(e, n);
	    else {
		gobble_input();
		error(key, data, _("illegal eps request"));
	    }
	    continue;
	}

	if (KEY("line")) {
	    char east1[50], north1[50];
	    char east2[50], north2[50];
	    double e1, n1, e2, n2;

	    if (sscanf(data, "%s %s %s %s", east1, north1, east2, north2) == 4
		&& (scan_easting(east1, &e1) && scan_easting(east2, &e2)
		    && scan_northing(north1, &n1) &&
		    scan_northing(north2, &n2)))
		read_line(e1, n1, e2, n2);
	    else {
		gobble_input();
		error(key, data, _("illegal line request"));
	    }
	    continue;
	}

	if (KEY("rectangle")) {
	    char east1[50], north1[50];
	    char east2[50], north2[50];
	    double e1, n1, e2, n2;

	    if (sscanf(data, "%s %s %s %s", east1, north1, east2, north2) == 4
		&& (scan_easting(east1, &e1) && scan_easting(east2, &e2)
		    && scan_northing(north1, &n1) &&
		    scan_northing(north2, &n2)))
		read_rectangle(e1, n1, e2, n2);
	    else {
		gobble_input();
		error(key, data, _("illegal rectangle request"));
	    }
	    continue;
	}

	if (KEY("comments")) {
	    switch (sscanf(data, "%s %s", name, mapset)) {
	    case 1:
		read_comment(name);
		break;
	    case 2:
		error(key, data, _("illegal comments request"));
		break;
	    default:
		read_comment("");
		break;
	    }
	    continue;
	}

	if (KEY("scale")) {
	    if (!can_reset_scale)
		continue;
	    if (check_scale(data))
		strcpy(PS.scaletext, data);
	    else {
		PS.scaletext[0] = 0;
		error(key, data, _("illegal scale request"));
	    }
	    continue;
	}

	if (KEY("labels")) {
	    if (scan_gis("paint/labels", "label", key, data, name, mapset, 1))
		read_labels(name, mapset);
	    continue;
	}

	if (KEY("header")) {
	    read_header();
	    PS.do_header = 1;
	    continue;
	}

	if (KEY("mapinfo")) {
	    read_info();
	    do_mapinfo = 1;
	    continue;
	}

	if (KEY("vlegend")) {
	    read_vlegend();
	    do_vlegend = 1;
	    continue;
	}

	if (KEY("outline")) {
	    if (PS.cell_fd < 0) {
		error(key, data, _("no raster map selected yet"));
		gobble_input();
	    }
	    else
		read_outline();
	    continue;
	}

	if (KEY("cell") || KEY("rast") || KEY("raster")) {
	    if (scan_gis("cell", "raster", key, data, name, mapset, 0))
		read_cell(name, mapset);
	    continue;
	}

	if (KEY("greyrast") || KEY("grayrast")) {
	    if (scan_gis("cell", "raster", key, data, name, mapset, 0))
		read_cell(name, mapset);
	    PS.grey = 1;
	    continue;
	}

	if (KEY("group")) {
	    G_strip(data);
	    if (I_find_group(data)) {
		grp.group_name = G_store(data);
		grp.do_group = 1;
		read_group();
	    }
	    else
		error(key, data, _("group not found"));
	    continue;
	}

	if (KEY("rgb")) {
	    G_strip(data);
	    grp.do_group = 1;
	    read_rgb(key, data);
	    continue;
	}

	if (KEY("vpoints")) {
	    if (scan_gis("vector", "vector", key, data, name, mapset, 1))
		read_vpoints(name, mapset);
	    continue;
	}

	if (KEY("vlines")) {
	    if (scan_gis("vector", "vector", key, data, name, mapset, 1))
		read_vlines(name, mapset);
	    continue;
	}

	if (KEY("vareas")) {
	    if (scan_gis("vector", "vector", key, data, name, mapset, 1))
		read_vareas(name, mapset);
	    continue;
	}

	if (KEY("window") || KEY("region")) {
	    if (scan_gis("windows", "region definition", key, data, name,
			 mapset, 1))
		read_wind(name, mapset);
	    continue;
	}

	if (KEY("grid")) {
	    PS.grid = -1;
	    PS.grid_numbers = 0;
	    sscanf(data, "%d", &(PS.grid));
	    if (PS.grid < 0) {
		PS.grid = 0;
		error(key, data, _("illegal grid spacing"));
		gobble_input();
	    }
	    else
		getgrid();
	    continue;
	}

	if (KEY("geogrid")) {
	    if (G_projection() == PROJECTION_XY) {
		error(key, data,
		      _("geogrid is not available for this projection"));
		gobble_input();
	    }
	    /*          if (G_projection() == PROJECTION_LL)
	       G_message(_("geogrid referenced to [???] ellipsoid"));  */
	    PS.geogrid = -1.;
	    PS.geogrid_numbers = 0;
	    sscanf(data, "%d %s", &(PS.geogrid), PS.geogridunit);
	    if (PS.geogrid < 0) {
		PS.geogrid = 0;
		error(key, data, _("illegal geo-grid spacing"));
		gobble_input();
	    }
	    else
		getgeogrid();
	    continue;
	}

	if (KEY("psfile")) {
	    if (PS.num_psfiles >= MAX_PSFILES)
		continue;
	    G_strip(data);
	    PS.psfiles[PS.num_psfiles] = G_store(data);
	    PS.num_psfiles++;
	    continue;
	}

	if (KEY("maskcolor")) {
	    int ret, r, g, b;

	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		PS.mask_r = r / 255.0;
		PS.mask_g = g / 255.0;
		PS.mask_b = b / 255.0;
		PS.mask_color = 1;
		continue;
	    }
	    else if (ret == 2) {	/* none */
		continue;
	    }
	    else {
		error(key, data, _("illegal color request"));
	    }
	}

	if (*key)
	    error(key, "", _("illegal request"));
    }
}
