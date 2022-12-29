
/****************************************************************************
 *
 * MODULE:       r.li.edgedensity
 * AUTHOR(S):    Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *               Commission from Faunalia Pontedera (PI) www.faunalia.it
 *               Rewrite: Markus Metz
 *
 * PURPOSE:      calculates edge density index
 * COPYRIGHT:    (C) 2006-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <fcntl.h>		/* for O_RDONLY usage */
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "../r.li.daemon/avlDefs.h"
#include "../r.li.daemon/avl.h"
#include "../r.li.daemon/daemon.h"

rli_func edgedensity;
int calculate(int fd, struct area_entry *ad, char **par, double *result);
int calculateD(int fd, struct area_entry *ad, char **par, double *result);
int calculateF(int fd, struct area_entry *ad, char **par, double *result);

static int brdr = 1;

int main(int argc, char *argv[])
{
    struct Option *raster, *conf, *output, *class;
    struct Flag *flag_brdr;
    struct GModule *module;
    char **par = NULL;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description =
	_("Calculates edge density index on a raster map, using a 4 neighbour algorithm");
    G_add_keyword(_("raster"));
    G_add_keyword(_("landscape structure analysis"));
    G_add_keyword(_("patch index"));

    /* define options */

    raster = G_define_standard_option(G_OPT_R_INPUT);

    conf = G_define_standard_option(G_OPT_F_INPUT);
    conf->key = "config";
    conf->description = _("Configuration file");
    conf->required = YES;

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    class = G_define_option();
    class->key = "patch_type";
    class->type = TYPE_STRING;
    class->required = NO;
    class->multiple = NO;
    class->label = _("The value of the patch type");
    class->description = _("It can be integer, double or float; "
			   "it will be changed in function of map type");

    flag_brdr = G_define_flag();
    flag_brdr->key = 'b';
    flag_brdr->description = _("Exclude border edges");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (class->answer == NULL)
	par = NULL;
    else
	par = &class->answer;

    brdr = flag_brdr->answer == 0;

    return calculateIndex(conf->answer, edgedensity, par, raster->answer,
			  output->answer);
}

int edgedensity(int fd, char **par, struct area_entry *ad, double *result)
{
    int ris = -1;
    double indice = 0;

    switch (ad->data_type) {
    case CELL_TYPE:
	{
	    ris = calculate(fd, ad, par, &indice);
	    break;
	}
    case DCELL_TYPE:
	{
	    ris = calculateD(fd, ad, par, &indice);
	    break;
	}
    case FCELL_TYPE:
	{
	    ris = calculateF(fd, ad, par, &indice);
	    break;
	}
    default:
	{
	    G_fatal_error("data type unknown");
	    return RLI_ERRORE;
	}
    }
    if (ris != RLI_OK) {
	return RLI_ERRORE;
    }

    *result = indice;

    return RLI_OK;
}


