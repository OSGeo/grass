#ifndef _ROWCOL_H
#define _ROWCOL_H

/* these defines work with modeling coordinates only */

/* TODO: avoid integer overflow */

/* view resolutions */
#define VXRES(gs)   (gs->x_mod * gs->xres)
#define VYRES(gs)   (gs->y_mod * gs->yres)

/* number of viewres rows/cols */
#define VROWS(gs)    (int)((gs->rows -1)/gs->y_mod)
#define VCOLS(gs)    (int)((gs->cols -1)/gs->x_mod)

/* data row & col to offset */
#define DRC2OFF(gs, drow, dcol)  (int)((dcol) + (drow) * gs->cols)

/* ycoord/xcoord to data row/col */
#define Y2DROW(gs,py)      (int)((gs->yrange - (py))/gs->yres)
#define X2DCOL(gs,px)      (int)((px)/gs->xres)

/* ycoord/xcoord to offset */
#define XY2OFF(gs, px, py) (int)DRC2OFF(gs, Y2DROW(gs,py), X2DCOL(gs,px))

/* ycoord/xcoord to viewres row/col */
#define Y2VROW(gs,py)      (int)((gs->yrange - (py))/(gs->yres * gs->y_mod))
#define X2VCOL(gs,px)      (int)((px)/(gs->xres * gs->x_mod))

/* viewres row/col to data row/col */
#define VROW2DROW(gs,vrow)      (int)(gs->y_mod * (vrow))
#define VCOL2DCOL(gs,vcol)      (int)(gs->x_mod * (vcol))

/* data row/col to ycoord/xcoord */
#define DROW2Y(gs,drow)      (gs->yrange - ((drow) * gs->yres))
#define DCOL2X(gs,dcol)      ((dcol) * gs->xres)

/* viewres row/col to ycoord/xcoord */
#define VROW2Y(gs,vrow)      (gs->yrange - ((vrow) * gs->yres * gs->y_mod))
#define VCOL2X(gs,vcol)      ((vcol) * gs->xres * gs->x_mod)

#endif /* _ROWCOL_H */
