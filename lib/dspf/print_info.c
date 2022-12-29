#include "viz.h"

int print_head_info(file_info * head)
{
    fprintf(stderr, "xdim = %d ydim = %d zdim = %d\n",
	    head->xdim, head->ydim, head->zdim);
    fprintf(stderr, " n = %f s = %f e = %f w = %f\n",
	    head->north, head->south, head->east, head->west);
    fprintf(stderr, "t = %f b = %f\n", head->top, head->bottom);
    fprintf(stderr, "ns_res = %f ew_res = %f tb_res = %f\n",
	    head->ns_res, head->ew_res, head->tb_res);
    fprintf(stderr, "min = %f max = %f \n", head->min, head->max);

    return 0;
}
