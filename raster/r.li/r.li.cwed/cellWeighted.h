/*
 *   AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include "../r.li.daemon/GenericCell.h"

typedef struct CoppiaPesata
{
    generic_cell c1;
    generic_cell c2;
    double d;
    long e;
} CoppiaPesata;

typedef CoppiaPesata *Coppie;

int addCoppia(Coppie * cc, generic_cell c1, generic_cell c2, double p,
	      long tc, long *siz);
int updateCoppia(Coppie * cc, generic_cell c1, generic_cell c2, long tc);
