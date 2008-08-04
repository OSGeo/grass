/* calcsd.c                                                             */

#undef TRACE
#undef DEBUG

#undef MAIN
#include "ransurf.h"
#include "local_proto.h"


void CalcSD(void)
{
    int Row, Col, DoFilter;
    double Effect;

    FUNCTION(CalcSD);

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

    DOUBLE(FilterSD);
    FilterSD = sqrt(FilterSD);
    DOUBLE(FilterSD);
    RETURN;
}
