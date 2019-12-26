#include "bouman.h"
#include "region.h"

int levels_reg(struct Region *region)
{
    int D;
    int wd, ht;
    struct Region region_buff;

    /* save region structure */
    copy_reg(region, &region_buff);

    D = 0;
    reg_to_wdht(region, &wd, &ht);
    while ((wd > 2) && (ht > 2)) {
	D++;
	dec_reg(region);
	reg_to_wdht(region, &wd, &ht);
    }

    /* restore region structure */
    copy_reg(&region_buff, region);

    return (D);
}

void dec_reg(struct Region *region)
{
    region->xmin = region->xmin / 2;
    region->xmax = region->xmax / 2;
    region->ymin = region->ymin / 2;
    region->ymax = region->ymax / 2;
}

void copy_reg(struct Region *region1, struct Region *region2)
{
    region2->xmin = region1->xmin;
    region2->xmax = region1->xmax;
    region2->ymin = region1->ymin;
    region2->ymax = region1->ymax;

    region2->free.left = region1->free.left;
    region2->free.right = region1->free.right;
    region2->free.top = region1->free.top;
    region2->free.bottom = region1->free.bottom;
}

void reg_to_wdht(struct Region *region, int *wd, int *ht)
{
    *wd = region->xmax - region->xmin;
    *ht = region->ymax - region->ymin;
}
