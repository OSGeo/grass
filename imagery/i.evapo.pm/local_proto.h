/* function.c */
#include <grass/raster.h>
DCELL calc_Delta		(DCELL T);
DCELL calc_g			(DCELL Z);
DCELL calc_Eo			(DCELL T);
DCELL calc_Ea			(DCELL Eo,DCELL RH);
DCELL calc_G			(DCELL Rn, int t);
DCELL calc_ETp			(DCELL T,DCELL Z,DCELL u2,DCELL Rn,int day,DCELL Rh,DCELL hc);
DCELL calc_openwaterETp	        (DCELL T,DCELL Z,DCELL u2,DCELL Rn,int day,DCELL Rh,DCELL hc);
