/**
 * \file wr_cellhd.c
 *
 * \brief Write Cell Header functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <stdio.h>
#include <grass/gis.h>


/**
 * \fn int G__write_Cell_head (FILE *fd, struct Cell_head *cellhd, int is_cellhd)
 *
 * \brief Write cell header or window.
 *
 * \param[in,out] fd
 * \param[in] cellhd
 * \param[in] is_cellhd 1 cell header; 0 window
 * \return always returns 1
 */

int G__write_Cell_head ( FILE *fd,
    const struct Cell_head *cellhd,int is_cellhd)
{
    char buf[1024];
    int fmt;

    fmt = cellhd->proj;

    fprintf (fd, "proj:       %d\n",  cellhd->proj);
    fprintf (fd, "zone:       %d\n",  cellhd->zone);

    G_format_northing (cellhd->north,buf,fmt);
    fprintf (fd, "north:      %s\n", buf);

    G_format_northing (cellhd->south,buf,fmt);
    fprintf (fd, "south:      %s\n", buf);

    G_format_easting (cellhd->east,buf,fmt);
    fprintf (fd, "east:       %s\n", buf);

    G_format_easting (cellhd->west,buf,fmt);
    fprintf (fd, "west:       %s\n", buf);

    fprintf (fd, "cols:       %d\n", cellhd->cols);
    fprintf (fd, "rows:       %d\n", cellhd->rows);

    G_format_resolution (cellhd->ew_res,buf,fmt);
    fprintf (fd, "e-w resol:  %s\n", buf);

    G_format_resolution (cellhd->ns_res,buf,fmt);
    fprintf (fd, "n-s resol:  %s\n", buf);

    if (is_cellhd)
    {
	fprintf(fd,"format:     %d\n",  cellhd->format);
	fprintf(fd,"compressed: %d\n",  cellhd->compressed);
    }

    return 1;
}


/**
 * \fn int G__write_Cell_head3 (FILE *fd, struct Cell_head *cellhd, int is_cellhd)
 *
 * \brief Write 3D cell header or window.
 *
 * \param[in,out] fd
 * \param[in] cellhd
 * \param[in] is_cellhd 1 cell header; 0 window
 * \return always returns 1
 */

int G__write_Cell_head3 ( FILE *fd,
    const struct Cell_head *cellhd,int is_cellhd)
{
    char buf[1024];
    int fmt;

    fmt = cellhd->proj;

    G__write_Cell_head ( fd, cellhd, is_cellhd );

    fprintf (fd, "top:        %g\n", cellhd->top);
    fprintf (fd, "bottom:     %g\n", cellhd->bottom);

    fprintf (fd, "cols3:      %d\n", cellhd->cols3);
    fprintf (fd, "rows3:      %d\n", cellhd->rows3);
    fprintf (fd, "depths:     %d\n", cellhd->depths);

    G_format_resolution (cellhd->ew_res3,buf,fmt);
    fprintf (fd, "e-w resol3: %s\n", buf);

    G_format_resolution (cellhd->ns_res3,buf,fmt);
    fprintf (fd, "n-s resol3: %s\n", buf);

    G_format_resolution (cellhd->tb_res,buf,fmt);
    fprintf (fd, "t-b resol:  %s\n", buf);

    return 1;
}
