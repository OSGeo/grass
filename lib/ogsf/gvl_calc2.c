/*!
   \file lib/ogsf/gvl_calc2.c

   \brief OGSF library - loading and manipulating volumes, MarchingCubes 33 Algorithm (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   Based on implementation of MarchingCubes 33 Algorithm by
   Thomas Lewiner, thomas.lewiner@polytechnique.org, Math Dept, PUC-Rio

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Tomas Paudits, 2004
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <float.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "mc33_table.h"

unsigned char m_case, m_config, m_subconfig;

/*!
   \brief ADD

   \param face
   \param v

   \return
 */
int mc33_test_face(char face, float *v)
{
    float A, B, C, D;

    switch (face) {
    case -1:
    case 1:
	A = v[0];
	B = v[4];
	C = v[5];
	D = v[1];
	break;

    case -2:
    case 2:
	A = v[1];
	B = v[5];
	C = v[6];
	D = v[2];
	break;

    case -3:
    case 3:
	A = v[2];
	B = v[6];
	C = v[7];
	D = v[3];
	break;

    case -4:
    case 4:
	A = v[3];
	B = v[7];
	C = v[4];
	D = v[0];
	break;

    case -5:
    case 5:
	A = v[0];
	B = v[3];
	C = v[2];
	D = v[1];
	break;

    case -6:
    case 6:
	A = v[4];
	B = v[7];
	C = v[6];
	D = v[5];
	break;

    default:
	fprintf(stderr, "Invalid face code %d\n", face);
	A = B = C = D = 0;
    };

    return face * A * (A * C - B * D) >= 0;
}

/*!
   \brief ADD

   \param s
   \param v

   \return
 */
