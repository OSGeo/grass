/* calcsd.c                                                             */
#include "ransurf.h"
#include "local_proto.h"

void CalcSD(void)
{
    int Row, Col, DoFilter;
    double Effect;

    G_debug(2, "CalcSD()");

    FilterSD = 0.0;
    for (DoFilter = 0; DoFilter < NumFilters; DoFilter++) {
	CopyFilter(&Filter, AllFilters[DoFilter]);
	if (Filter.Mult < 0.0)
	    Filter.Mult *= -1.0;

	MakeBigF();
	for (Row = 0; Row < BigF.NumR; Row++) {
	    for (Col = 0; Col < BigF.NumC; Col++) {
		DistDecay(&Effect, Row - BigF.RowPlus, Col - BigF.ColPlus);
		FilterSD += Effect * Effect;
	    }
	}
    }

    G_debug(3, "(FilterSD):%.12lf", FilterSD);
    FilterSD = sqrt(FilterSD);
    G_debug(3, "(FilterSD):%.12lf", FilterSD);

    return;
}
