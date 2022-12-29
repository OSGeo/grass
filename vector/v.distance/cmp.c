#include "local_proto.h"

int cmp_near(const void *pa, const void *pb)
{
    NEAR *p1 = (NEAR *) pa;
    NEAR *p2 = (NEAR *) pb;

    if (p1->from_cat < p2->from_cat)
	return -1;
    if (p1->from_cat > p2->from_cat)
	return 1;
    return 0;
}

int cmp_near_to(const void *pa, const void *pb)
{
    NEAR *p1 = (NEAR *) pa;
    NEAR *p2 = (NEAR *) pb;

    if (p1->from_cat < p2->from_cat)
	return -1;

    if (p1->from_cat > p2->from_cat)
	return 1;

    if (p1->to_cat < p2->to_cat)
	return -1;

    if (p1->to_cat > p2->to_cat)
	return 1;

    return 0;
}


int cmp_exist(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}
