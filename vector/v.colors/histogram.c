#include <stdlib.h>
#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

struct Area {
    int cat;
    double area;
    double H, S, L;
};

static int compare_area_L(const void *, const void *);
static void rgb_to_hsl(int, int, int, double *, double *, double *);
static void hsl_to_rgb(double, double, double, int *, int *, int *);
#if 0
static void rgb_to_hsi(int, int, int, double *, double *, double *);
static void hsi_to_rgb(double, double, double, int *, int *, int *);
#endif

void histogram_eq_colors(struct Map_info *Map, int field, struct Colors *dst,
                         struct Colors *src)
{
    int red, grn, blu;
    double H, S, L, L_min, L_max;
    int num_areas, n_areas, cat, i;
    double total_area, sum_area;
    struct Area *areas;

    if (Vect_level(Map) < 2)
        G_fatal_error(_("Topology level required for histogram equalization."));

    num_areas = Vect_get_num_areas(Map);
    areas = (struct Area *)G_malloc(num_areas * sizeof(struct Area));
    total_area = 0;
    n_areas = 0;

    G_message(_("Reading features..."));

    for (i = 0; i < num_areas; i++) {
        int id = i + 1;
        double area;

        G_progress(i, num_areas);

        if (!Vect_area_alive(Map, id))
            continue;

        cat = Vect_get_area_cat(Map, id, field);
        area = Vect_get_area_area(Map, id);

        total_area += area;

        Rast_get_c_color(&cat, &red, &grn, &blu, src);

        rgb_to_hsl(red, grn, blu, &H, &S, &L);

        areas[n_areas].cat = cat;
        areas[n_areas].area = area;
        areas[n_areas].H = H;
        areas[n_areas].S = S;
        areas[n_areas].L = L;
        n_areas++;
    }

    G_progress(1, 1);

    qsort(areas, n_areas, sizeof(struct Area), compare_area_L);

    Rast_init_colors(dst);

    Rast_get_default_color(&red, &grn, &blu, src);
    Rast_set_default_color(red, grn, blu, dst);

    Rast_get_null_value_color(&red, &grn, &blu, src);
    Rast_set_null_value_color(red, grn, blu, dst);

    L_min = areas[0].L;
    L_max = areas[n_areas - 1].L;

    sum_area = total_area;

    G_message(_("Equalizing color histogram..."));

    for (i = n_areas - 1; i >= 0; i--) {
        G_progress(i, n_areas);

        /* L is evaluated only if areas[i].L changes */
        if (i == n_areas - 1 || areas[i].L < areas[i + 1].L)
            L = L_min + (L_max - L_min) * sum_area / total_area;

        cat = areas[i].cat;
        H = areas[i].H;
        S = areas[i].S;

        hsl_to_rgb(H, S, L, &red, &grn, &blu);
        Rast_set_c_color(cat, red, grn, blu, dst);

        sum_area -= areas[i].area;
    }

    G_progress(1, 1);
}

void histogram_eq_colors_bak(struct Map_info *Map, int field,
                             struct Colors *dst, struct Colors *src)
{
    int red, grn, blu;
    double H, S, L, L_min, L_max;
    int num_areas, n_areas, cat, i;
    double total_area, sum_area;
    struct Area *areas;

    if (Vect_level(Map) < 2)
        G_fatal_error(_("Topology level required for histogram equalization."));

    num_areas = Vect_get_num_areas(Map);
    areas = (struct Area *)G_malloc(num_areas * sizeof(struct Area));
    total_area = 0;
    n_areas = 0;

    G_message(_("Reading features..."));

    for (i = 0; i < num_areas; i++) {
        int id = i + 1;
        double area;

        G_progress(i, num_areas);

        if (!Vect_area_alive(Map, id))
            continue;

        cat = Vect_get_area_cat(Map, id, field);
        area = Vect_get_area_area(Map, id);

        total_area += area;

        Rast_get_c_color(&cat, &red, &grn, &blu, src);

        rgb_to_hsl(red, grn, blu, &H, &S, &L);

        areas[n_areas].cat = cat;
        areas[n_areas].area = area;
        areas[n_areas].H = H;
        areas[n_areas].S = S;
        areas[n_areas].L = L;
        n_areas++;
    }

    G_progress(1, 1);

    qsort(areas, n_areas, sizeof(struct Area), compare_area_L);

    Rast_init_colors(dst);

    Rast_get_default_color(&red, &grn, &blu, src);
    Rast_set_default_color(red, grn, blu, dst);

    Rast_get_null_value_color(&red, &grn, &blu, src);
    Rast_set_null_value_color(red, grn, blu, dst);

    L_min = areas[0].L;
    L_max = areas[n_areas - 1].L;