int mc33_test_interior(char s, float *v)
{
    float t, At = 0, Bt = 0, Ct = 0, Dt = 0, a, b;
    char test = 0;
    char edge = -1;

    switch (m_case) {
    case 4:
    case 10:
	a = (v[4] - v[0]) * (v[6] - v[2]) - (v[7] - v[3]) * (v[5] - v[1]);
	b = v[2] * (v[4] - v[0]) + v[0] * (v[6] - v[2])
	    - v[1] * (v[7] - v[3]) - v[3] * (v[5] - v[1]);
	t = -b / (2 * a);

	if (t < 0 || t > 1)
	    return s > 0;

	At = v[0] + (v[4] - v[0]) * t;
	Bt = v[3] + (v[7] - v[3]) * t;
	Ct = v[2] + (v[6] - v[2]) * t;
	Dt = v[1] + (v[5] - v[1]) * t;
	break;

    case 6:
    case 7:
    case 12:
    case 13:
	switch (m_case) {
	case 6:
	    edge = cell_table[OFFSET_T6_1_1 + m_config].polys[0];
	    break;
	case 7:
	    edge = cell_table[OFFSET_T7_4_1 + m_config].polys[13];
	    break;
	case 12:
	    edge = cell_table[OFFSET_T12_2_S1 + m_config].polys[14];
	    break;
	case 13:
	    edge =
		cell_table[OFFSET_T13_5_1 + m_subconfig +
			   m_config * 4].polys[2];
	    break;
	}

	switch (edge) {
	case 0:
	    t = v[0] / (v[0] - v[1]);
	    At = 0;
	    Bt = v[3] + (v[2] - v[3]) * t;
	    Ct = v[7] + (v[6] - v[7]) * t;
	    Dt = v[4] + (v[5] - v[4]) * t;
	    break;
	case 1:
	    t = v[1] / (v[1] - v[2]);
	    At = 0;
	    Bt = v[0] + (v[3] - v[0]) * t;
	    Ct = v[4] + (v[7] - v[4]) * t;
	    Dt = v[5] + (v[6] - v[5]) * t;
	    break;
	case 2:
	    t = v[2] / (v[2] - v[3]);
	    At = 0;
	    Bt = v[1] + (v[0] - v[1]) * t;
	    Ct = v[5] + (v[4] - v[5]) * t;
	    Dt = v[6] + (v[7] - v[6]) * t;
	    break;
	case 3:
	    t = v[3] / (v[3] - v[0]);
	    At = 0;
	    Bt = v[2] + (v[1] - v[2]) * t;
	    Ct = v[6] + (v[5] - v[6]) * t;
	    Dt = v[7] + (v[4] - v[7]) * t;
	    break;
	case 4:
	    t = v[4] / (v[4] - v[5]);
	    At = 0;
	    Bt = v[7] + (v[6] - v[7]) * t;
	    Ct = v[3] + (v[2] - v[3]) * t;
	    Dt = v[0] + (v[1] - v[0]) * t;
	    break;
	case 5:
	    t = v[5] / (v[5] - v[6]);
	    At = 0;
	    Bt = v[4] + (v[7] - v[4]) * t;
	    Ct = v[0] + (v[3] - v[0]) * t;
	    Dt = v[1] + (v[2] - v[1]) * t;
	    break;
	case 6:
	    t = v[6] / (v[6] - v[7]);
	    At = 0;
	    Bt = v[5] + (v[4] - v[5]) * t;
	    Ct = v[1] + (v[0] - v[1]) * t;
	    Dt = v[2] + (v[3] - v[2]) * t;
	    break;
	case 7:
	    t = v[7] / (v[7] - v[4]);
	    At = 0;
	    Bt = v[6] + (v[5] - v[6]) * t;
	    Ct = v[2] + (v[1] - v[2]) * t;
	    Dt = v[3] + (v[0] - v[3]) * t;
	    break;
	case 8:
	    t = v[0] / (v[0] - v[4]);
	    At = 0;
	    Bt = v[3] + (v[7] - v[3]) * t;
	    Ct = v[2] + (v[6] - v[2]) * t;
	    Dt = v[1] + (v[5] - v[1]) * t;
	    break;
	case 9:
	    t = v[1] / (v[1] - v[5]);
	    At = 0;
	    Bt = v[0] + (v[4] - v[0]) * t;
	    Ct = v[3] + (v[7] - v[3]) * t;
	    Dt = v[2] + (v[6] - v[2]) * t;
	    break;
	case 10:
	    t = v[2] / (v[2] - v[6]);
	    At = 0;
	    Bt = v[1] + (v[5] - v[1]) * t;
	    Ct = v[0] + (v[4] - v[0]) * t;
	    Dt = v[3] + (v[7] - v[3]) * t;
	    break;
	case 11:
	    t = v[3] / (v[3] - v[7]);
	    At = 0;
	    Bt = v[2] + (v[6] - v[2]) * t;
	    Ct = v[1] + (v[5] - v[1]) * t;
	    Dt = v[0] + (v[4] - v[0]) * t;
	    break;
	default:
	    fprintf(stderr, "Invalid edge %d\n", edge);
	    break;
	}
	break;

    default:
	fprintf(stderr, "Invalid ambiguous case %d\n", m_case);
	break;
    }

    if (At >= 0)
	test++;
    if (Bt >= 0)
	test += 2;
    if (Ct >= 0)
	test += 4;
    if (Dt >= 0)
	test += 8;

    switch (test) {
    case 0:
	return s > 0;
    case 1:
	return s > 0;
    case 2:
	return s > 0;
    case 3:
	return s > 0;
    case 4:
	return s > 0;
    case 5:
	if (At * Ct < Bt * Dt)
	    return s > 0;
	break;
    case 6:
	return s > 0;
    case 7:
	return s < 0;
    case 8:
	return s > 0;
    case 9:
	return s > 0;
    case 10:
	if (At * Ct >= Bt * Dt)
	    return s > 0;
	break;
    case 11:
	return s < 0;
    case 12:
	return s > 0;
    case 13:
	return s < 0;
    case 14:
	return s < 0;
    case 15:
	return s < 0;
    }

    return s < 0;
}

/*!
   \brief ADD

   \param c_ndx
   \param v

   \return
 */
