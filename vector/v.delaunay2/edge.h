#ifndef EDGE_H
#define EDGE_H

#define ORG(e)   ((e)->org)
#define DEST(e)  ((e)->dest)
#define ONEXT(e) ((e)->onext)
#define OPREV(e) ((e)->oprev)
#define DNEXT(e) ((e)->dnext)
#define DPREV(e) ((e)->dprev)

#define OTHER_VERTEX(e,p) (ORG(e) == p ? DEST(e) : ORG(e))
#define NEXT(e,p)         (ORG(e) == p ? ONEXT(e) : DNEXT(e))
#define PREV(e,p)         (ORG(e) == p ? OPREV(e) : DPREV(e))

#define SAME_EDGE(e1,e2) (e1 == e2)

struct edge *join(struct edge *e1, struct vertex *v1,
		  struct edge *e2, struct vertex *v2, side s);
void delete_edge(struct edge *e);
void splice(struct edge *a, struct edge *b, struct vertex *v);
struct edge *create_edge(struct vertex *v1, struct vertex *v2);

#endif
