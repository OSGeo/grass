#include <stdio.h>
#include <grass/gis.h>

#define POLYGON_DIMENSION 20
/* From FAOSOIL CD, after USDA 1951, p209 */

struct vector
{
    double sand;
    double clay;
    double silt;
};

double point_in_triangle(double point_x, double point_y, double point_z,
			 double t1_x, double t1_y, double t1_z, double t2_x,
			 double t2_y, double t2_z, double t3_x, double t3_y,
			 double t3_z)
{
    G_debug(1,"point_in_triangle: sand=%5.3f clay=%5.3f silt=%5.3f",
				point_x,point_y,point_z);
    double answer;
    double answer1_x, answer1_y, answer1_z;
    double answer2_x, answer2_y, answer2_z;
    double answer3_x, answer3_y, answer3_z;

    /* Consider three points forming a trinagle from a given soil class boundary ABC */
    /* Consider F an additional point in space */
    double af1, af2, af3;	/*Points for vector AF */
    double bf1, bf2, bf3;	/*Points for vector BF */
    double cf1, cf2, cf3;	/*Points for vector CF */
    double ab1, ab2, ab3;	/*Points for vector AB */
    double bc1, bc2, bc3;	/*Points for vector BC */
    double ca1, ca2, ca3;	/*Points for vector CA */

    /* Create vectors AB, BC and CA */
    ab1 = (t2_x - t1_x);
    ab2 = (t2_y - t1_y);
    ab3 = (t2_z - t1_z);
    bc1 = (t3_x - t2_x);
    bc2 = (t3_y - t2_y);
    bc3 = (t3_z - t2_z);
    ca1 = (t1_x - t3_x);
    ca2 = (t1_y - t3_y);
    ca3 = (t1_z - t3_z);
    /* Create vectors AF, BF and CF */
    af1 = (point_x - t1_x);
    af2 = (point_y - t1_y);
    af3 = (point_z - t1_z);
    bf1 = (point_x - t2_x);
    bf2 = (point_y - t2_y);
    bf3 = (point_z - t2_z);
    cf1 = (point_x - t3_x);
    cf2 = (point_y - t3_y);
    cf3 = (point_z - t3_z);
    /* Calculate the following CrossProducts: */
    /* AFxAB */
    answer1_x = (af2 * ab3) - (af3 * ab2);
    answer1_y = (af3 * ab1) - (af1 * ab3);
    answer1_z = (af1 * ab2) - (af2 * ab1);
    /* G_message("answer(AFxAB)= %f %f %f",answer1_x, answer1_y, answer1_z); */
    /*BFxBC */
    answer2_x = (bf2 * bc3) - (bf3 * bc2);
    answer2_y = (bf3 * bc1) - (bf1 * bc3);
    answer2_z = (bf1 * bc2) - (bf2 * bc1);
    /* G_message("answer(BFxBC)= %f %f %f",answer2_x, answer2_y, answer2_z); */
    /*CFxCA */
    answer3_x = (cf2 * ca3) - (cf3 * ca2);
    answer3_y = (cf3 * ca1) - (cf1 * ca3);
    answer3_z = (cf1 * ca2) - (cf2 * ca1);
    /* G_message("answer(CFxCA)= %f %f %f",answer3_x, answer3_y, answer3_z); */
    answer = 0.0; /*initialize value */
    if ((int)answer1_x >= 0 && (int)answer2_x >= 0 && (int)answer3_x >= 0) {
	answer += 1.0;
    }
    else if ((int)answer1_x <= 0 && (int)answer2_x <= 0 &&
	     (int)answer3_x <= 0) {
	answer -= 1.0;
    }
    if ((int)answer1_y >= 0 && (int)answer2_y >= 0 && (int)answer3_y >= 0) {
	answer += 1.0;
    }
    else if ((int)answer1_y <= 0 && (int)answer2_y <= 0 &&
	     (int)answer3_y <= 0) {
	answer -= 1.0;
    }
    if ((int)answer1_z >= 0 && (int)answer2_z >= 0 && (int)answer3_z >= 0) {
	answer += 1.0;
    }
    else if ((int)answer1_z <= 0 && (int)answer2_z <= 0 &&
	     (int)answer3_z <= 0) {
	answer -= 1.0;
    }
    if (answer == 3 || answer == -3) {
	answer = 1;
    }
    else {
	answer = 0;
    }
    return answer;
}

