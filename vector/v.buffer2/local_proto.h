#ifndef LOCAL_PROTO_H
#define LOCAL_PROTO_H

void Vect_line_buffer2(struct line_pnts *Points, double da, double db, double dalpha, int round, int caps, double tol, struct line_pnts **oPoints, struct line_pnts ***iPoints, int *inner_count);
void Vect_area_buffer2(struct Map_info *Map, int area, double da, double db, double dalpha, int round, int caps, double tol, struct line_pnts **oPoints, struct line_pnts ***iPoints, int *inner_count);
void Vect_point_buffer2(double px, double py, double da, double db, double dalpha, int round, double tol, struct line_pnts **oPoints);
void Vect_line_parallel2(struct line_pnts *InPoints, double da, double db, double dalpha, int side, int round, double tol, struct line_pnts *OutPoints);

#endif
