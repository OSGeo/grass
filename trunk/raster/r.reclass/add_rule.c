#include "rule.h"

int add_rule(RULE ** tail, CELL lo, CELL hi, CELL new)
{
    RULE *r;

    r = (RULE *) G_malloc(sizeof(RULE));

    r->lo = lo;
    r->hi = hi;
    r->new = new;
    r->next = NULL;
    if (*tail)
	(*tail)->next = r;
    *tail = r;

    return 0;
}