int calculate(int fd, struct area_entry *ad, char **par, double *result)
{
    CELL *buf, *buf_sup, *buf_null;
    CELL corrCell, precCell, supCell;
    CELL ptype;
    long nedges, area; 
    int i, j;
    int mask_fd, *mask_buf, *mask_sup, *mask_tmp, masked;
    struct Cell_head hd;

    Rast_get_window(&hd);

    /* open mask if needed */
    mask_fd = -1;
    mask_buf = mask_sup = NULL;
    masked = FALSE;
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	for (j = 0; j < ad->cl; j++)
	    mask_buf[j] = 0;

	masked = TRUE;
    }

    buf_null = Rast_allocate_c_buf();
    if (buf_null == NULL) {
	G_fatal_error("malloc buf_null failed");
	return RLI_ERRORE;
    }

    /* the first time buf_sup is all null */
    Rast_set_c_null_value(buf_null, Rast_window_cols());
    buf_sup = buf_null;

    if (par != NULL) {	/* only 1 class */
	char *sval;

	sval = par[0];
	ptype = atoi(sval);
    }
    else
	Rast_set_c_null_value(&ptype, 1);

    nedges = 0;
    area = 0;

    /* for each raster row */
    for (i = 0; i < ad->rl; i++) {

	/* read row of raster */
	buf = RLI_get_cell_raster_row(fd, i + ad->y, ad);

	if (i > 0)		/* not first row */
	    buf_sup = RLI_get_cell_raster_row(fd, i - 1 + ad->y, ad);

	/* read mask if needed */
	if (masked) {
	    mask_tmp = mask_sup;
	    mask_sup = mask_buf;
	    mask_buf = mask_tmp;
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return RLI_ERRORE;
	}

	Rast_set_c_null_value(&precCell, 1);

	for (j = 0; j < ad->cl; j++) {
	    corrCell = buf[j + ad->x];

	    if (masked && mask_buf[j] == 0) {
		Rast_set_c_null_value(&corrCell, 1);
	    }

	    supCell = buf_sup[j + ad->x];
	    if (masked && (mask_sup[j] == 0)) {
		Rast_set_c_null_value(&supCell, 1);
	    }

	    if (brdr) {
		if (!Rast_is_c_null_value(&corrCell)) {
		    area++; 
		    if (Rast_is_c_null_value(&ptype) || corrCell == ptype) {
			if (Rast_is_c_null_value(&precCell) || precCell != corrCell) {
			    nedges++;
			}
			if (Rast_is_c_null_value(&supCell) || supCell != corrCell) {
			    nedges++;
			}
			/* right and bottom */
			if (i == ad->rl - 1)
			    nedges++;
			if (j == ad->cl - 1)
			    nedges++;
		    }
		}
		else /* corrCell == NULL */ {
		    if (!Rast_is_c_null_value(&precCell)) {
			if (Rast_is_c_null_value(&ptype) || precCell == ptype) {
			    nedges++;
			}
		    }
		    if (!Rast_is_c_null_value(&supCell)) {
			if (Rast_is_c_null_value(&ptype) || supCell == ptype) {
			    nedges++;
			}
		    }
		}
	    }
	    else {
		/* exclude border edges */
		if (!Rast_is_c_null_value(&corrCell)) {
		    area++; 
		    if (Rast_is_c_null_value(&ptype) || corrCell == ptype) {
			if (j > 0 && !(masked && mask_buf[j - 1] == 0) &&
			    (Rast_is_c_null_value(&precCell) || precCell != corrCell)) {
			    nedges++;
			}
			if (i > 0 && !(masked && mask_sup[i - 1] == 0) &&
			    (Rast_is_c_null_value(&supCell) || supCell != corrCell)) {
			    nedges++;
			}
		    }
		}
		else if (Rast_is_c_null_value(&corrCell) && !(masked && mask_buf[j] == 0)) {
		    if (!Rast_is_c_null_value(&precCell)) {
			if (Rast_is_c_null_value(&ptype) || precCell == ptype) {
			    nedges++;
			}
		    }
		    if (!Rast_is_c_null_value(&supCell)) {
			if (Rast_is_c_null_value(&ptype) || supCell == ptype) {
			    nedges++;
			}
		    }
		}
	    }
	    precCell = corrCell;
	}
    }

    /* calculate index */
    if (area > 0) {
	double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
	double elength, cell_size;

	/* calculate distance */
	G_begin_distance_calculations();
	/* EW Dist at North edge */
	EW_DIST1 = G_distance(hd.east, hd.north, hd.west, hd.north);
	/* EW Dist at South Edge */
	EW_DIST2 = G_distance(hd.east, hd.south, hd.west, hd.south);
	/* NS Dist at East edge */
	NS_DIST1 = G_distance(hd.east, hd.north, hd.east, hd.south);
	/* NS Dist at West edge */
	NS_DIST2 = G_distance(hd.west, hd.north, hd.west, hd.south);

	elength = ((EW_DIST1 + EW_DIST2) / (2 * hd.cols) + 
	          (NS_DIST1 + NS_DIST2) / (2  * hd.rows)) / 2;

	cell_size = ((EW_DIST1 + EW_DIST2) / (2 * hd.cols)) *
	            ((NS_DIST1 + NS_DIST2) / (2 * hd.rows));

	*result = (double) nedges * elength * 10000 / (area * cell_size);
    }
    else
	Rast_set_d_null_value(result, 1);

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
	G_free(mask_sup);
    }
    G_free(buf_null);

    return RLI_OK;
}