int mc33_process_cube(int c_ndx, float *v)
{
    m_case = cases[c_ndx][0];
    m_config = cases[c_ndx][1];
    m_subconfig = 0;

    switch (m_case) {
    case 0:
	return -1;

    case 1:
	return OFFSET_T1 + m_config;

    case 2:
	return OFFSET_T2 + m_config;

    case 3:
	if (mc33_test_face(test[OFFSET_TEST3 + m_config][0], v))
	    return OFFSET_T3_2 + m_config;	/* 3.2 */
	else
	    return OFFSET_T3_1 + m_config;	/* 3.1 */

    case 4:
	if (mc33_test_interior(test[OFFSET_TEST4 + m_config][0], v))
	    return OFFSET_T4_1 + m_config;	/* 4.1 */
	else
	    return OFFSET_T4_2 + m_config;	/* 4.2 */

    case 5:
	return OFFSET_T5 + m_config;

    case 6:
	if (mc33_test_face(test[OFFSET_TEST6 + m_config][0], v))
	    return OFFSET_T6_2 + m_config;	/* 6.2 */
	else {
	    if (mc33_test_interior(test[OFFSET_TEST6 + m_config][1], v))
		return OFFSET_T6_1_1 + m_config;	/* 6.1.1 */
	    else
		return OFFSET_T6_1_2 + m_config;	/* 6.1.2 */
	}

    case 7:
	if (mc33_test_face(test[OFFSET_TEST7 + m_config][0], v))
	    m_subconfig += 1;
	if (mc33_test_face(test[OFFSET_TEST7 + m_config][1], v))
	    m_subconfig += 2;
	if (mc33_test_face(test[OFFSET_TEST7 + m_config][2], v))
	    m_subconfig += 4;

	switch (subconfig7[m_subconfig]) {
	case 0:
	    if (mc33_test_interior(test[OFFSET_TEST7 + m_config][3], v))
		return OFFSET_T7_4_2 + m_config;	/* 7.4.2 */
	    else
		return OFFSET_T7_4_1 + m_config;	/* 7.4.1 */
	case 1:
	    return OFFSET_T7_3_S1 + m_config;	/* 7.3 */
	case 2:
	    return OFFSET_T7_3_S2 + m_config;	/* 7.3 */
	case 3:
	    return OFFSET_T7_3_S3 + m_config;	/* 7.3 */
	case 4:
	    return OFFSET_T7_2_S1 + m_config;	/* 7.2 */
	case 5:
	    return OFFSET_T7_2_S2 + m_config;	/* 7.2 */
	case 6:
	    return OFFSET_T7_2_S3 + m_config;	/* 7.2 */
	case 7:
	    return OFFSET_T7_1 + m_config;	/* 7.1 */
	};

    case 8:
	return OFFSET_T8 + m_config;

    case 9:
	return OFFSET_T9 + m_config;

    case 10:
	if (mc33_test_face(test[OFFSET_TEST10 + m_config][0], v)) {
	    if (mc33_test_face(test[OFFSET_TEST10 + m_config][1], v))
		return OFFSET_T10_1_1_S2 + m_config;	/* 10.1.1 */
	    else {
		return OFFSET_T10_2_S1 + m_config;	/* 10.2 */
	    }
	}
	else {
	    if (mc33_test_face(test[OFFSET_TEST10 + m_config][1], v)) {
		return OFFSET_T10_2_S2 + m_config;	/* 10.2 */
	    }
	    else {
		if (mc33_test_interior(test[OFFSET_TEST10 + m_config][2], v))
		    return OFFSET_T10_1_1_S1 + m_config;	/* 10.1.1 */
		else
		    return OFFSET_T10_1_2 + m_config;	/* 10.1.2 */
	    }
	}

    case 11:
	return OFFSET_T11 + m_config;

    case 12:
	if (mc33_test_face(test[OFFSET_TEST12 + m_config][0], v)) {
	    if (mc33_test_face(test[OFFSET_TEST12 + m_config][1], v))
		return OFFSET_T12_1_1_S2 + m_config;	/* 12.1.1 */
	    else {
		return OFFSET_T12_2_S1 + m_config;	/* 12.2 */
	    }
	}
	else {
	    if (mc33_test_face(test[OFFSET_TEST12 + m_config][1], v)) {
		return OFFSET_T12_2_S2 + m_config;	/* 12.2 */
	    }
	    else {
		if (mc33_test_interior(test[OFFSET_TEST12 + m_config][2], v))
		    return OFFSET_T12_1_1_S1 + m_config;	/* 12.1.1 */
		else
		    return OFFSET_T12_1_2 + m_config;	/* 12.1.2 */
	    }
	}

    case 13:
	if (mc33_test_face(test[OFFSET_TEST13 + m_config][0], v))
	    m_subconfig += 1;
	if (mc33_test_face(test[OFFSET_TEST13 + m_config][1], v))
	    m_subconfig += 2;
	if (mc33_test_face(test[OFFSET_TEST13 + m_config][2], v))
	    m_subconfig += 4;
	if (mc33_test_face(test[OFFSET_TEST13 + m_config][3], v))
	    m_subconfig += 8;
	if (mc33_test_face(test[OFFSET_TEST13 + m_config][4], v))
	    m_subconfig += 16;
	if (mc33_test_face(test[OFFSET_TEST13 + m_config][5], v))
	    m_subconfig += 32;

	switch (subconfig13[m_subconfig]) {
	case 0:		/* 13.1 */
	    return OFFSET_T13_1_S1 + m_config;

	case 1:		/* 13.2 */
	    return OFFSET_T13_2_S1 + 0 + m_config * 6;
	case 2:		/* 13.2 */
	    return OFFSET_T13_2_S1 + 1 + m_config * 6;
	case 3:		/* 13.2 */
	    return OFFSET_T13_2_S1 + 2 + m_config * 6;
	case 4:		/* 13.2 */
	    return OFFSET_T13_2_S1 + 3 + m_config * 6;
	case 5:		/* 13.2 */
	    return OFFSET_T13_2_S1 + 4 + m_config * 6;
	case 6:		/* 13.2 */
	    return OFFSET_T13_2_S1 + 5 + m_config * 6;

	case 7:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 0 + m_config * 12;
	case 8:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 1 + m_config * 12;
	case 9:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 2 + m_config * 12;
	case 10:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 3 + m_config * 12;
	case 11:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 4 + m_config * 12;
	case 12:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 5 + m_config * 12;
	case 13:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 6 + m_config * 12;
	case 14:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 7 + m_config * 12;
	case 15:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 8 + m_config * 12;
	case 16:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 9 + m_config * 12;
	case 17:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 10 + m_config * 12;
	case 18:		/* 13.3 */
	    return OFFSET_T13_3_S1 + 11 + m_config * 12;

	case 19:		/* 13.4 */
	    return OFFSET_T13_4 + 0 + m_config * 4;
	case 20:		/* 13.4 */
	    return OFFSET_T13_4 + 1 + m_config * 4;
	case 21:		/* 13.4 */
	    return OFFSET_T13_4 + 2 + m_config * 4;
	case 22:		/* 13.4 */
	    return OFFSET_T13_4 + 3 + m_config * 4;

	case 23:		/* 13.5 */
	    m_subconfig = 0;
	    if (mc33_test_interior(test[OFFSET_TEST13 + m_config][6], v))
		return OFFSET_T13_5_1 + 0 + m_config * 4;
	    else
		return OFFSET_T13_5_2 + 0 + m_config * 4;
	case 24:		/* 13.5 */
	    m_subconfig = 1;
	    if (mc33_test_interior(test[OFFSET_TEST13 + m_config][6], v))
		return OFFSET_T13_5_1 + 1 + m_config * 4;
	    else
		return OFFSET_T13_5_2 + 1 + m_config * 4;
	case 25:		/* 13.5 */
	    m_subconfig = 2;
	    if (mc33_test_interior(test[OFFSET_TEST13 + m_config][6], v))
		return OFFSET_T13_5_1 + 2 + m_config * 4;
	    else
		return OFFSET_T13_5_2 + 2 + m_config * 4;
	case 26:		/* 13.5 */
	    m_subconfig = 3;
	    if (mc33_test_interior(test[OFFSET_TEST13 + m_config][6], v))
		return OFFSET_T13_5_1 + 3 + m_config * 4;
	    else
		return OFFSET_T13_5_2 + 3 + m_config * 4;

	case 27:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 0 + m_config * 12;
	case 28:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 1 + m_config * 12;
	case 29:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 2 + m_config * 12;
	case 30:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 3 + m_config * 12;
	case 31:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 4 + m_config * 12;
	case 32:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 5 + m_config * 12;
	case 33:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 6 + m_config * 12;
	case 34:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 7 + m_config * 12;
	case 35:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 8 + m_config * 12;
	case 36:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 9 + m_config * 12;
	case 37:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 10 + m_config * 12;
	case 38:		/* 13.3 */
	    return OFFSET_T13_3_S2 + 11 + m_config * 12;

	case 39:		/* 13.2 */
	    return OFFSET_T13_2_S2 + 0 + m_config * 6;
	case 40:		/* 13.2 */
	    return OFFSET_T13_2_S2 + 1 + m_config * 6;
	case 41:		/* 13.2 */
	    return OFFSET_T13_2_S2 + 2 + m_config * 6;
	case 42:		/* 13.2 */
	    return OFFSET_T13_2_S2 + 3 + m_config * 6;
	case 43:		/* 13.2 */
	    return OFFSET_T13_2_S2 + 4 + m_config * 6;
	case 44:		/* 13.2 */
	    return OFFSET_T13_2_S2 + 5 + m_config * 6;

	case 45:		/* 13.1 */
	    return OFFSET_T13_1_S2 + m_config;

	default:
	    fprintf(stderr, "Marching Cubes: Impossible case 13?\n");
	}

    case 14:
	return OFFSET_T14 + m_config;
    }

    return -1;
}
