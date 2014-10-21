
/****** drawline.c *****************************************************

	Function to draw a line segment defined by points (row, col) 
	and (backrow, backcol) using Bresenham's Algorithm.
	Note: 	do not draw the end point (backrow, backcol) 
		except when it is the same as the beginning point 
		(row, col) in order to be able utilize path segments
		drawn before the current path segment.
		 
 ***********************************************************************/
#include <grass/segment.h>


void drawline(int x1, int y1, int x2, int y2)
{
    extern char *value;
    extern SEGMENT out_seg;

    int dx, dy, i, e;
    int incx, incy, inc1, inc2;
    int x, y;
    int data = 1;

    /*debug: printf("\n(%d,%d)->(%d,%d): ", x1,y1,x2,y2); */
    dx = x2 - x1;
    dy = y2 - y1;
    incx = 1;
    incy = 1;
    if (dx < 0) {
	dx = -dx;
	incx = -1;
    }
    if (dy < 0) {
	dy = -dy;
	incy = -1;
    }
    x = x1;
    y = y1;

    value = (char *)&data;

    if (dx > dy) {
	Segment_put(&out_seg, value, x, y);
	/*debug: printf("put1-(%d,%d) ",x,y); */
	e = 2 * dy - dx;
	inc1 = 2 * (dy - dx);
	inc2 = 2 * dy;
	for (i = 0; i < dx - 1; i++) {
	    if (e >= 0) {
		y += incy;
		e += inc1;
	    }
	    else
		e += inc2;
	    x += incx;
	    Segment_put(&out_seg, value, x, y);
	    /*debug:printf("put2-(%d,%d) ",x,y); */
	}
    }
    else {
	Segment_put(&out_seg, value, x, y);
	/*debug:printf("put3-(%d,%d) ",x,y); */
	e = 2 * dx - dy;
	inc1 = 2 * (dx - dy);
	inc2 = 2 * dx;
	for (i = 0; i < dy - 1; i++) {
	    if (e >= 0) {
		x += incx;
		e += inc1;
	    }
	    else
		e += inc2;
	    y += incy;
	    Segment_put(&out_seg, value, x, y);
	    /*debug:rintf("put4-(%d,%d) ",x,y); */
	}
    }
}