int calculateD(int fd, struct area_entry *ad, char **par, double *result)
{
    DCELL *buf, *buf_sup, *buf_null;
    DCELL corrCell, precCell, supCell;
    DCELL ptype;
    long nedges, area; 
    int i, j;
    int mask_fd, *mask_buf, *mask_sup, *mask_tmp, masked;
    struct Cell_head hd;

    Rast_get_window(&hd);

    /* open mask if needed */
    mask_fd = -1;
    mask_buf = mask_sup = NULL;
    masked = FALSE;
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	for (j = 0; j < ad->cl; j++)
	    mask_buf[j] = 0;

	masked = TRUE;
    }

    buf_null = Rast_allocate_d_buf();
    if (buf_null == NULL) {
	G_fatal_error("malloc buf_null failed");
	return RLI_ERRORE;
    }

    /* the first time buf_sup is all null */
    Rast_set_d_null_value(buf_null, Rast_window_cols());
    buf_sup = buf_null;

    if (par != NULL) {	/* only 1 class */
	char *sval;

	sval = par[0];
	ptype = atof(sval);
    }
    else
	Rast_set_d_null_value(&ptype, 1);

    nedges = 0;
    area = 0;

    /* for each raster row */
    for (i = 0; i < ad->rl; i++) {

	/* read row of raster */
	buf = RLI_get_dcell_raster_row(fd, i + ad->y, ad);

	if (i > 0)		/* not first row */
	    buf_sup = RLI_get_dcell_raster_row(fd, i - 1 + ad->y, ad);

	/* read mask if needed */
	if (masked) {
	    mask_tmp = mask_sup;
	    mask_sup = mask_buf;
	    mask_buf = mask_tmp;
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return RLI_ERRORE;
	}

	Rast_set_d_null_value(&precCell, 1);

	for (j = 0; j < ad->cl; j++) {
	    corrCell = buf[j + ad->x];

	    if (masked && mask_buf[j] == 0) {
		Rast_set_d_null_value(&corrCell, 1);
	    }

	    supCell = buf_sup[j + ad->x];
	    if (masked && (mask_sup[j] == 0)) {
		Rast_set_d_null_value(&supCell, 1);
	    }

	    if (brdr) {
		if (!Rast_is_d_null_value(&corrCell)) {
		    area++; 
		    if (Rast_is_d_null_value(&ptype) || corrCell == ptype) {
			if (Rast_is_d_null_value(&precCell) || precCell != corrCell) {
			    nedges++;
			}
			if (Rast_is_d_null_value(&supCell) || supCell != corrCell) {
			    nedges++;
			}
			/* right and bottom */
			if (i == ad->rl - 1)
			    nedges++;
			if (j == ad->cl - 1)
			    nedges++;
		    }
		}
		else /* corrCell == NULL */ {
		    if (!Rast_is_d_null_value(&precCell)) {
			if (Rast_is_d_null_value(&ptype) || precCell == ptype) {
			    nedges++;
			}
		    }
		    if (!Rast_is_d_null_value(&supCell)) {
			if (Rast_is_d_null_value(&ptype) || supCell == ptype) {
			    nedges++;
			}
		    }
		}
	    }
	    else {
		/* exclude border edges */
		if (!Rast_is_d_null_value(&corrCell)) {
		    area++; 
		    if (Rast_is_d_null_value(&ptype) || corrCell == ptype) {
			if (j > 0 && !(masked && mask_buf[j - 1] == 0) &&
			    (Rast_is_d_null_value(&precCell) || precCell != corrCell)) {
			    nedges++;
			}
			if (i > 0 && !(masked && mask_sup[i - 1] == 0) &&
			    (Rast_is_d_null_value(&supCell) || supCell != corrCell)) {
			    nedges++;
			}
		    }
		}
		else if (Rast_is_d_null_value(&corrCell) && !(masked && mask_buf[j] == 0)) {
		    if (!Rast_is_d_null_value(&precCell)) {
			if (Rast_is_d_null_value(&ptype) || precCell == ptype) {
			    nedges++;
			}
		    }
		    if (!Rast_is_d_null_value(&supCell)) {
			if (Rast_is_d_null_value(&ptype) || supCell == ptype) {
			    nedges++;
			}
		    }
		}
	    }
	    precCell = corrCell;
	}
    }

    /* calculate index */
    if (area > 0) {
	double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
	double elength, cell_size;

	/* calculate distance */
	G_begin_distance_calculations();
	/* EW Dist at North edge */
	EW_DIST1 = G_distance(hd.east, hd.north, hd.west, hd.north);
	/* EW Dist at South Edge */
	EW_DIST2 = G_distance(hd.east, hd.south, hd.west, hd.south);
	/* NS Dist at East edge */
	NS_DIST1 = G_distance(hd.east, hd.north, hd.east, hd.south);
	/* NS Dist at West edge */
	NS_DIST2 = G_distance(hd.west, hd.north, hd.west, hd.south);

	elength = ((EW_DIST1 + EW_DIST2) / (2 * hd.cols) + 
	          (NS_DIST1 + NS_DIST2) / (2  * hd.rows)) / 2;

	cell_size = ((EW_DIST1 + EW_DIST2) / (2 * hd.cols)) *
	            ((NS_DIST1 + NS_DIST2) / (2 * hd.rows));

	*result = (double) nedges * elength * 10000 / (area * cell_size);
    }
    else
	Rast_set_d_null_value(result, 1);

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
	G_free(mask_sup);
    }
    G_free(buf_null);

    return RLI_OK;
}

