#ifndef GEOM_PRIMITIVES_H
#define GEOM_PRIMITIVES_H

#define CREATE_VECTOR(p1, p2, dx, dy) (dx = p2->x - p1->x, dy = p2->y - p1->y)

#define DOT_PRODUCT_2V(a1, a2, b1, b2) (a1 * b1 + a2 * b2)

#define CROSS_PRODUCT_2V(a1, a2, b1, b2) (a1 * b2 - a2 * b1)
/*
   cross-product around p2
   ((p2->x - p1->x) * (p3->y - p2->y) - (p2->y - p1->y) * (p3->x - p2->x))

   around p1
 */
#define CROSS_PRODUCT_3P(p1, p2, p3) ((p2->x - p1->x) * (p3->y - p1->y) - (p2->y - p1->y) * (p3->x - p1->x))

/* predicate testing if p3 is to the left of the line through p1 and p2 */
#define LEFT_OF(p1, p2, p3) (CROSS_PRODUCT_3P(p1, p2, p3) > 0)

/* predicate testing if p3 is to the right of the line through p1 and p2 */
#define RIGHT_OF(p1, p2, p3) (CROSS_PRODUCT_3P(p1, p2, p3) < 0)

#endif