    sum_area = 0;

    G_message(_("Equalizing color histogram..."));

    for (i = 0; i < n_areas; i++) {
        G_progress(i, n_areas);

        cat = areas[i].cat;
        sum_area += areas[i].area;

        H = areas[i].H;
        S = areas[i].S;
        /* TODO? areas[i].L may not change, but L grows anyway */
        L = L_min + (L_max - L_min) * sum_area / total_area;

        hsl_to_rgb(H, S, L, &red, &grn, &blu);
        Rast_set_c_color(cat, red, grn, blu, dst);
    }

    G_progress(1, 1);
}

static int compare_area_L(const void *p1, const void *p2)
{
    const struct Area *area1 = p1;
    const struct Area *area2 = p2;
    double delta = area1->L - area2->L;

    return delta > 0 ? 1 : (delta < 0 ? -1 : 0);
}

static void rgb_to_hsl(int red, int grn, int blu, double *H, double *S,
                       double *L)
{
    double R, G, B;
    double M, m, C;

    R = red / 255.;
    G = grn / 255.;
    B = blu / 255.;

    M = fmax(R, fmax(G, B));
    m = fmin(R, fmin(G, B));
    C = M - m;

    if (C == 0.)
        *H = 0;
    else if (M == R)
        *H = ((G - B) / C) / 6.;
    else if (M == G)
        *H = ((B - R) / C + 2.) / 6.;
    else
        *H = ((R - G) / C + 4.) / 6.;

    *L = (M + m) / 2.;
    *S = C == 0. ? 0. : C / (1. - fabs(2 * *L - 1.));
}

static void hsl_to_rgb(double H, double S, double L, int *red, int *grn,
                       int *blu)
{
    double C, X, m;
    double R, G, B;

    H *= 6.;
    C = (1. - fabs(2. * L - 1.)) * S;
    X = C * (1. - fabs(fmod(H, 2.) - 1.));
    R = G = B = 0.;
    if (H <= 1.) {
        R = C;
        G = X;
    }
    else if (H <= 2.) {
        R = X;
        G = C;
    }
    else if (H <= 3.) {
        G = C;
        B = X;
    }
    else if (H <= 4.) {
        G = X;
        B = C;
    }
    else if (H <= 5.) {
        R = X;
        B = C;
    }
    else if (H <= 6.) {
        R = C;
        B = X;
    }
    m = L - C / 2.;

    R += m;
    G += m;
    B += m;

    /* + 0.5 for rounding */
    *red = R * 255. + 0.5;
    *grn = G * 255. + 0.5;
    *blu = B * 255. + 0.5;
}

#if 0
/* XXX: Unused for now, but let's keep them!
 * Why do we use HSL over HSI? In the HSI color space, an R, G, or B of 255 is
 * not guaranteed to produce a fully saturated color after histogram
 * equalization because its saturation may not be 1 depending on the intensity
 * even if the color is on its maximum surface. In the HSL color space, any
 * color that touches the maximum surface will get assigned a saturation of 1,
 * which in turn results in the same saturation 1 after histogram equalization.
 */
static void rgb_to_hsi(int red, int grn, int blu, double *H, double *S, double *I)
{
    double R, G, B;
    double theta;

    R = red / 255.;
    G = grn / 255.;
    B = blu / 255.;

    theta = acos((R - G + R - B) / 2. / (sqrt(pow(R - G, 2.) + (R - B) * (G - B)) + 1e-16));
    *H = (G >= B ? theta : 2. * M_PI - theta) / (2. * M_PI);
    *I = (R + G + B) / 3.;
    *S = 1. - fmin(R, fmin(G, B)) / *I;
}

static void hsi_to_rgb(double H, double S, double I, int *red, int *grn, int *blu)
{
    double R, G, B;

    H *= 2. * M_PI;

    if (H < M_PI / 3. * 2.) {
	B = I * (1. - S);
	R = I * (1. + S * cos(H) / cos(M_PI / 3. - H));
	G = 3. * I - (R + B);
    } else if (H < M_PI / 3. * 4.) {
	H -= M_PI / 3. * 2.;
	R = I * (1. - S);
	G = I * (1. + S * cos(H) / cos(M_PI / 3. - H));
	B = 3. * I - (R + G);
    } else {
	H -= M_PI / 3. * 4.;
	G = I * (1. - S);
	B = I * (1. + S * cos(H) / cos(M_PI / 3. - H));
	R = 3. * I - (G + B);
    }

    /* + 0.5 for rounding */
    *red = R * 255. + 0.5;
    *grn = G * 255. + 0.5;
    *blu = B * 255. + 0.5;
}
#endif
