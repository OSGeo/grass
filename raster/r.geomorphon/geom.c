#include "local_proto.h"
/* static double dirs[NUM_DIRS] = { 0.7854, 0., 5.4978, 4.7124, 3.9270, 3.1416, 2.3562, 1.5708 };*/	/* radians */
static double sins[NUM_DIRS] = { 0.7071067812, 0, -0.7071067812, -1, -0.7071067812, 0, 0.7071067812, 1 };       /* sinus */
static double coss[NUM_DIRS] = { 0.7071067812, 1, 0.7071067812, 0, -0.7071067812, -1, -0.7071067812, 0 };       /* cosinus */

/* DIRS in DEGREES from NORTH: 45,0,315,270,225,180,135,90 */

#define TERNARY_MAX 6561        /* 3**8 */
static unsigned int global_ternary_codes[TERNARY_MAX];

void generate_ternary_codes()
{
    unsigned i;

    for (i = 0; i < TERNARY_MAX; ++i)
        global_ternary_codes[i] = ternary_rotate(i);
}

unsigned int ternary_rotate(unsigned int value)
{
    /* this function returns rotated and mirrored
     * ternary code from any number
     * function is used to create lookup table with original
     * terrain patterns (6561) and its rotated and mirrored counterparts (498)*/

    unsigned char pattern[NUM_DIRS];
    unsigned char rev_pattern[NUM_DIRS];
    unsigned char tmp_pattern[NUM_DIRS];
    unsigned char tmp_rev_pattern[NUM_DIRS];
    unsigned int code = 10000, tmp_code, rev_code = 10000, tmp_rev_code;
    int power = 1;
    int i, j, k;

    for (i = 0; i < NUM_DIRS; i++) {
        pattern[i] = value % 3;
        rev_pattern[7 - i] = value % 3;
        value /= 3;
    }

    for (j = 0; j < NUM_DIRS; j++) {
        power = 1;
        tmp_code = 0;
        tmp_rev_code = 0;
        for (i = 0; i < NUM_DIRS; i++) {
            k = (i - j) < 0 ? j - NUM_DIRS : j;
            tmp_pattern[i] = pattern[i - k];
            tmp_rev_pattern[i] = rev_pattern[i - k];
            tmp_code += tmp_pattern[i] * power;
            tmp_rev_code += tmp_rev_pattern[i] * power;
            power *= 3;
        }
        code = MIN(tmp_code, code);
        rev_code = MIN(tmp_rev_code, rev_code);
    }
    return MIN(code, rev_code);
}

FORMS determine_form(int num_minus, int num_plus)
{
    /* determine form according number of positives and negatives
     * simple approach to determine form pattern */

    const FORMS forms[9][9] = {
        /* minus ------------- plus ---------------- */
        /*       0   1   2   3   4   5   6   7   8  */
        /* 0 */ {FL, FL, FL, FS, FS, VL, VL, VL, PT},
        /* 1 */ {FL, FL, FS, FS, FS, VL, VL, VL, __},
        /* 2 */ {FL, SH, SL, SL, HL, HL, VL, __, __},
        /* 3 */ {SH, SH, SL, SL, SL, HL, __, __, __},
        /* 4 */ {SH, SH, SP, SL, SL, __, __, __, __},
        /* 5 */ {RI, RI, SP, SP, __, __, __, __, __},
        /* 6 */ {RI, RI, RI, __, __, __, __, __, __},
        /* 7 */ {RI, RI, __, __, __, __, __, __, __},
        /* 8 */ {PK, __, __, __, __, __, __, __, __},
    };
    /* (See the FORMS enum for the legend.) */

    return forms[num_minus][num_plus];
}

/*
 * Return the degree of difference between the actual landform and the
 * nominal landform based on the definitions in the plugin manual and the
 * original paper.
 */
int form_deviation(const unsigned num_minus, const unsigned num_plus)
{
    const int dev[9][9] = {
        /* minus ------------- plus ---------------- */
        /*       0   1   2   3   4   5   6   7   8  */
        /* 0 */ {0,  1,  2,  0,  1,  1,  0,  1,  0},
        /* 1 */ {1,  2,  2,  1,  2,  2,  1,  2, -1},
        /* 2 */ {2,  2,  2,  1,  2,  1,  2, -1, -1},
        /* 3 */ {0,  1,  1,  0,  1,  0, -1, -1, -1},
        /* 4 */ {1,  2,  2,  1,  2, -1, -1, -1, -1},
        /* 5 */ {1,  2,  1,  0, -1, -1, -1, -1, -1},
        /* 6 */ {0,  1,  2, -1, -1, -1, -1, -1, -1},
        /* 7 */ {1,  2, -1, -1, -1, -1, -1, -1, -1},
        /* 8 */ {0, -1, -1, -1, -1, -1, -1, -1, -1},
    };

    return (num_minus < 9 && num_plus < 9) ? dev[num_minus][num_plus] : -1;
}

int determine_binary(int *pattern, int sign)
{
    /* extract binary pattern for zenith (+) or nadir (-) from unrotated ternary pattern */
    int n, i;
    unsigned char binary = 0, result = 255, test = 0;

    for (i = 0, n = 1; i < NUM_DIRS; i++, n *= 2)
        binary += (pattern[i] == sign) ? n : 0;
    /* rotate */
    for (i = 0; i < NUM_DIRS; ++i) {
        if ((i &= 7) == 0)
            test = binary;
        else
            test = (binary << i) | (binary >> (NUM_DIRS - i));
        result = MIN(result, test);
    }
    return (int)result;
}


int rotate(unsigned char binary)
{
    int i;
    unsigned char result = 255, test = 0;

    /* rotate */
    for (i = 0; i < NUM_DIRS; ++i) {
        if ((i &= 7) == 0)
            test = binary;
        else
            test = (binary << i) | (binary >> (NUM_DIRS - i));
        result = MIN(result, test);
    }
    return (int)result;
}

