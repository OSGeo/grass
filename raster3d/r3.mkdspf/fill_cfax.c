#include "vizual.h"
/* #include "cell_table.h"  (in vizual.h)  */


/* place vertex data into CUBEFAX structure */
void fill_cfax(Cube_data * Cube, int flag, int index, float
	       TEMP_VERT[13][3], float TEMP_NORM[13][3])
{
    int p;			/*loop variable */
    int n;			/*index into TEMP_VERT array */
    int num = 0;		/*polygon number index into TEMP_NORM for flat shading */
    cube_info *CUBEFAX;

    CUBEFAX = Cube->data;	/* make old code work w/ new structure */

    /* DEBUG 
       fprintf(stderr,"%d npoly  %d flag\n",CUBEFAX[NTHRESH].npoly,flag);
       END DEBUG */

    CUBEFAX[NTHRESH].npoly = cell_table[index].npolys;

    /* this is for gradient shading (3 normals per polygon) */
    if (flag > 1) {
	for (p = 0, num = 0; num < CUBEFAX[NTHRESH].npoly; num++) {
	    poly_info *CP;

	    /* NOTE: normals recorded by edge */

	    /* avoid a little redundant calculation:  dpg */
	    CP = &(CUBEFAX[NTHRESH].poly[num]);

	    n = cell_table[index].polys[p];
	    CP->v1[0] = TEMP_VERT[n][0];
	    CP->v1[1] = TEMP_VERT[n][1];
	    CP->v1[2] = TEMP_VERT[n][2];
	    CP->n1[0] = (TEMP_NORM[n][0] + 1.) * 127;
	    CP->n1[1] = (TEMP_NORM[n][1] + 1.) * 127;
	    CP->n1[2] = (TEMP_NORM[n][2] + 1.) * 127;
	    p++;

	    n = cell_table[index].polys[p];
	    CP->v2[0] = TEMP_VERT[n][0];
	    CP->v2[1] = TEMP_VERT[n][1];
	    CP->v2[2] = TEMP_VERT[n][2];
	    CP->n2[0] = (TEMP_NORM[n][0] + 1.) * 127;
	    CP->n2[1] = (TEMP_NORM[n][1] + 1.) * 127;
	    CP->n2[2] = (TEMP_NORM[n][2] + 1.) * 127;
	    p++;

	    n = cell_table[index].polys[p];
	    CP->v3[0] = TEMP_VERT[n][0];
	    CP->v3[1] = TEMP_VERT[n][1];
	    CP->v3[2] = TEMP_VERT[n][2];
	    CP->n3[0] = (TEMP_NORM[n][0] + 1.) * 127;
	    CP->n3[1] = (TEMP_NORM[n][1] + 1.) * 127;
	    CP->n3[2] = (TEMP_NORM[n][2] + 1.) * 127;
	    p++;
	}
    }
    else if (flag == 1) {	/* this is for flat shading (NOTE: 1 normal per polygon) */
	for (p = 0, num = 0; num < CUBEFAX[NTHRESH].npoly; num++) {
	    poly_info *CP;

	    /* avoid a little redundant calculation:  dpg */
	    CP = &(CUBEFAX[NTHRESH].poly[num]);

	    n = cell_table[index].polys[p];
	    CP->v1[0] = TEMP_VERT[n][0];
	    CP->v1[1] = TEMP_VERT[n][1];
	    CP->v1[2] = TEMP_VERT[n][2];
	    p++;

	    n = cell_table[index].polys[p];
	    CP->v2[0] = TEMP_VERT[n][0];
	    CP->v2[1] = TEMP_VERT[n][1];
	    CP->v2[2] = TEMP_VERT[n][2];
	    p++;

	    n = cell_table[index].polys[p];
	    CP->v3[0] = TEMP_VERT[n][0];
	    CP->v3[1] = TEMP_VERT[n][1];
	    CP->v3[2] = TEMP_VERT[n][2];
	    p++;
	    /* NOTE: in calc_fnorm() flat shaded normals recorded by polygon */
	    /*now assigning the normal for this polygon */
	    CP->n1[0] = (TEMP_NORM[num][0] + 1.) * 127;
	    CP->n1[1] = (TEMP_NORM[num][1] + 1.) * 127;
	    CP->n1[2] = (TEMP_NORM[num][2] + 1.) * 127;

	}
    }
}
