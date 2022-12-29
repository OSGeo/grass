#include "Gwater.h"

int haf_basin_side(int updir, int downdir, int thisdir)
{
    int newup, newthis;

    newup = updir - downdir;
    if (newup < 0)
	newup += 8;
    newthis = thisdir - downdir;
    if (newthis < 0)
	newthis += 8;
    if (newthis < newup)
	return (LEFT);
    if (newthis > newup)
	return (RITE);
    return (NEITHER);
}
