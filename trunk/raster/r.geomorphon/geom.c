#include "local_proto.h"
/* static double dirs[8] = { 0.7854, 0., 5.4978, 4.7124, 3.9270, 3.1416, 2.3562, 1.5708 };*/	/* radians */
static double sins[8] = { 0.7071067812, 0, -0.7071067812, -1, -0.7071067812, 0, 0.7071067812, 1 };	/* sinus */
static double coss[8] = { 0.7071067812, 1, 0.7071067812, 0, -0.7071067812, -1, -0.7071067812, 0 };	/* cosinus */

/* DIRS in DEGREES from NORTH: 45,0,315,270,225,180,135,90 */

unsigned int ternary_rotate(unsigned int value)
{
    /* this function returns rotated and mirrored
     * ternary code from any number
     * function is used to create lookup table with original
     * terrain patterns (6561) and its rotated and mirrored counterparts (498)*/

    unsigned char pattern[8];
    unsigned char rev_pattern[8];
    unsigned char tmp_pattern[8];
    unsigned char tmp_rev_pattern[8];
    unsigned int code = 10000, tmp_code, rev_code = 10000, tmp_rev_code;
    int power = 1;
    int i, j, k;

    for (i = 0; i < 8; i++) {
	pattern[i] = value % 3;
	rev_pattern[7 - i] = value % 3;
	value /= 3;
    }

    for (j = 0; j < 8; j++) {
	power = 1;
	tmp_code = 0;
	tmp_rev_code = 0;
	for (i = 0; i < 8; i++) {
	    k = (i - j) < 0 ? j - 8 : j;
	    tmp_pattern[i] = pattern[i - k];
	    tmp_rev_pattern[i] = rev_pattern[i - k];
	    tmp_code += tmp_pattern[i] * power;
	    tmp_rev_code += tmp_rev_pattern[i] * power;
	    power *= 3;
	}
	code = tmp_code < code ? tmp_code : code;
	rev_code = tmp_rev_code < rev_code ? tmp_rev_code : rev_code;
    }
    return code < rev_code ? code : rev_code;
}

int determine_form(int num_minus, int num_plus)
{
    /* determine form according number of positives and negatives
     * simple approach to determine form pattern */

    const FORMS forms[9][9] = {
	/* minus ------------- plus ---------------- */
	/*       0   1   2   3   4   5   6   7   8  */
	/* 0 */ {FL, FL, FL, FS, FS, VL, VL, VL, PT},
	/* 1 */ {FL, FL, FS, FS, FS, VL, VL, VL, __},
	/* 2 */ {FL, SH, SL, SL, CN, CN, VL, __, __},
	/* 3 */ {SH, SH, SL, SL, SL, CN, __, __, __},
	/* 4 */ {SH, SH, CV, SL, SL, __, __, __, __},
	/* 5 */ {RI, RI, CV, CV, __, __, __, __, __},
	/* 6 */ {RI, RI, RI, __, __, __, __, __, __},
	/* 7 */ {RI, RI, __, __, __, __, __, __, __},
	/* 8 */ {PK, __, __, __, __, __, __, __, __},
    };

    /* legend:
       FL,  flat
       PK,  peak, summit
       RI,  ridge
       SH,  shoulder
       CV,  convex
       SL,  slope
       CN,  concave
       FS,  footslope
       VL,  valley
       PT,  pit, depression
       __  error, impossible
     */
    return forms[num_minus][num_plus];
}

int determine_binary(int *pattern, int sign)
{
    /* extract binary pattern for zenith (+) or nadir (-) from unrotated ternary pattern */
    int n, i;
    unsigned char binary = 0, result = 255, test = 0;

    for (i = 0, n = 1; i < 8; i++, n *= 2)
	binary += (pattern[i] == sign) ? n : 0;
    /* rotate */
    for (i = 0; i < 8; ++i) {
	if ((i &= 7) == 0)
	    test = binary;
	else
	    test = (binary << i) | (binary >> (8 - i));
	result = (result < test) ? result : test;
    }
    return (int)result;
}


int rotate(unsigned char binary)
{
    int i;
    unsigned char result = 255, test = 0;

    /* rotate */
    for (i = 0; i < 8; ++i) {
	if ((i &= 7) == 0)
	    test = binary;
	else
	    test = (binary << i) | (binary >> (8 - i));
	result = (result < test) ? result : test;
    }
    return (int)result;
}

