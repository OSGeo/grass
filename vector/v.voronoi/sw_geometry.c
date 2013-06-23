#include <stdlib.h>
#include <math.h>
#include "sw_defs.h"

int geominit(void)
{
    double sn;

    freeinit(&efl, sizeof(struct Edge));
    nvertices = 0;
    nedges = 0;
    sn = nsites + 4;
    sqrt_nsites = sqrt(sn);
    deltay = ymax - ymin;
    deltax = xmax - xmin;

    return 0;
}


struct Edge *bisect(struct Site *s1, struct Site *s2)
{
    double dx, dy, adx, ady;
    struct Edge *newedge;

    newedge = (struct Edge *)getfree(&efl);

    newedge->reg[0] = s1;
    newedge->reg[1] = s2;
    ref(s1);
    ref(s2);
    newedge->ep[0] = (struct Site *)NULL;
    newedge->ep[1] = (struct Site *)NULL;

    dx = s2->coord.x - s1->coord.x;
    dy = s2->coord.y - s1->coord.y;
    adx = dx > 0 ? dx : -dx;
    ady = dy > 0 ? dy : -dy;
    newedge->c =
	s1->coord.x * dx + s1->coord.y * dy + (dx * dx + dy * dy) * 0.5;
    if (adx > ady) {
	newedge->a = 1.0;
	newedge->b = dy / dx;
	newedge->c /= dx;
    }
    else {
	newedge->b = 1.0;
	newedge->a = dx / dy;
	newedge->c /= dy;
    }

    newedge->edgenbr = nedges;
    out_bisector(newedge);
    nedges++;
    return (newedge);
}


struct Site *intersect(struct Halfedge *el1, struct Halfedge *el2)
{
    struct Edge *e1, *e2, *e;
    struct Halfedge *el;
    double d, xint, yint;
    int right_of_site;
    struct Site *v;

    e1 = el1->ELedge;
    e2 = el2->ELedge;
    if (e1 == (struct Edge *)NULL || e2 == (struct Edge *)NULL)
	return ((struct Site *)NULL);
    if (e1->reg[1] == e2->reg[1])
	return ((struct Site *)NULL);

    d = e1->a * e2->b - e1->b * e2->a;
    if (-1.0e-10 < d && d < 1.0e-10)
	return ((struct Site *)NULL);

    xint = (e1->c * e2->b - e2->c * e1->b) / d;
    yint = (e2->c * e1->a - e1->c * e2->a) / d;

    if ((e1->reg[1]->coord.y < e2->reg[1]->coord.y) ||
	(e1->reg[1]->coord.y == e2->reg[1]->coord.y &&
	 e1->reg[1]->coord.x < e2->reg[1]->coord.x)) {
	el = el1;
	e = e1;
    }
    else {
	el = el2;
	e = e2;
    }
    right_of_site = xint >= e->reg[1]->coord.x;
    if ((right_of_site && el->ELpm == le) ||
	(!right_of_site && el->ELpm == re))
	return ((struct Site *)NULL);

    v = (struct Site *)getfree(&sfl);
    v->refcnt = 0;
    v->coord.x = xint;
    v->coord.y = yint;
    return (v);
}

/* returns 1 if p is to right of halfedge e */
int right_of(struct Halfedge *el, struct Point *p)
{
    struct Edge *e;
    struct Site *topsite;
    int right_of_site, above, fast;
    double dxp, dyp, dxs, t1, t2, t3, yl;

    e = el->ELedge;
    topsite = e->reg[1];
    right_of_site = p->x > topsite->coord.x;
    if (right_of_site && el->ELpm == le)
	return (1);
    if (!right_of_site && el->ELpm == re)
	return (0);

    if (e->a == 1.0) {
	double xl = e->c - e->b * p->y;
	t1 = p->x - xl;
	t2 = p->y - topsite->coord.y;
	t3 = xl - topsite->coord.x;
	above = t1 * t1 > t2 * t2 + t3 * t3;
#if 0
	if (e->b < 0.0)
	    above = !above;
#endif

#if 0
	dyp = p->y - topsite->coord.y;
	dxp = p->x - topsite->coord.x;
	fast = 0;
	if ((!right_of_site & (e->b < 0.0)) | (right_of_site & (e->b >= 0.0))) {
	    above = dyp >= e->b * dxp;
	    fast = above;
	}
	else {
	    above = p->x + p->y * e->b > e->c;
	    if (e->b < 0.0)
		above = !above;
	    if (!above)
		fast = 1;
	}
	if (!fast) {
	    dxs = topsite->coord.x - (e->reg[0])->coord.x;
	    above = e->b * (dxp * dxp - dyp * dyp) <
		dxs * dyp * (1.0 + 2.0 * dxp / dxs + e->b * e->b);
	    if (e->b < 0.0)
		above = !above;
	}
#endif
    }
    else {			/*e->b==1.0 */
	yl = e->c - e->a * p->x;
	t1 = p->y - yl;
	t2 = p->x - topsite->coord.x;
	t3 = yl - topsite->coord.y;
	above = t1 * t1 > t2 * t2 + t3 * t3;
    }
    return (el->ELpm == le ? above : !above);
}


int endpoint(struct Edge *e, int lr, struct Site *s)
{
    e->ep[lr] = s;
    ref(s);
    if (e->ep[re - lr] == (struct Site *)NULL)
	return -1;
    out_ep(e);
    deref(e->reg[le]);
    deref(e->reg[re]);
    makefree((struct Freenode *)e, &efl);

    return 0;
}

double dist(struct Site *s, struct Site *t)
{
    double dx, dy;

    dx = s->coord.x - t->coord.x;
    dy = s->coord.y - t->coord.y;
    return (sqrt(dx * dx + dy * dy));
}

int makevertex(struct Site *v)
{
    v->sitenbr = nvertices;
    nvertices++;
    out_vertex(v);
    return 0;
}


int deref(struct Site *v)
{
    v->refcnt--;
    if (v->refcnt == 0)
	makefree((struct Freenode *)v, &sfl);
    return 0;
}

int ref(struct Site *v)
{
    v->refcnt++;
    return 0;
}
