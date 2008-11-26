#include  "data_types.h"
#include  "memory.h"
#include  "edge.h"

/* 
 *  Construct an edge from vertices v1, v2 and add it to rings of edges e1, e2
 */
struct edge *join(struct edge *e1, struct vertex *v1,
		  struct edge *e2, struct vertex *v2, side s)
{

    struct edge *new_edge;

    /* v1, v2 - vertices to be joined.
       e1, e2 - edges to which v1, v2 belong to */

    new_edge = create_edge(v1, v2);

    if (s == left) {
	if (ORG(e1) == v1)
	    splice(OPREV(e1), new_edge, v1);
	else
	    splice(DPREV(e1), new_edge, v1);
	splice(e2, new_edge, v2);
    }
    else {
	splice(e1, new_edge, v1);
	if (ORG(e2) == v2)
	    splice(OPREV(e2), new_edge, v2);
	else
	    splice(DPREV(e2), new_edge, v2);
    }
    return new_edge;
}

/* 
 *  Remove an edge.
 */
void delete_edge(struct edge *e)
{
    struct vertex *u, *v;

    /* Save destination and origin. */
    u = ORG(e);
    v = DEST(e);

    /* Set entry points. */
    if (u->entry_pt == e)
	u->entry_pt = ONEXT(e);
    if (v->entry_pt == e)
	v->entry_pt = DNEXT(e);

    /* Four edge references need adjustment */
    if (ORG(ONEXT(e)) == u)
	OPREV(ONEXT(e)) = OPREV(e);
    else
	DPREV(ONEXT(e)) = OPREV(e);

    if (ORG(OPREV(e)) == u)
	ONEXT(OPREV(e)) = ONEXT(e);
    else
	DNEXT(OPREV(e)) = ONEXT(e);

    if (ORG(DNEXT(e)) == v)
	OPREV(DNEXT(e)) = DPREV(e);
    else
	DPREV(DNEXT(e)) = DPREV(e);

    if (ORG(DPREV(e)) == v)
	ONEXT(DPREV(e)) = DNEXT(e);
    else
	DNEXT(DPREV(e)) = DNEXT(e);

    free_edge(e);
}


 /*  Add an edge to a ring of edges. */
void splice(struct edge *a, struct edge *b, struct vertex *v)
{
    struct edge *next;

    /* b must be the unnattached edge and a must be the previous 
       ccw edge to b. */

    if (ORG(a) == v) {
	next = ONEXT(a);
	ONEXT(a) = b;
    }
    else {
	next = DNEXT(a);
	DNEXT(a) = b;
    }

    if (ORG(next) == v)
	OPREV(next) = b;
    else
	DPREV(next) = b;

    if (ORG(b) == v) {
	ONEXT(b) = next;
	OPREV(b) = a;
    }
    else {
	DNEXT(b) = next;
	DPREV(b) = a;
    }
}

 /*  Create a new edge and initialize it */
struct edge *create_edge(struct vertex *v1, struct vertex *v2)
{
    struct edge *new_edge;

    new_edge = get_edge();

    DNEXT(new_edge) = DPREV(new_edge) = ONEXT(new_edge) = OPREV(new_edge) =
	new_edge;
    ORG(new_edge) = v1;
    DEST(new_edge) = v2;
    if (v1->entry_pt == MY_NULL)
	v1->entry_pt = new_edge;
    if (v2->entry_pt == MY_NULL)
	v2->entry_pt = new_edge;
    return new_edge;
}