int determine_ternary(int *pattern)
{
    /* extract rotated and mirrored ternary pattern form unrotated ternary pattern */
    unsigned ternary_code = 0;
    int power, i;

    for (i = 0, power = 1; i < 8; ++i, power *= 3)
	ternary_code += (pattern[i] + 1) * power;
    return global_ternary_codes[ternary_code];
}

float intensity(float *elevation, int pattern_size)
{
    /* calculate relative elevation of the central cell against its visibility surround */
    float sum_elevation = 0.;
    int i;

    for (i = 0; i < 8; i++)
	sum_elevation -= elevation[i];

    return sum_elevation / (float)pattern_size;
}

float exposition(float *elevation)
{
    /* calculate relative elevation of the central cell against its visibility */
    float max = 0.;
    int i;

    for (i = 0; i < 8; i++)
	max = fabs(elevation[i]) > fabs(max) ? elevation[i] : max;
    return -max;
}

float range(float *elevation)
{
    /* calculate relative difference in visible range of central cell */
    float max = -90000000000., min = 9000000000000.;	/* should be enough */
    int i;

    for (i = 0; i < 8; i++) {
	max = elevation[i] > max ? elevation[i] : max;
	min = elevation[i] < min ? elevation[i] : min;
    }

    return max - min;
}

float variance(float *elevation, int pattern_size)
{
    /* calculate relative variation of the visible neighbourhood of the cell  */
    float sum_elevation = 0;
    float variance = 0;
    int i;

    for (i = 0; i < 8; i++)
	sum_elevation += elevation[i];
    sum_elevation /= (float)pattern_size;
    for (i = 0; i < 8; i++)
	variance +=
	    ((sum_elevation - elevation[i]) * (sum_elevation - elevation[i]));

    return variance / (float)pattern_size;
}

int radial2cartesian(PATTERN * pattern)
{
    /* this function converts radial coordinates of geomorphon
     * (assuming center as 0,0) to cartezian coordinates
     * with the beginning in the central cell of geomorphon */
    int i;

    for (i = 0; i < 8; ++i)
	if (pattern->distance > 0) {
	    pattern->x[i] = pattern->distance[i] * sins[i];
	    pattern->y[i] = pattern->distance[i] * coss[i];
	}
	else {
	    pattern->x[i] = 0;
	    pattern->y[i] = 0;
	}
    return 0;
}

float extends(PATTERN * pattern, int pattern_size)
{
    int i, j;
    float area = 0;

    for (i = 0, j = 1; i < 8; ++i, ++j) {
	j = j < 8 ? j : 0;
	area +=
	    (pattern->x[i] * pattern->y[j] - pattern->x[j] * pattern->y[i]);
    }
    return fabs(area) / 2.;
}

int shape(PATTERN * pattern, int pattern_size, float *azimuth,
	  float *elongation, float *width)
{
    /* calculates azimuth, elongation and width of geomorphon's polygon */
    int i;
    double avg_x = 0, avg_y = 0;
    double avg_x_y = 0;
    double avg_x_square;
    double rx, ry;
    double sine, cosine;
    double result;
    double rxmin, rxmax, rymin, rymax;

    for (i = 0; i < 8; ++i) {
	avg_y += pattern->y[i];
	avg_x += pattern->x[i];
	avg_x_square += pattern->x[i] * pattern->x[i];
	avg_x_y += pattern->x[i] * pattern->y[i];
    }
    avg_y /= (float)pattern_size;
    avg_x /= (float)pattern_size;
    avg_x_y /= (float)pattern_size;
    avg_x_square /= (float)pattern_size;
    result = (avg_x_y - avg_x * avg_y) / (avg_x_square - avg_x * avg_x);
    result = atan(result);
    *azimuth = (float)RAD2DEGREE(PI2 - result);

    /* rotation */
    sine = sin(result);
    cosine = cos(result);
    for (i = 0; i < 8; ++i) {
	rx = pattern->x[i] * cosine - pattern->y[i] * sine;
	ry = pattern->x[i] * sine + pattern->y[i] * cosine;
	rxmin = rx < rxmin ? rx : rxmin;
	rxmax = rx > rxmax ? rx : rxmax;
	rymin = ry < rymin ? ry : rymin;
	rymax = ry > rymax ? ry : rymax;
    }
    rx = (rxmax - rxmin);
    ry = (rymax - rymin);
    *elongation = rx > ry ? (float)rx / ry : (float)ry / rx;
    *width = rx > ry ? ry : rx;

    return 0;
}