int determine_ternary(int *pattern)
{
    /* extract rotated and mirrored ternary pattern form unrotated ternary pattern */
    return global_ternary_codes[preliminary_ternary(pattern)];
}

int preliminary_ternary(const int *pattern)
{
    unsigned ternary_code = 0;
    int power, i;

    for (i = 0, power = 1; i < NUM_DIRS; ++i, power *= 3)
        ternary_code += (pattern[i] + 1) * power;
    return ternary_code;
}

float intensity(float *elevation, int pattern_size)
{
    /* calculate relative elevation of the central cell against its visibility surround */
    float sum_elevation = 0.;
    int i;

    for (i = 0; i < NUM_DIRS; i++)
        sum_elevation -= elevation[i];

    return sum_elevation / (float)pattern_size;
}

float exposition(float *elevation)
{
    /* calculate relative elevation of the central cell against its visibility */
    float max;
    int i;

    max = elevation[0];
    for (i = 1; i < NUM_DIRS; i++)
        if (fabs(elevation[i]) > fabs(max))
            max = elevation[i];
    return -max;
}

float range(float *elevation)
{
    /* calculate relative difference in visible range of central cell */
    float max, min;
    int i;

    max = min = elevation[0];
    for (i = 1; i < NUM_DIRS; i++) {
        max = MAX(elevation[i], max);
        min = MIN(elevation[i], min);
    }

    return max - min;
}

float variance(float *elevation, int pattern_size)
{
    /* calculate relative variation of the visible neighbourhood of the cell  */
    float sum_elevation = 0;
    float variance = 0;
    int i;

    for (i = 0; i < NUM_DIRS; i++)
        sum_elevation += elevation[i];
    sum_elevation /= (float)pattern_size;
    for (i = 0; i < NUM_DIRS; i++)
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

    for (i = 0; i < NUM_DIRS; ++i)
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

/*
 * Return area in square metres of the octagon of the geomorphon mesh
 * projection onto the horizontal plane.
 */
float extends(PATTERN * pattern)
{
    int i, j;
    float area = 0;

    for (i = 0, j = 1; i < NUM_DIRS; ++i, ++j) {
        j = j < NUM_DIRS ? j : 0;
        area +=
            (pattern->x[i] * pattern->y[j] - pattern->x[j] * pattern->y[i]);
    }
    return fabs(area) / 2.;
}

static double distance_3d(const double x1, const double y1, const double z1,
                          const double x2, const double y2, const double z2)
{
    const double dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

/*
 * Return perimeter length in metres of the same plane figure as above.
 */
double octa_perimeter(const PATTERN * p)
{
    unsigned i, j;
    double ret = 0.0;

    for (i = 0, j = 1; i < NUM_DIRS; ++i, ++j) {
        j = j < NUM_DIRS ? j : 0;
        ret += distance_3d(p->x[i], p->y[i], 0, p->x[j], p->y[j], 0);
    }
    return ret;
}

/*
 * Return perimeter length in metres of the geomorphon shape mesh.
 */
double mesh_perimeter(const PATTERN * p)
{
    unsigned i, j;
    double ret = 0.0;

    for (i = 0, j = 1; i < NUM_DIRS; ++i, ++j) {
        j = j < NUM_DIRS ? j : 0;
        ret += distance_3d(p->x[i], p->y[i], p->elevation[i],
                           p->x[j], p->y[j], p->elevation[j]);
    }
    return ret;
}

/*
 * Return area in square metres of the 8 triangles that constitute the
 * geomorphon mesh.
 */
double mesh_area(const PATTERN * p)
{
    unsigned i, j;
    double ret = 0.0;

    for (i = 0, j = 1; i < NUM_DIRS; ++i, ++j) {
        double a, b, c, s;

        j = j < NUM_DIRS ? j : 0;
        a = distance_3d(0, 0, 0, p->x[i], p->y[i], p->elevation[i]);
        b = distance_3d(0, 0, 0, p->x[j], p->y[j], p->elevation[j]);
        c = distance_3d(p->x[i], p->y[i], p->elevation[i],
                        p->x[j], p->y[j], p->elevation[j]);
        s = (a + b + c) / 2.0;
        /* Ready for the Heron's formula. */
        ret += sqrt(s * (s - a) * (s - b) * (s - c));
    }
    return ret;
}

int shape(PATTERN * pattern, int pattern_size, float *azimuth,
          float *elongation, float *width)
{
    /* calculates azimuth, elongation and width of geomorphon's polygon */
    int i;
    double avg_x = 0, avg_y = 0;
    double avg_x_y = 0;
    double avg_x_square = 0;
    double rx, ry;
    double sine, cosine;
    double result;
    double rxmin, rxmax, rymin, rymax;

    for (i = 0; i < NUM_DIRS; ++i) {
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
    rxmin = rxmax = pattern->x[0] * cosine - pattern->y[0] * sine;
    rymin = rymax = pattern->x[0] * sine + pattern->y[0] * cosine;
    for (i = 1; i < NUM_DIRS; ++i) {
        rx = pattern->x[i] * cosine - pattern->y[i] * sine;
        ry = pattern->x[i] * sine + pattern->y[i] * cosine;
        rxmin = MIN(rx, rxmin);
        rxmax = MAX(rx, rxmax);
        rymin = MIN(ry, rymin);
        rymax = MAX(ry, rymax);
    }
    rx = (rxmax - rxmin);
    ry = (rymax - rymin);
    *elongation = rx > ry ? (float)rx / ry : (float)ry / rx;
    *width = MIN(rx, ry);

    return 0;
}