int calculateF(int fd, struct area_entry *ad, char **par, double *result)
{
    FCELL *buf, *buf_sup, *buf_null;
    FCELL corrCell, precCell, supCell;
    FCELL ptype;
    long nedges, area; 
    int i, j;
    int mask_fd, *mask_buf, *mask_sup, *mask_tmp, masked;
    struct Cell_head hd;

    Rast_get_window(&hd);

    /* open mask if needed */
    mask_fd = -1;
    mask_buf = mask_sup = NULL;
    masked = FALSE;
    if (ad->mask == 1) {
	if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
	    return RLI_ERRORE;
	mask_buf = G_malloc(ad->cl * sizeof(int));
	if (mask_buf == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	mask_sup = G_malloc(ad->cl * sizeof(int));
	if (mask_sup == NULL) {
	    G_fatal_error("malloc mask_buf failed");
	    return RLI_ERRORE;
	}
	for (j = 0; j < ad->cl; j++)
	    mask_buf[j] = 0;

	masked = TRUE;
    }

    buf_null = Rast_allocate_f_buf();
    if (buf_null == NULL) {
	G_fatal_error("malloc buf_null failed");
	return RLI_ERRORE;
    }

    /* the first time buf_sup is all null */
    Rast_set_f_null_value(buf_null, Rast_window_cols());
    buf_sup = buf_null;

    if (par != NULL) {	/* only 1 class */
	char *sval;

	sval = par[0];
	ptype = atof(sval);
    }
    else
	Rast_set_f_null_value(&ptype, 1);

    nedges = 0;
    area = 0;

    /* for each raster row */
    for (i = 0; i < ad->rl; i++) {

	/* read row of raster */
	buf = RLI_get_fcell_raster_row(fd, i + ad->y, ad);

	if (i > 0)		/* not first row */
	    buf_sup = RLI_get_fcell_raster_row(fd, i - 1 + ad->y, ad);

	/* read mask if needed */
	if (masked) {
	    mask_tmp = mask_sup;
	    mask_sup = mask_buf;
	    mask_buf = mask_tmp;
	    if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
		return RLI_ERRORE;
	}

	Rast_set_f_null_value(&precCell, 1);

	for (j = 0; j < ad->cl; j++) {
	    corrCell = buf[j + ad->x];

	    if (masked && mask_buf[j] == 0) {
		Rast_set_f_null_value(&corrCell, 1);
	    }

	    supCell = buf_sup[j + ad->x];
	    if (masked && (mask_sup[j] == 0)) {
		Rast_set_f_null_value(&supCell, 1);
	    }

	    if (brdr) {
		if (!Rast_is_f_null_value(&corrCell)) {
		    area++; 
		    if (Rast_is_f_null_value(&ptype) || corrCell == ptype) {
			if (Rast_is_f_null_value(&precCell) || precCell != corrCell) {
			    nedges++;
			}
			if (Rast_is_f_null_value(&supCell) || supCell != corrCell) {
			    nedges++;
			}
			/* right and bottom */
			if (i == ad->rl - 1)
			    nedges++;
			if (j == ad->cl - 1)
			    nedges++;
		    }
		}
		else /* corrCell == NULL */ {
		    if (!Rast_is_f_null_value(&precCell)) {
			if (Rast_is_f_null_value(&ptype) || precCell == ptype) {
			    nedges++;
			}
		    }
		    if (!Rast_is_f_null_value(&supCell)) {
			if (Rast_is_f_null_value(&ptype) || supCell == ptype) {
			    nedges++;
			}
		    }
		}
	    }
	    else {
		/* exclude border edges */
		if (!Rast_is_f_null_value(&corrCell)) {
		    area++; 
		    if (Rast_is_f_null_value(&ptype) || corrCell == ptype) {
			if (j > 0 && !(masked && mask_buf[j - 1] == 0) &&
			    (Rast_is_f_null_value(&precCell) || precCell != corrCell)) {
			    nedges++;
			}
			if (i > 0 && !(masked && mask_sup[i - 1] == 0) &&
			    (Rast_is_f_null_value(&supCell) || supCell != corrCell)) {
			    nedges++;
			}
		    }
		}
		else if (Rast_is_f_null_value(&corrCell) && !(masked && mask_buf[j] == 0)) {
		    if (!Rast_is_f_null_value(&precCell)) {
			if (Rast_is_f_null_value(&ptype) || precCell == ptype) {
			    nedges++;
			}
		    }
		    if (!Rast_is_f_null_value(&supCell)) {
			if (Rast_is_f_null_value(&ptype) || supCell == ptype) {
			    nedges++;
			}
		    }
		}
	    }
	    precCell = corrCell;
	}
    }

    /* calculate index */
    if (area > 0) {
	double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
	double elength, cell_size;

	/* calculate distance */
	G_begin_distance_calculations();
	/* EW Dist at North edge */
	EW_DIST1 = G_distance(hd.east, hd.north, hd.west, hd.north);
	/* EW Dist at South Edge */
	EW_DIST2 = G_distance(hd.east, hd.south, hd.west, hd.south);
	/* NS Dist at East edge */
	NS_DIST1 = G_distance(hd.east, hd.north, hd.east, hd.south);
	/* NS Dist at West edge */
	NS_DIST2 = G_distance(hd.west, hd.north, hd.west, hd.south);

	elength = ((EW_DIST1 + EW_DIST2) / (2 * hd.cols) + 
	          (NS_DIST1 + NS_DIST2) / (2  * hd.rows)) / 2;

	cell_size = ((EW_DIST1 + EW_DIST2) / (2 * hd.cols)) *
	            ((NS_DIST1 + NS_DIST2) / (2 * hd.rows));

	*result = (double) nedges * elength * 10000 / (area * cell_size);
    }
    else
	Rast_set_d_null_value(result, 1);

    if (masked) {
	close(mask_fd);
	G_free(mask_buf);
	G_free(mask_sup);
    }
    G_free(buf_null);

    return RLI_OK;
}
