/* @(#)ply_t_bmif.c     2.1   6/26/87 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ply_to_cll.h"

#define A(x)	fprintf(stderr,"%d\n", x)

#define MAXLINE	 90
#define FGET	    fgets(buff,MAXLINE,stdin)
#define READLINE	if (FGET==NULL) exit(EXIT_FAILURE) ;\
					sscanf (buff,"%1c %lf %lf %d\n", &type, &U_y, &U_x, &code)
#define XADJ(x)	 (x * U_to_A_xconv + U_to_A_xadd) ;
#define YADJ(y)	 (y * U_to_A_yconv + U_to_A_yadd) ;

struct element xy[MAX_VERTICIES];

int main(void)
{
    static double x[MAX_VERTICIES];
    static double y[MAX_VERTICIES];
    int num_verticies;
    int num_points;
    int num_pkgs;
    int code, this_code;
    char type, this_type;

    double U_to_A_yadd;
    double U_to_A_xadd;
    double U_to_A_yconv;
    double U_to_A_xconv;
    double U_x, U_y;
    double x_size, y_size, west, south;
    int incr, numrows, numcols;
    char buff[MAXLINE];
    char word[80];

    /* read through Grips Header to pick up coordinate conversion factors */
    do {
	if (FGET == NULL)
	    exit(EXIT_FAILURE);
	sscanf(buff, "%s\n", word);
	incr = strcmp("ENDT\0", word);
    }
    while (incr != 0);

    for (;;) {
	if (FGET == NULL)
	    return 1;
	if (!strncmp("SIZE", buff, 4)) {
	    sscanf(buff, "SIZE %d %d\n", &numrows, &numcols);
	}
	else if (!strncmp("CONVER", buff, 4)) {
	    sscanf(buff, "CONVER%lf%lf%lf%lf",
		   &U_to_A_yconv, &U_to_A_xconv, &U_to_A_yadd, &U_to_A_xadd);
	}
	else if (!strncmp("BOUND", buff, 4)) {
	    sscanf(buff, "BOUND%lf%lf%lf%lf",
		   &y_size, &x_size, &south, &west);
	    U_to_A_yconv = 1.0 / y_size;
	    U_to_A_xconv = 1.0 / x_size;
	    U_to_A_yadd = -(south / y_size);
	    U_to_A_xadd = -(west / x_size);
	}
	else
	    break;
    }

    U_to_A_yconv *= -1;
    U_to_A_yadd = (double)numrows - U_to_A_yadd;

#ifdef DEBUG
    fprintf(stderr, "NUMBER OF ROWS: %d    NUMBER OF COLUMNS: %d\n",
	    numrows, numcols);
    fprintf(stderr, "UtA_xconv %f UtA_yconv %f UtA_xadd %f UtA_yadd %f\n",
	    U_to_A_xconv, U_to_A_yconv, U_to_A_xadd, U_to_A_yadd);
#endif

    /* begin cycling through the coordinates */

    set_limits(numrows, numcols);

    READLINE;
    if (type == '(')
	READLINE;

    num_verticies = 0;
    num_pkgs = 0;

    while (type != 'E') {
	num_verticies = 0;
	this_type = type;
	this_code = code;

	x[num_verticies] = XADJ(U_x);
	y[num_verticies++] = YADJ(U_y);

	READLINE;
	while (type == ' ') {
	    x[num_verticies] = XADJ(U_x);
	    y[num_verticies++] = YADJ(U_y);
	    if (num_verticies > MAX_VERTICIES) {
		while (type == ' ') {
		    num_verticies++;
		    READLINE;
		}
		fprintf(stderr, "MAXIMUM NUMBER OF VERTICIES EXCEEDED\n");
		fprintf(stderr,
			"  number of vertices read: %d   allowed: %d\n",
			num_verticies, MAX_VERTICIES);
		fprintf(stderr, "ABORTING\n");
		exit(EXIT_FAILURE);
	    }
	    READLINE;
	}

	x[num_verticies] = x[0];
	y[num_verticies] = y[0];

	switch (this_type) {	/* switch to A(rea), L(ine), or D(ot) */
	case 'A':
	case 'a':
	    find_area(x, y, num_verticies, xy, &num_points);
	    save_area(xy, num_points, this_code);
	    break;
	case 'L':
	case 'l':
	    do_line(x, y, num_verticies, this_code);
	    break;
	case 'D':
	case 'd':
	    do_dots(x, y, num_verticies, this_code);
	    break;
	default:
	    break;
	}
	num_pkgs++;
    }
    write_end_record(numrows + 1, numrows, numcols, 0);
    write_end_record(0, numrows, numcols, 0);
    exit(0);
}
