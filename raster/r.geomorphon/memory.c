#include "local_proto.h"

int open_map(MAPS * rast)
{

    int row, col;
    char *mapset;
    struct Cell_head cellhd;
    void *tmp_buf;

    mapset = (char *)G_find_raster2(rast->elevname, "");

    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), rast->elevname);

    rast->fd = Rast_open_old(rast->elevname, mapset);
    Rast_get_cellhd(rast->elevname, mapset, &cellhd);
    rast->raster_type = Rast_map_type(rast->elevname, mapset);

    if (window.ew_res < cellhd.ew_res || window.ns_res < cellhd.ns_res)
	G_warning(_("Region resolution shoudn't be lesser than map %s resolution. Run g.region raster=%s to set proper resolution"),
		  rast->elevname, rast->elevname);

    tmp_buf = Rast_allocate_buf(rast->raster_type);
    rast->elev = (FCELL **) G_malloc((row_buffer_size + 1) * sizeof(FCELL *));

    for (row = 0; row < row_buffer_size + 1; ++row) {
	rast->elev[row] = Rast_allocate_buf(FCELL_TYPE);
	Rast_get_row(rast->fd, tmp_buf, row, rast->raster_type);
	for (col = 0; col < ncols; ++col)
	    get_cell(col, rast->elev[row], tmp_buf, rast->raster_type);
    }				/* end elev */

    G_free(tmp_buf);
    return 0;
}

int get_cell(int col, float *buf_row, void *buf, RASTER_MAP_TYPE raster_type)
{

    switch (raster_type) {

    case CELL_TYPE:
	if (Rast_is_null_value(&((CELL *) buf)[col], CELL_TYPE))
	    Rast_set_f_null_value(&buf_row[col], 1);
	else
	    buf_row[col] = (FCELL) ((CELL *) buf)[col];
	break;

    case FCELL_TYPE:
	if (Rast_is_null_value(&((FCELL *) buf)[col], FCELL_TYPE))
	    Rast_set_f_null_value(&buf_row[col], 1);
	else
	    buf_row[col] = (FCELL) ((FCELL *) buf)[col];
	break;

    case DCELL_TYPE:
	if (Rast_is_null_value(&((DCELL *) buf)[col], DCELL_TYPE))
	    Rast_set_f_null_value(&buf_row[col], 1);
	else
	    buf_row[col] = (FCELL) ((DCELL *) buf)[col];
	break;
    }

    return 0;
}

int shift_buffers(int row)
{
    int i;
    int col;
    void *tmp_buf;
    FCELL *tmp_elev_buf;

    tmp_buf = Rast_allocate_buf(elevation.raster_type);
    tmp_elev_buf = elevation.elev[0];

    for (i = 1; i < row_buffer_size + 1; ++i)
	elevation.elev[i - 1] = elevation.elev[i];

    elevation.elev[row_buffer_size] = tmp_elev_buf;
    Rast_get_row(elevation.fd, tmp_buf, row + row_radius_size + 1,
		 elevation.raster_type);

    for (col = 0; col < ncols; ++col)
	get_cell(col, elevation.elev[row_buffer_size], tmp_buf,
		 elevation.raster_type);

    G_free(tmp_buf);
    return 0;
}

int free_map(FCELL ** map, int n)
{
    int i;

    for (i = 0; i < n; ++i)
	G_free(map[i]);
    G_free(map);
    return 0;
}

int write_form_cat_colors(char *raster, CATCOLORS * ccolors)
{
    struct Colors colors;
    struct Categories cats;
    int i;

    Rast_init_colors(&colors);

    for (i = 1; i < CNT; ++i)
	Rast_add_color_rule(&ccolors[i].cat, ccolors[i].r, ccolors[i].g,
			    ccolors[i].b, &ccolors[i].cat, ccolors[i].r,
			    ccolors[i].g, ccolors[i].b, &colors, CELL_TYPE);
    Rast_write_colors(raster, G_mapset(), &colors);
    Rast_free_colors(&colors);
    Rast_init_cats("Forms", &cats);
    for (i = 1; i < CNT; ++i)
	Rast_set_cat(&ccolors[i].cat, &ccolors[i].cat, ccolors[i].label,
		     &cats, CELL_TYPE);
    Rast_write_cats(raster, &cats);
    Rast_free_cats(&cats);
    return 0;
}

int write_contrast_colors(char *raster)
{
    struct Colors colors;
    /* struct Categories cats; */

    FCOLORS fcolors[9] = {	/* colors for positive openness */
	{-2500, 0, 0, 50, NULL},
	{-100, 0, 0, 56, NULL},
	{-15, 0, 56, 128, NULL},
	{-3, 0, 128, 255, NULL},
	{0, 255, 255, 255, NULL},
	{3, 255, 128, 0, NULL},
	{15, 128, 56, 0, NULL},
	{100, 56, 0, 0, NULL},
	{2500, 50, 0, 0, NULL}
    };
    int i;

    Rast_init_colors(&colors);

    for (i = 0; i < 8; ++i)
	Rast_add_d_color_rule(&fcolors[i].cat, fcolors[i].r, fcolors[i].g,
			      fcolors[i].b, &fcolors[i + 1].cat,
			      fcolors[i + 1].r, fcolors[i + 1].g,
			      fcolors[i + 1].b, &colors);
    Rast_write_colors(raster, G_mapset(), &colors);
    Rast_free_colors(&colors);
    /*
       Rast_init_cats("Forms", &cats);
       for(i=0;i<8;++i)
       Rast_set_cat(&ccolors[i].cat, &ccolors[i].cat, ccolors[i].label, &cats, CELL_TYPE);
       Rast_write_cats(raster, &cats);
       Rast_free_cats(&cats);
     */
    return 0;
}