int prct2tex(double sand_input, double clay_input, double silt_input)
{
    G_debug(1,"%5.3f||%5.3f||%5.3f",sand_input,clay_input,silt_input);
    
    /* set up index for soil texture classes */
    int index = 20;

    /* set up mark index for inside/outside polygon check */
    double mark[POLYGON_DIMENSION] = { 0.0 };
    /*G_message("in prct2tex()"); */
    /*setup the 3Dvectors and initialize them */
    /* index 0 */
    struct vector cls_clay[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 1 */
    struct vector cls_sandy_clay[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 2 */
    struct vector cls_silty_clay[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 3 */
    struct vector cls_sandy_clay_loam[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 4 */
    struct vector cls_clay_loam[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 5 */
    struct vector cls_silty_clay_loam[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 6 */
    struct vector cls_sand[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 7 */
    struct vector cls_loamy_sand[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 8 */
    struct vector cls_sandy_loam[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 9 */
    struct vector cls_loam[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 10 */
    struct vector cls_silt_loam[POLYGON_DIMENSION] = { { 0.0 } };
    /* index 11 */
    struct vector cls_silt[POLYGON_DIMENSION] = { { 0.0 } };

    if ((sand_input + clay_input + silt_input) <= 10.0) {
	sand_input = sand_input * 100.0;
	clay_input = clay_input * 100.0;
	silt_input = silt_input * 100.0;
    }
    /*G_message("%5.3f||%5.3f||%5.3f|",sand_input,clay_input,silt_input); */

    /*Feed the polygons for index 0 */
    cls_clay[0].sand = 0.0;
    cls_clay[0].clay = 100.0;
    cls_clay[0].silt = 0.0;
    cls_clay[1].sand = 0.0;
    cls_clay[1].clay = 60.0;
    cls_clay[1].silt = 40.0;
    cls_clay[2].sand = 20.0;
    cls_clay[2].clay = 40.0;
    cls_clay[2].silt = 40.0;
    cls_clay[3].sand = 50.0;
    cls_clay[3].clay = 40.0;
    cls_clay[3].silt = 10.0;
    cls_clay[4].sand = 50.0;
    cls_clay[4].clay = 50.0;
    cls_clay[4].silt = 0.0;
    /* Check for index 0 */
    /* G_message("in prct2tex(): check for index 0"); */
    mark[0] =
	point_in_triangle(sand_input, clay_input, silt_input,
			  cls_clay[0].sand, cls_clay[0].clay,
			  cls_clay[0].silt, cls_clay[1].sand,
			  cls_clay[1].clay, cls_clay[1].silt,
			  cls_clay[2].sand, cls_clay[2].clay,
			  cls_clay[2].silt);
    mark[1] =
	point_in_triangle(sand_input, clay_input, silt_input,
			  cls_clay[0].sand, cls_clay[0].clay,
			  cls_clay[0].silt, cls_clay[2].sand,
			  cls_clay[2].clay, cls_clay[2].silt,
			  cls_clay[3].sand, cls_clay[3].clay,
			  cls_clay[3].silt);
    mark[2] =
	point_in_triangle(sand_input, clay_input, silt_input,
			  cls_clay[0].sand, cls_clay[0].clay,
			  cls_clay[0].silt, cls_clay[3].sand,
			  cls_clay[3].clay, cls_clay[3].silt,
			  cls_clay[4].sand, cls_clay[4].clay,
			  cls_clay[4].silt);
    /* G_message("Clay: mark[0]=%f",mark[0]); */
    /* G_message("Clay: mark[1]=%f",mark[1]); */
    /* G_message("Clay: mark[2]=%f",mark[2]); */
    if (mark[0] == 1 || mark[1] == 1 || mark[2] == 1) {
	index = 0;
	/* G_message("Clay: index labelled as 0"); */
    }
    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 1 */
	cls_sandy_clay[0].sand = 50.0;
	cls_sandy_clay[0].clay = 50.0;
	cls_sandy_clay[0].silt = 0.0;
	cls_sandy_clay[1].sand = 50.0;
	cls_sandy_clay[1].clay = 35.0;
	cls_sandy_clay[1].silt = 15.0;
	cls_sandy_clay[2].sand = 65.0;
	cls_sandy_clay[2].clay = 35.0;
	cls_sandy_clay[2].silt = 0.0;
	/* Check for index 1 */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_clay[0].sand, cls_sandy_clay[0].clay,
			      cls_sandy_clay[0].silt, cls_sandy_clay[1].sand,
			      cls_sandy_clay[1].clay, cls_sandy_clay[1].silt,
			      cls_sandy_clay[2].sand, cls_sandy_clay[2].clay,
			      cls_sandy_clay[2].silt);

	/* G_message("Sandy Clay: mark[0]=%f",mark[0]); */
	if (mark[0] == 1) {
	    index = 1;
	    /* G_message("Sandy Clay: index labelled as 1"); */
	}
    }
    if (index == 20) {		/* if index not found then continue */
	/*Feed the polygons for index 2 */
	cls_silty_clay[0].sand = 0.0;
	cls_silty_clay[0].clay = 60.0;
	cls_silty_clay[0].silt = 40.0;
	cls_silty_clay[1].sand = 0.0;
	cls_silty_clay[1].clay = 40.0;
	cls_silty_clay[1].silt = 60.0;
	cls_silty_clay[2].sand = 20.0;
	cls_silty_clay[2].clay = 40.0;
	cls_silty_clay[2].silt = 40.0;
	/* Check for index 2 */
	/* G_message("sand=%5.3f||clay=%5.3f||silt=%5.3f",sand_input,
				clay_input,silt_input); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silty_clay[0].sand, cls_silty_clay[0].clay,
			      cls_silty_clay[0].silt, cls_silty_clay[1].sand,
			      cls_silty_clay[1].clay, cls_silty_clay[1].silt,
			      cls_silty_clay[2].sand, cls_silty_clay[2].clay,
			      cls_silty_clay[2].silt);

	/* G_message("Silty Clay: mark[0]=%f",mark[0]); */
	if (mark[0] == 1) {
	    index = 2;
	    /* G_message("Silty Clay: index labelled as 2"); */
	}
    }
    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 3 */
	cls_sandy_clay_loam[0].sand = 65.0;
	cls_sandy_clay_loam[0].clay = 35.0;
	cls_sandy_clay_loam[0].silt = 0.0;
	cls_sandy_clay_loam[1].sand = 50.0;
	cls_sandy_clay_loam[1].clay = 35.0;
	cls_sandy_clay_loam[1].silt = 15.0;
	cls_sandy_clay_loam[2].sand = 50.0;
	cls_sandy_clay_loam[2].clay = 30.0;
	cls_sandy_clay_loam[2].silt = 20.0;
	cls_sandy_clay_loam[3].sand = 55.0;
	cls_sandy_clay_loam[3].clay = 25.0;
	cls_sandy_clay_loam[3].silt = 20.0;
	cls_sandy_clay_loam[4].sand = 75.0;
	cls_sandy_clay_loam[4].clay = 25.0;
	cls_sandy_clay_loam[4].silt = 0.0;
	/* Check for index 0 */
	/* G_message("in prct2tex(): check for index 3"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_clay_loam[0].sand,
			      cls_sandy_clay_loam[0].clay,
			      cls_sandy_clay_loam[0].silt,
			      cls_sandy_clay_loam[1].sand,
			      cls_sandy_clay_loam[1].clay,
			      cls_sandy_clay_loam[1].silt,
			      cls_sandy_clay_loam[2].sand,
			      cls_sandy_clay_loam[2].clay,
			      cls_sandy_clay_loam[2].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_clay_loam[0].sand,
			      cls_sandy_clay_loam[0].clay,
			      cls_sandy_clay_loam[0].silt,
			      cls_sandy_clay_loam[2].sand,
			      cls_sandy_clay_loam[2].clay,
			      cls_sandy_clay_loam[2].silt,
			      cls_sandy_clay_loam[3].sand,
			      cls_sandy_clay_loam[3].clay,
			      cls_sandy_clay_loam[3].silt);
	mark[2] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_clay_loam[0].sand,
			      cls_sandy_clay_loam[0].clay,
			      cls_sandy_clay_loam[0].silt,
			      cls_sandy_clay_loam[3].sand,
			      cls_sandy_clay_loam[3].clay,
			      cls_sandy_clay_loam[3].silt,
			      cls_sandy_clay_loam[4].sand,
			      cls_sandy_clay_loam[4].clay,
			      cls_sandy_clay_loam[4].silt);
	/* G_message("Sandy Clay Loam: mark[0]=%f",mark[0]); */
	/* G_message("Sandy Clay Loam: mark[1]=%f",mark[1]); */
	/* G_message("Sandy Clay Loam: mark[2]=%f",mark[2]); */
	if (mark[0] == 1 || mark[1] == 1 || mark[2] == 1) {
	    index = 3;
	    /* G_message("Sandy Clay Loam: index labelled as 3"); */
	}
    }
    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 4 */
	cls_clay_loam[0].sand = 20.0;
	cls_clay_loam[0].clay = 40.0;
	cls_clay_loam[0].silt = 40.0;
	cls_clay_loam[1].sand = 20.0;
	cls_clay_loam[1].clay = 30.0;
	cls_clay_loam[1].silt = 50.0;
	cls_clay_loam[2].sand = 50.0;
	cls_clay_loam[2].clay = 30.0;
	cls_clay_loam[2].silt = 20.0;
	cls_clay_loam[3].sand = 10.0;
	cls_clay_loam[3].clay = 50.0;
	cls_clay_loam[3].silt = 40.0;
	/* Check for index 4 */
	/* G_message("in prct2tex(): check for index 4"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_clay_loam[0].sand, cls_clay_loam[0].clay,
			      cls_clay_loam[0].silt, cls_clay_loam[1].sand,
			      cls_clay_loam[1].clay, cls_clay_loam[1].silt,
			      cls_clay_loam[2].sand, cls_clay_loam[2].clay,
			      cls_clay_loam[2].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_clay_loam[0].sand, cls_clay_loam[0].clay,
			      cls_clay_loam[0].silt, cls_clay_loam[2].sand,
			      cls_clay_loam[2].clay, cls_clay_loam[2].silt,
			      cls_clay_loam[3].sand, cls_clay_loam[3].clay,
			      cls_clay_loam[3].silt);
	/* G_message("Clay Loam: mark[0]=%f",mark[0]); */
	/* G_message("Clay Loam: mark[1]=%f",mark[1]); */
	if (mark[0] == 1 || mark[1] == 1) {
	    index = 4;
	    /* G_message("Clay Loam: index labelled as 4"); */
	}
    }
    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 5 */
	cls_silty_clay_loam[0].sand = 0.0;
	cls_silty_clay_loam[0].clay = 40.0;
	cls_silty_clay_loam[0].silt = 60.0;
	cls_silty_clay_loam[1].sand = 0.0;
	cls_silty_clay_loam[1].clay = 30.0;
	cls_silty_clay_loam[1].silt = 70.0;
	cls_silty_clay_loam[2].sand = 20.0;
	cls_silty_clay_loam[2].clay = 30.0;
	cls_silty_clay_loam[2].silt = 50.0;
	cls_silty_clay_loam[3].sand = 20.0;
	cls_silty_clay_loam[3].clay = 40.0;
	cls_silty_clay_loam[3].silt = 40.0;
	/* Check for index 5 */
	/* G_message("in prct2tex(): check for index 5"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silty_clay_loam[0].sand,
			      cls_silty_clay_loam[0].clay,
			      cls_silty_clay_loam[0].silt,
			      cls_silty_clay_loam[1].sand,
			      cls_silty_clay_loam[1].clay,
			      cls_silty_clay_loam[1].silt,
			      cls_silty_clay_loam[2].sand,
			      cls_silty_clay_loam[2].clay,
			      cls_silty_clay_loam[2].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silty_clay_loam[0].sand,
			      cls_silty_clay_loam[0].clay,
			      cls_silty_clay_loam[0].silt,
			      cls_silty_clay_loam[2].sand,
			      cls_silty_clay_loam[2].clay,
			      cls_silty_clay_loam[2].silt,
			      cls_silty_clay_loam[3].sand,
			      cls_silty_clay_loam[3].clay,
			      cls_silty_clay_loam[3].silt);
	/* G_message("Silty Clay Loam: mark[0]=%f",mark[0]); */
	/* G_message("Silty Clay Loam: mark[1]=%f",mark[1]); */
	if (mark[0] == 1 || mark[1] == 1) {
	    index = 5;
	    /* G_message("Silty Clay Loam: index labelled as 5"); */
	}
    }
    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 6 */
	cls_sand[0].sand = 85.0;
	cls_sand[0].clay = 15.0;
	cls_sand[0].silt = 0.0;
	cls_sand[1].sand = 85.0;
	cls_sand[1].clay = 0.0;
	cls_sand[1].silt = 15.0;
	cls_sand[2].sand = 100.0;
	cls_sand[2].clay = 0.0;
	cls_sand[2].silt = 0.0;
	/* Check for index 6 */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sand[0].sand, cls_sand[0].clay,
			      cls_sand[0].silt, cls_sand[1].sand,
			      cls_sand[1].clay, cls_sand[1].silt,
			      cls_sand[2].sand, cls_sand[2].clay,
			      cls_sand[2].silt);
	/* G_message("Sand: mark[0]=%f",mark[0]); */
	if (mark[0] == 1) {
	    index = 6;
	    /* G_message("Sand: index labelled as 6"); */
	}
    }
    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 7 */
	cls_loamy_sand[0].sand = 80.0;
	cls_loamy_sand[0].clay = 20.0;
	cls_loamy_sand[0].silt = 0.0;
	cls_loamy_sand[1].sand = 70.0;
	cls_loamy_sand[1].clay = 0.0;
	cls_loamy_sand[1].silt = 30.0;
	cls_loamy_sand[2].sand = 85.0;
	cls_loamy_sand[2].clay = 0.0;
	cls_loamy_sand[2].silt = 15.0;
	cls_loamy_sand[3].sand = 85.0;
	cls_loamy_sand[3].clay = 15.0;
	cls_loamy_sand[3].silt = 0.0;
	/* Check for index 7 */
	/* G_message("in prct2tex(): check for index 7"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_loamy_sand[0].sand, cls_loamy_sand[0].clay,
			      cls_loamy_sand[0].silt, cls_loamy_sand[1].sand,
			      cls_loamy_sand[1].clay, cls_loamy_sand[1].silt,
			      cls_loamy_sand[2].sand, cls_loamy_sand[2].clay,
			      cls_loamy_sand[2].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_loamy_sand[0].sand, cls_loamy_sand[0].clay,
			      cls_loamy_sand[0].silt, cls_loamy_sand[2].sand,
			      cls_loamy_sand[2].clay, cls_loamy_sand[2].silt,
			      cls_loamy_sand[3].sand, cls_loamy_sand[3].clay,
			      cls_loamy_sand[3].silt);
	/* G_message("Loamy Sand: mark[0]=%f",mark[0]); */
	/* G_message("Loamy Sand: mark[1]=%f",mark[1]); */
	if (mark[0] == 1 || mark[1] == 1) {
	    index = 7;
	    /* G_message("Loamy Sand: index labelled as 7"); */
	}
    }

    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 8 */
	cls_sandy_loam[0].sand = 75.0;
	cls_sandy_loam[0].clay = 25.0;
	cls_sandy_loam[0].silt = 0.0;
	cls_sandy_loam[1].sand = 55.0;
	cls_sandy_loam[1].clay = 25.0;
	cls_sandy_loam[1].silt = 20.0;
	cls_sandy_loam[2].sand = 55.0;
	cls_sandy_loam[2].clay = 10.0;
	cls_sandy_loam[2].silt = 35.0;
	cls_sandy_loam[3].sand = 40.0;
	cls_sandy_loam[3].clay = 10.0;
	cls_sandy_loam[3].silt = 50.0;
	cls_sandy_loam[4].sand = 50.0;
	cls_sandy_loam[4].clay = 0.0;
	cls_sandy_loam[4].silt = 50.0;
	cls_sandy_loam[5].sand = 70.0;
	cls_sandy_loam[5].clay = 0.0;
	cls_sandy_loam[5].silt = 30.0;
	cls_sandy_loam[6].sand = 80.0;
	cls_sandy_loam[6].clay = 20.0;
	cls_sandy_loam[6].silt = 0.0;
	/* Check for index 8 */
	/* G_message("in prct2tex(): check for index 8"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_loam[2].sand, cls_sandy_loam[2].clay,
			      cls_sandy_loam[2].silt, cls_sandy_loam[3].sand,
			      cls_sandy_loam[3].clay, cls_sandy_loam[3].silt,
			      cls_sandy_loam[4].sand, cls_sandy_loam[4].clay,
			      cls_sandy_loam[4].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_loam[2].sand, cls_sandy_loam[2].clay,
			      cls_sandy_loam[2].silt, cls_sandy_loam[4].sand,
			      cls_sandy_loam[4].clay, cls_sandy_loam[4].silt,
			      cls_sandy_loam[5].sand, cls_sandy_loam[5].clay,
			      cls_sandy_loam[5].silt);
	mark[2] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_loam[2].sand, cls_sandy_loam[2].clay,
			      cls_sandy_loam[2].silt, cls_sandy_loam[5].sand,
			      cls_sandy_loam[5].clay, cls_sandy_loam[5].silt,
			      cls_sandy_loam[6].sand, cls_sandy_loam[6].clay,
			      cls_sandy_loam[6].silt);
	mark[3] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_loam[2].sand, cls_sandy_loam[2].clay,
			      cls_sandy_loam[2].silt, cls_sandy_loam[6].sand,
			      cls_sandy_loam[6].clay, cls_sandy_loam[6].silt,
			      cls_sandy_loam[0].sand, cls_sandy_loam[0].clay,
			      cls_sandy_loam[0].silt);
	mark[4] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_sandy_loam[2].sand, cls_sandy_loam[2].clay,
			      cls_sandy_loam[2].silt, cls_sandy_loam[0].sand,
			      cls_sandy_loam[0].clay, cls_sandy_loam[0].silt,
			      cls_sandy_loam[1].sand, cls_sandy_loam[1].clay,
			      cls_sandy_loam[1].silt);
	/* G_message("Sandy Loam: mark[0]=%f",mark[0]); */
	/* G_message("Sandy Loam: mark[1]=%f",mark[1]); */
	if (mark[0] == 1 || mark[1] == 1 || mark[2] == 1 || mark[3] == 1 ||
	    mark[4] == 1) {
	    index = 8;
	    /* G_message("Sandy Loam: index labelled as 8"); */
	}
    }

    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 9 */
	cls_loam[0].sand = 50.0;
	cls_loam[0].clay = 30.0;
	cls_loam[0].silt = 20.0;
	cls_loam[1].sand = 20.0;
	cls_loam[1].clay = 30.0;
	cls_loam[1].silt = 50.0;
	cls_loam[2].sand = 40.0;
	cls_loam[2].clay = 10.0;
	cls_loam[2].silt = 50.0;
	cls_loam[3].sand = 55.0;
	cls_loam[3].clay = 10.0;
	cls_loam[3].silt = 35.0;
	cls_loam[4].sand = 55.0;
	cls_loam[4].clay = 25.0;
	cls_loam[4].silt = 15.0;
	/* Check for index 9 */
	/* G_message("in prct2tex(): check for index 9"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_loam[0].sand, cls_loam[0].clay,
			      cls_loam[0].silt, cls_loam[1].sand,
			      cls_loam[1].clay, cls_loam[1].silt,
			      cls_loam[2].sand, cls_loam[2].clay,
			      cls_loam[2].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_loam[0].sand, cls_loam[0].clay,
			      cls_loam[0].silt, cls_loam[2].sand,
			      cls_loam[2].clay, cls_loam[2].silt,
			      cls_loam[3].sand, cls_loam[3].clay,
			      cls_loam[3].silt);
	mark[2] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_loam[0].sand, cls_loam[0].clay,
			      cls_loam[0].silt, cls_loam[3].sand,
			      cls_loam[3].clay, cls_loam[3].silt,
			      cls_loam[4].sand, cls_loam[4].clay,
			      cls_loam[4].silt);
	/* G_message("Sandy Loam: mark[0]=%f",mark[0]); */
	/* G_message("Sandy Loam: mark[1]=%f",mark[1]); */
	if (mark[0] == 1 || mark[1] == 1 || mark[2] == 1) {
	    index = 9;
	    /* G_message("Loam: index labelled as 9"); */
	}
    }

    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 10 */
	cls_silt_loam[0].sand = 20.0;
	cls_silt_loam[0].clay = 30.0;
	cls_silt_loam[0].silt = 50.0;
	cls_silt_loam[1].sand = 0.0;
	cls_silt_loam[1].clay = 30.0;
	cls_silt_loam[1].silt = 70.0;
	cls_silt_loam[2].sand = 0.0;
	cls_silt_loam[2].clay = 10.0;
	cls_silt_loam[2].silt = 90.0;
	cls_silt_loam[3].sand = 15.0;
	cls_silt_loam[3].clay = 10.0;
	cls_silt_loam[3].silt = 75.0;
	cls_silt_loam[4].sand = 25.0;
	cls_silt_loam[4].clay = 0.0;
	cls_silt_loam[4].silt = 75.0;
	cls_silt_loam[5].sand = 50.0;
	cls_silt_loam[5].clay = 0.0;
	cls_silt_loam[5].silt = 50.0;
	/* Check for index 10 */
	/* G_message("in prct2tex(): check for index 10"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silt_loam[3].sand, cls_silt_loam[3].clay,
			      cls_silt_loam[3].silt, cls_silt_loam[4].sand,
			      cls_silt_loam[4].clay, cls_silt_loam[4].silt,
			      cls_silt_loam[5].sand, cls_silt_loam[5].clay,
			      cls_silt_loam[5].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silt_loam[3].sand, cls_silt_loam[3].clay,
			      cls_silt_loam[3].silt, cls_silt_loam[5].sand,
			      cls_silt_loam[5].clay, cls_silt_loam[5].silt,
			      cls_silt_loam[0].sand, cls_silt_loam[0].clay,
			      cls_silt_loam[0].silt);
	mark[2] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silt_loam[3].sand, cls_silt_loam[3].clay,
			      cls_silt_loam[3].silt, cls_silt_loam[0].sand,
			      cls_silt_loam[0].clay, cls_silt_loam[0].silt,
			      cls_silt_loam[1].sand, cls_silt_loam[1].clay,
			      cls_silt_loam[1].silt);
	mark[3] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silt_loam[3].sand, cls_silt_loam[3].clay,
			      cls_silt_loam[3].silt, cls_silt_loam[1].sand,
			      cls_silt_loam[1].clay, cls_silt_loam[1].silt,
			      cls_silt_loam[2].sand, cls_silt_loam[2].clay,
			      cls_silt_loam[2].silt);
	/* G_message("Silt Loam: mark[0]=%f",mark[0]); */
	/* G_message("Silt Loam: mark[1]=%f",mark[1]); */
	/* G_message("Silt Loam: mark[2]=%f",mark[2]); */
	/* G_message("Silt Loam: mark[3]=%f",mark[3]); */
	if (mark[0] == 1 || mark[1] == 1 || mark[2] == 1 || mark[3] == 1) {
	    index = 10;
	    /* G_message("Silt Loam: index labelled as 10"); */
	}
    }

    if (index == 20) {	/* if index not found then continue */
	/*Feed the polygons for index 11 */
	cls_silt[0].sand = 15.0;
	cls_silt[0].clay = 10.0;
	cls_silt[0].silt = 75.0;
	cls_silt[1].sand = 0.0;
	cls_silt[1].clay = 10.0;
	cls_silt[1].silt = 90.0;
	cls_silt[2].sand = 0.0;
	cls_silt[2].clay = 0.0;
	cls_silt[2].silt = 100.0;
	cls_silt[3].sand = 25.0;
	cls_silt[3].clay = 0.0;
	cls_silt[3].silt = 75.0;
	/* Check for index 11 */
	/* G_message("in prct2tex(): check for index 11"); */
	mark[0] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silt[0].sand, cls_silt[0].clay,
			      cls_silt[0].silt, cls_silt[1].sand,
			      cls_silt[1].clay, cls_silt[1].silt,
			      cls_silt[2].sand, cls_silt[2].clay,
			      cls_silt[2].silt);
	mark[1] =
	    point_in_triangle(sand_input, clay_input, silt_input,
			      cls_silt[0].sand, cls_silt[0].clay,
			      cls_silt[0].silt, cls_silt[2].sand,
			      cls_silt[2].clay, cls_silt[2].silt,
			      cls_silt[3].sand, cls_silt[3].clay,
			      cls_silt[3].silt);
	/* G_message("Silt: mark[0]=%f",mark[0]); */
	/* G_message("Silt: mark[1]=%f",mark[1]); */
	if (mark[0] == 1 || mark[1] == 1) {
	    index = 11;
	    /* G_message("Silt: index labelled as 11"); */
	}
    }
    if(index==0){
       G_debug(1,"clay");
       } else if (index==1){
       G_debug(1,"sandy clay");
       } else if (index==2){
       G_debug(1,"silty clay");
       } else if (index==3){
       G_debug(1,"sandy clay loam");
       } else if (index==4){
       G_debug(1,"clay loam");
       } else if (index==5){
       G_debug(1,"silty clay loam");
       } else if (index==6){
       G_debug(1,"sand");
       } else if (index==7){
       G_debug(1,"loamy sand");
       } else if (index==8){
       G_debug(1,"sandy loam");
       } else if (index==9){
       G_debug(1,"loam");
       } else if (index==10){
       G_message("silt loam");
       } else if (index==11){
       G_debug(1,"silt");
       } else {
       G_debug(1,"Unable to allocate class");
       }
    return index;
}
