#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define DEG2RAD(a) ((a) * M_PI / 180.0)
#define RAD2DEG(a) ((a) * 180.0 / M_PI)

static double get_shift(double east)
{
    double shift;

    shift = 0;
    while (east + shift > 180)
        shift -= 360;
    while (east + shift < -180)
        shift += 360;

    return shift;
}

void print_window(struct Cell_head *window, int print_flag, int flat_flag,
                  enum OutputFormat format, JSON_Object *root_object)
{
    const char *prj, *datum, *ellps;
    int x, width = 11;

    char north[30], south[30], east[30], west[30], nsres[30], ewres[30],
        nsres3[30], ewres3[30], tbres[30];
    char buf[50];
    char *sep = "\n";

    double d_nsres, d_ewres, d_nsres3, d_ewres3, d_tbres;
    double ew_dist1, ew_dist2, ns_dist1, ns_dist2;
    double longitude, latitude;

    JSON_Value *region_value;
    JSON_Object *region;

    if (print_flag & PRINT_SH) {
        x = G_projection() == PROJECTION_LL ? -1 : 0;
        if (flat_flag)
            sep = " ";
    }
    else
        x = window->proj;

    G_format_northing(window->north, north, x);
    G_format_northing(window->south, south, x);
    G_format_easting(window->east, east, x);
    G_format_easting(window->west, west, x);

    d_ewres = window->ew_res;
    d_ewres3 = window->ew_res3;
    d_nsres = window->ns_res;
    d_nsres3 = window->ns_res3;
    d_tbres = window->tb_res;
    G_format_resolution(window->ew_res, ewres, x);
    G_format_resolution(window->ew_res3, ewres3, x);
    G_format_resolution(window->ns_res, nsres, x);
    G_format_resolution(window->ns_res3, nsres3, x);
    G_format_resolution(window->tb_res, tbres, -1);
    G_begin_distance_calculations();

    /* EW Dist at North edge */
    ew_dist1 =
        G_distance(window->east, window->north, window->west, window->north);
    /* EW Dist at South Edge */
    ew_dist2 =
        G_distance(window->east, window->south, window->west, window->south);
    /* NS Dist at East edge */
    ns_dist1 =
        G_distance(window->east, window->north, window->east, window->south);
    /* NS Dist at West edge */
    ns_dist2 =
        G_distance(window->west, window->north, window->west, window->south);

    /*  width */
    if (print_flag & PRINT_REG)
        width = 11;

    if (print_flag & PRINT_CENTER || print_flag & PRINT_MBBOX)
        width = 16;

    if (print_flag & PRINT_LL || print_flag & PRINT_NANGLE)
        width = 18;

    if (print_flag & PRINT_EXTENT)
        width = 19;

    /* flag.dist_res */
    if (print_flag & PRINT_METERS) {
        d_ewres = ((ew_dist1 + ew_dist2) / 2) / window->cols;
        snprintf(ewres, sizeof(ewres), "%.8f", d_ewres);
        G_trim_decimal(ewres);
        d_ewres3 = ((ew_dist1 + ew_dist2) / 2) / window->cols3;
        snprintf(ewres3, sizeof(ewres3), "%.8f", d_ewres3);
        G_trim_decimal(ewres3);
        d_nsres = ((ns_dist1 + ns_dist2) / 2) / window->rows;
        snprintf(nsres, sizeof(nsres), "%.8f", d_nsres);
        G_trim_decimal(nsres);
        d_nsres3 = ((ns_dist1 + ns_dist2) / 2) / window->rows3;
        snprintf(nsres3, sizeof(nsres3), "%.8f", d_nsres3);
        G_trim_decimal(nsres3);
        d_tbres = (window->top - window->bottom) / window->depths;
        snprintf(tbres, sizeof(tbres), "%.8f", d_tbres);
        G_trim_decimal(tbres);
    }

    /* flag.print & flag.gprint */
    if (print_flag & PRINT_REG) {
        prj = G_database_projection_name();
        if (!prj)
            prj = "** unknown **";

        switch (format) {
        case SHELL:
            fprintf(stdout, "projection=%d%s", window->proj, sep);
            fprintf(stdout, "zone=%d%s", window->zone, sep);
            break;
        case PLAIN:
            fprintf(stdout, "%-*s %d (%s)\n", width,
                    "projection:", window->proj, prj);
            fprintf(stdout, "%-*s %d\n", width, "zone:", window->zone);
            break;
        case JSON:
            json_object_dotset_number(root_object, "projection.code",
                                      window->proj);
            json_object_dotset_string(root_object, "projection.name", prj);
            json_object_set_number(root_object, "zone", window->zone);
            break;
        }

        /* don't print datum/ellipsoid in XY-Locations */
        if (window->proj != 0) {
            datum = G_database_datum_name();
            if (!datum)
                datum = "** unknown (default: WGS84) **";
            ellps = G_database_ellipse_name();
            if (!ellps)
                ellps = "** unknown (default: WGS84) **";

            /*
               please remove before GRASS 7 released
               backward compatibility issue

               if (print_flag & PRINT_SH)
               {
               if (datum[0] != '*')
               fprintf(stdout, "datum=%s%s", datum, sep);
               else
               fprintf(stdout, "datum=wgs84%s", sep);
               if (ellps[0] != '*')
               fprintf(stdout, "ellipsoid=%s%s", ellps, sep);
               else
               fprintf(stdout, "ellipsoid=wgs84%s", sep);
               }
               else
               {
               fprintf(stdout, "%-*s %s\n", width, "datum:", datum);
               fprintf(stdout, "%-*s %s\n", width, "ellipsoid:", ellps);
               }
             */

            switch (format) {
            case JSON:
                json_object_set_string(root_object, "datum", datum);
                json_object_set_string(root_object, "ellipsoid", ellps);
                break;
            default:
                if (!(print_flag & PRINT_SH)) {
                    fprintf(stdout, "%-*s %s\n", width, "datum:", datum);
                    fprintf(stdout, "%-*s %s\n", width, "ellipsoid:", ellps);
                }
                break;
            }
        }

        switch (format) {
        case SHELL:
            fprintf(stdout, "n=%s%s", north, sep);
            fprintf(stdout, "s=%s%s", south, sep);
            fprintf(stdout, "w=%s%s", west, sep);
            fprintf(stdout, "e=%s%s", east, sep);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "t=%g%s", window->top, sep);
                fprintf(stdout, "b=%g%s", window->bottom, sep);
            }
            fprintf(stdout, "nsres=%s%s", nsres, sep);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "nsres3=%s%s", nsres3, sep);
            }
            fprintf(stdout, "ewres=%s%s", ewres, sep);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "ewres3=%s%s", ewres3, sep);
                fprintf(stdout, "tbres=%s%s", tbres, sep);
            }
            fprintf(stdout, "rows=%d%s", window->rows, sep);
            if (print_flag & PRINT_3D)
                fprintf(stdout, "rows3=%d%s", window->rows3, sep);
            fprintf(stdout, "cols=%d%s", window->cols, sep);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "cols3=%d%s", window->cols3, sep);
                fprintf(stdout, "depths=%d%s", window->depths, sep);
            }
#ifdef HAVE_LONG_LONG_INT
            fprintf(stdout, "cells=%lld%s",
                    (long long)window->rows * window->cols, sep);
            if (print_flag & PRINT_3D)
                fprintf(stdout, "cells3=%lld%s",
                        (long long)window->rows3 * window->cols3 *
                            window->depths,
                        sep);
#else
            fprintf(stdout, "cells=%ld%s", (long)window->rows * window->cols,
                    sep);
            if (print_flag & PRINT_3D)
                fprintf(stdout, "cells3=%ld%s",
                        (long)window->rows3 * window->cols3 * window->depths,
                        sep);
#endif
            break;
        case PLAIN:
            fprintf(stdout, "%-*s %s\n", width, "north:", north);
            fprintf(stdout, "%-*s %s\n", width, "south:", south);
            fprintf(stdout, "%-*s %s\n", width, "west:", west);
            fprintf(stdout, "%-*s %s\n", width, "east:", east);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "%-*s %.8f\n", width, "top:", window->top);
                fprintf(stdout, "%-*s %.8f\n", width,
                        "bottom:", window->bottom);
            }
            fprintf(stdout, "%-*s %s\n", width, "nsres:", nsres);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "%-*s %s\n", width, "nsres3:", nsres3);
            }
            fprintf(stdout, "%-*s %s\n", width, "ewres:", ewres);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "%-*s %s\n", width, "ewres3:", ewres3);
                fprintf(stdout, "%-*s %s\n", width, "tbres:", tbres);
            }

            fprintf(stdout, "%-*s %d\n", width, "rows:", window->rows);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "%-*s %d\n", width, "rows3:", window->rows3);
            }
            fprintf(stdout, "%-*s %d\n", width, "cols:", window->cols);
            if (print_flag & PRINT_3D) {
                fprintf(stdout, "%-*s %d\n", width, "cols3:", window->cols3);
                fprintf(stdout, "%-*s %d\n", width, "depths:", window->depths);
            }
#ifdef HAVE_LONG_LONG_INT
            fprintf(stdout, "%-*s %lld\n", width,
                    "cells:", (long long)window->rows * window->cols);
            if (print_flag & PRINT_3D)
                fprintf(stdout, "%-*s %lld\n", width, "cells3:",
                        (long long)window->rows3 * window->cols3 *
                            window->depths);
#else
            fprintf(stdout, "%-*s %ld\n", width,
                    "cells:", (long)window->rows * window->cols);
            if (print_flag & PRINT_3D)
                fprintf(stdout, "%-*s %ld\n", width, "cells3:",
                        (long)window->rows3 * window->cols3 * window->depths);
#endif
            break;
        case JSON:
            region_value = json_value_init_object();
            region = json_object(region_value);
            json_object_set_number(region, "north", window->north);
            json_object_set_number(region, "south", window->south);
            json_object_set_number(region, "west", window->west);
            json_object_set_number(region, "east", window->east);
            json_object_set_number(region, "ns-res", d_nsres);
            json_object_set_number(region, "ns-res3", d_nsres3);
            json_object_set_number(region, "ew-res", d_ewres);
            json_object_set_number(region, "ew-res3", d_ewres3);
            json_object_set_value(root_object, "region", region_value);
            json_object_set_number(root_object, "top", window->top);
            json_object_set_number(root_object, "bottom", window->bottom);
            json_object_set_number(root_object, "tbres", d_tbres);
            json_object_set_number(root_object, "rows", window->rows);
            json_object_set_number(root_object, "rows3", window->rows3);
            json_object_set_number(root_object, "cols", window->cols);
            json_object_set_number(root_object, "cols3", window->cols3);
            json_object_set_number(root_object, "depths", window->depths);

#ifdef HAVE_LONG_LONG_INT
            json_object_set_number(root_object, "cells",
                                   (long long)window->rows * window->cols);
            json_object_set_number(root_object, "cells3",
                                   (long long)window->rows3 * window->cols3 *
                                       window->depths);
#else
            json_object_set_number(root_object, "cells",
                                   (long)window->rows * window->cols);
            json_object_set_number(root_object, "cells3",
                                   (long)window->rows3 * window->cols3 *
                                       window->depths);
#endif
            break;
        }
    }

    /* flag.lprint: show corners and center in lat/long  MN 2001 */
    if (print_flag & PRINT_LL) {
        double lo1, la1, lo2, la2, lo3, la3, lo4, la4, loc, lac;

        /* if coordinates are not in lat/long format, transform them: */
        if ((G_projection() != PROJECTION_LL) && window->proj != 0) {
            /* projection information of input map */
            struct Key_Value *in_proj_info, *in_unit_info;
            struct pj_info iproj, oproj, tproj; /* proj parameters  */

            /* read current projection info */
            if ((in_proj_info = G_get_projinfo()) == NULL)
                G_fatal_error(
                    _("Can't get projection info of current project"));

            if ((in_unit_info = G_get_projunits()) == NULL)
                G_fatal_error(
                    _("Can't get projection units of current project"));

            if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
                G_fatal_error(
                    _("Can't get projection key values of current project"));

            G_free_key_value(in_proj_info);
            G_free_key_value(in_unit_info);

            oproj.pj = NULL;
            tproj.def = NULL;

            if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
                G_fatal_error(
                    _("Unable to initialize coordinate transformation"));

            /*
             *  1 ------ 2
             *  |        |  map corners
             *  |        |
             *  4--------3
             */

            latitude = window->north;
            longitude = window->west;
            /* get lat/long w/ same datum/ellipsoid as input */
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                              &latitude, NULL) < 0)
                G_fatal_error(
                    _("Error in %s (projection of input coordinate pair)"),
                    "GPJ_transform()");

            lo1 = longitude;
            la1 = latitude;

            latitude = window->north;
            longitude = window->east;
            /* get lat/long w/ same datum/ellipsoid as input */
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                              &latitude, NULL) < 0)
                G_fatal_error(
                    _("Error in %s (projection of input coordinate pair)"),
                    "GPJ_transform()");

            lo2 = longitude;
            la2 = latitude;

            latitude = window->south;
            longitude = window->east;
            /* get lat/long w/ same datum/ellipsoid as input */
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                              &latitude, NULL) < 0)
                G_fatal_error(
                    _("Error in %s (projection of input coordinate pair)"),
                    "GPJ_transform()");

            lo3 = longitude;
            la3 = latitude;

            latitude = window->south;
            longitude = window->west;
            /* get lat/long w/ same datum/ellipsoid as input */
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                              &latitude, NULL) < 0)
                G_fatal_error(
                    _("Error in %s (projection of input coordinate pair)"),
                    "GPJ_transform()");

            lo4 = longitude;
            la4 = latitude;

            /* center coordinates of the current region,
             * not average of the projected corner coordinates */
            latitude = (window->north + window->south) / 2.;
            longitude = (window->west + window->east) / 2.;
            /* get lat/long w/ same datum/ellipsoid as input */
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                              &latitude, NULL) < 0)
                G_fatal_error(
                    _("Error in %s (projection of input coordinate pair)"),
                    "GPJ_transform()");

            loc = longitude;
            lac = latitude;

            switch (format) {
            case SHELL:
                fprintf(stdout, "nw_long=%.8f%snw_lat=%.8f%s", lo1, sep, la1,
                        sep);
                fprintf(stdout, "ne_long=%.8f%sne_lat=%.8f%s", lo2, sep, la2,
                        sep);
                fprintf(stdout, "se_long=%.8f%sse_lat=%.8f%s", lo3, sep, la3,
                        sep);
                fprintf(stdout, "sw_long=%.8f%ssw_lat=%.8f%s", lo4, sep, la4,
                        sep);
                fprintf(stdout, "center_long=%.8f%s", loc, sep);
                fprintf(stdout, "center_lat=%.8f%s", lac, sep);
                break;
            case PLAIN:
                G_format_easting(lo1, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s long: %s ", width,
                        "north-west corner:", buf);
                G_format_northing(la1, buf, PROJECTION_LL);
                fprintf(stdout, "lat: %s\n", buf);

                G_format_easting(lo2, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s long: %s ", width,
                        "north-east corner:", buf);
                G_format_northing(la2, buf, PROJECTION_LL);
                fprintf(stdout, "lat: %s\n", buf);

                G_format_easting(lo3, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s long: %s ", width,
                        "south-east corner:", buf);
                G_format_northing(la3, buf, PROJECTION_LL);
                fprintf(stdout, "lat: %s\n", buf);

                G_format_easting(lo4, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s long: %s ", width,
                        "south-west corner:", buf);
                G_format_northing(la4, buf, PROJECTION_LL);
                fprintf(stdout, "lat: %s\n", buf);

                G_format_easting(loc, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s %11s\n", width, "center longitude:", buf);

                G_format_northing(lac, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s %11s\n", width, "center latitude:", buf);
                break;
            case JSON:
                json_object_set_number(root_object, "nw_long", lo1);
                json_object_set_number(root_object, "nw_lat", la1);
                json_object_set_number(root_object, "ne_long", lo2);
                json_object_set_number(root_object, "ne_lat", la2);
                json_object_set_number(root_object, "se_long", lo3);
                json_object_set_number(root_object, "se_lat", la3);
                json_object_set_number(root_object, "sw_long", lo4);
                json_object_set_number(root_object, "sw_lat", la4);
                json_object_set_number(root_object, "center_long", loc);
                json_object_set_number(root_object, "center_lat", lac);
                break;
            }

            if (!(print_flag & PRINT_REG)) {
                switch (format) {
                case SHELL:
                    fprintf(stdout, "rows=%d%s", window->rows, sep);
                    fprintf(stdout, "cols=%d%s", window->cols, sep);
                    break;
                case PLAIN:
                    fprintf(stdout, "%-*s %d\n", width, "rows:", window->rows);
                    fprintf(stdout, "%-*s %d\n", width, "cols:", window->cols);
                    break;
                case JSON:
                    json_object_set_number(root_object, "rows", window->rows);
                    json_object_set_number(root_object, "cols", window->cols);
                    break;
                }
            }
        }
        else { /* in lat/long already */

            if (window->proj != 0)
                G_message(
                    _("You are already in Lat/Long. Use the -p flag instead."));
            else
                G_message(
                    _("You are in a simple XY project, projection to Lat/Lon "
                      "is not possible. Use the -p flag instead."));
        }
    }

    /* flag.eprint */
    if (print_flag & PRINT_EXTENT) {
        switch (format) {
        case SHELL:
            fprintf(stdout, "ns_extent=%f%s", window->north - window->south,
                    sep);
            fprintf(stdout, "ew_extent=%f%s", window->east - window->west, sep);
            break;
        case PLAIN:
            if (G_projection() != PROJECTION_LL) {
                fprintf(stdout, "%-*s %f\n", width,
                        "north-south extent:", window->north - window->south);
                fprintf(stdout, "%-*s %f\n", width,
                        "east-west extent:", window->east - window->west);
            }
            else {
                G_format_northing(window->north - window->south, buf,
                                  PROJECTION_LL);
                fprintf(stdout, "%-*s %s\n", width, "north-south extent:", buf);
                G_format_easting(window->east - window->west, buf,
                                 PROJECTION_LL);
                fprintf(stdout, "%-*s %s\n", width, "east-west extent:", buf);
            }
            break;
        case JSON:
            json_object_set_number(root_object, "ns_extent",
                                   window->north - window->south);
            json_object_set_number(root_object, "ew_extent",
                                   window->east - window->west);
            break;
        }
    }

    /* flag.center */
    if (print_flag & PRINT_CENTER) {
        switch (format) {
        case SHELL:
            fprintf(stdout, "center_easting=%f%s",
                    (window->west + window->east) / 2., sep);
            fprintf(stdout, "center_northing=%f%s",
                    (window->north + window->south) / 2., sep);
            break;
        case PLAIN:
            if (G_projection() != PROJECTION_LL) {
                fprintf(stdout, "%-*s %f\n", width,
                        "center easting:", (window->west + window->east) / 2.);
                fprintf(stdout, "%-*s %f\n", width, "center northing:",
                        (window->north + window->south) / 2.);
            }
            else {
                G_format_northing((window->north + window->south) / 2., buf,
                                  PROJECTION_LL);
                fprintf(stdout, "%-*s %s\n", width, "north-south center:", buf);
                G_format_easting((window->west + window->east) / 2., buf,
                                 PROJECTION_LL);
                fprintf(stdout, "%-*s %s\n", width, "east-west center:", buf);
            }
            break;
        case JSON:
            json_object_set_number(root_object, "center_easting",
                                   (window->west + window->east) / 2.);
            json_object_set_number(root_object, "center_northing",
                                   (window->north + window->south) / 2.);
            break;
        }
    }

    /* flag.gmt_style */
    if (print_flag & PRINT_GMT) {
        char gmt[120];
        switch (format) {
        case JSON:
            snprintf(gmt, 120, "%s/%s/%s/%s", west, east, south, north);
            json_object_set_string(root_object, "GMT", gmt);
            break;
        default:
            fprintf(stdout, "%s/%s/%s/%s\n", west, east, south, north);
            break;
        }
    }

    /* flag.wms_style */
    if (print_flag & PRINT_WMS) {
        char wms[150];
        switch (format) {
        case JSON:
            snprintf(wms, 150, "bbox=%s,%s,%s,%s", west, south, east, north);
            json_object_set_string(root_object, "WMS", wms);
            break;
        default:
            G_format_northing(window->north, north, -1);
            G_format_northing(window->south, south, -1);
            G_format_easting(window->east, east, -1);
            G_format_easting(window->west, west, -1);
            fprintf(stdout, "bbox=%s,%s,%s,%s%s", west, south, east, north,
                    sep);
            break;
        }
    }

    /* flag.nangle */
    if (print_flag & PRINT_NANGLE) {
        double convergence;

        if (G_projection() == PROJECTION_XY)
            convergence = NAN;
        else if (G_projection() == PROJECTION_LL)
            convergence = 0.0;
        else {
            struct Key_Value *in_proj_info, *in_unit_info;

            /* proj parameters  */
            struct pj_info iproj, oproj, tproj;

#ifdef HAVE_PROJ_H
            PJ_COORD c;
            PJ_FACTORS fact;
#else
            struct FACTORS fact;
            LP lp;
#endif

            /* read current projection info */
            if ((in_proj_info = G_get_projinfo()) == NULL)
                G_fatal_error(
                    _("Can't get projection info of current project"));

            if ((in_unit_info = G_get_projunits()) == NULL)
                G_fatal_error(
                    _("Can't get projection units of current project"));

            if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
                G_fatal_error(
                    _("Can't get projection key values of current project"));

            G_free_key_value(in_proj_info);
            G_free_key_value(in_unit_info);

            oproj.pj = NULL;
            tproj.def = NULL;

            if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
                G_fatal_error(
                    _("Unable to initialize coordinate transformation"));

            /* center coordinates of the current region,
             * not average of the projected corner coordinates */
            latitude = (window->north + window->south) / 2.;
            longitude = (window->west + window->east) / 2.;
            /* get lat/long w/ same datum/ellipsoid as input */
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                              &latitude, NULL) < 0)
                G_fatal_error(
                    _("Error in %s (projection of input coordinate pair)"),
                    "GPJ_transform()");

#ifdef HAVE_PROJ_H
            c.lpzt.lam = DEG2RAD(longitude);
            c.lpzt.phi = DEG2RAD(latitude);
            c.lpzt.z = 0;
            c.lpzt.t = 0;
            fact = proj_factors(iproj.pj, c);
            convergence = RAD2DEG(fact.meridian_convergence);
#else
            G_zero(&fact, sizeof(struct FACTORS));
            lp.u = DEG2RAD(longitude);
            lp.v = DEG2RAD(latitude);
            pj_factors(lp, iproj.pj, 0.0, &fact);
            convergence = RAD2DEG(fact.conv);
#endif
        }

        switch (format) {
        case SHELL:
            fprintf(stdout, "converge_angle=%f%s", convergence, sep);
            break;
        case PLAIN:
            fprintf(stdout, "%-*s %f\n", width,
                    "convergence angle:", convergence);
            break;
        case JSON:
            json_object_set_number(root_object, "converge_angle", convergence);
        }
    }

    /* flag.bbox
       Calculate the largest bounding box in lat-lon coordinates
       and print it to stdout
     */
    if (print_flag & PRINT_MBBOX) {
        double sh_ll_w, sh_ll_e, sh_ll_n, sh_ll_s, loc;

        /* Needed to calculate the LL bounding box */
        if ((G_projection() != PROJECTION_XY)) {
            /* projection information of input and output map */
            struct Key_Value *in_proj_info, *in_unit_info, *out_proj_info,
                *out_unit_info;
            struct pj_info iproj; /* input map proj parameters  */
            struct pj_info oproj; /* output map proj parameters  */
            struct pj_info tproj; /* transformation parameters  */
            int r, c;

            /* read current projection info */
            if ((in_proj_info = G_get_projinfo()) == NULL)
                G_fatal_error(
                    _("Can't get projection info of current project"));
            /* do not wrap to -180, 180, otherwise east can be < west */
            /* TODO: for PROJ 6+, the +over switch must be added to the
             * transformation pipeline if authority:name or WKt are used
             * as crs definition */
            G_set_key_value("over", "defined", in_proj_info);

            if ((in_unit_info = G_get_projunits()) == NULL)
                G_fatal_error(
                    _("Can't get projection units of current project"));

            if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
                G_fatal_error(
                    _("Can't get projection key values of current project"));

            /*  output projection to lat/long and wgs84 ellipsoid */
            out_proj_info = G_create_key_value();
            out_unit_info = G_create_key_value();

            G_set_key_value("proj", "ll", out_proj_info);

#if PROJ_VERSION_MAJOR < 6
            char buff[100], dum[100];

            /* PROJ6+ has its own datum transformation parameters */
            if (G_get_datumparams_from_projinfo(in_proj_info, buff, dum) < 0)
                G_fatal_error(_(
                    "WGS84 output not possible as this project does not "
                    "contain "
                    "datum transformation parameters. Try running g.setproj."));
            else
#endif
                G_set_key_value("datum", "wgs84", out_proj_info);

            G_set_key_value("unit", "degree", out_unit_info);
            G_set_key_value("units", "degrees", out_unit_info);
            G_set_key_value("meters", "1.0", out_unit_info);

            if (pj_get_kv(&oproj, out_proj_info, out_unit_info) < 0)
                G_fatal_error(
                    _("Unable to update lat/long projection parameters"));

            G_free_key_value(in_proj_info);
            G_free_key_value(in_unit_info);
            G_free_key_value(out_proj_info);
            G_free_key_value(out_unit_info);

            tproj.def = NULL;

            if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
                G_fatal_error(
                    _("Unable to initialize coordinate transformation"));

            /*Calculate the largest bounding box */

            /* center */
            latitude = (window->north + window->south) / 2.;
            longitude = (window->west + window->east) / 2.;
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                              &latitude, NULL) < 0)
                G_fatal_error(
                    _("Error in %s (projection of input coordinate pair)"),
                    "GPJ_transform()");

            sh_ll_w = sh_ll_e = longitude;
            sh_ll_n = sh_ll_s = latitude;

            /* western and eastern border */
            for (r = 0; r <= window->rows; r++) {
                latitude = window->north - r * window->ns_res;
                if (r == window->rows)
                    latitude = window->south;
                longitude = window->west;
                if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                                  &latitude, NULL) < 0)
                    G_fatal_error(
                        _("Error in %s (projection of input coordinate pair)"),
                        "GPJ_transform()");

                if (sh_ll_n < latitude)
                    sh_ll_n = latitude;
                if (sh_ll_s > latitude)
                    sh_ll_s = latitude;

                if (sh_ll_e < longitude)
                    sh_ll_e = longitude;
                if (sh_ll_w > longitude)
                    sh_ll_w = longitude;

                latitude = window->north - r * window->ns_res;
                if (r == window->rows)
                    latitude = window->south;
                longitude = window->east;
                if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                                  &latitude, NULL) < 0)
                    G_fatal_error(
                        _("Error in %s (projection of input coordinate pair)"),
                        "GPJ_transform()");

                if (sh_ll_n < latitude)
                    sh_ll_n = latitude;
                if (sh_ll_s > latitude)
                    sh_ll_s = latitude;

                if (sh_ll_e < longitude)
                    sh_ll_e = longitude;
                if (sh_ll_w > longitude)
                    sh_ll_w = longitude;
            }

            /* northern and southern border */
            for (c = 1; c < window->cols; c++) {
                latitude = window->north;
                longitude = window->west + c * window->ew_res;
                if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                                  &latitude, NULL) < 0)
                    G_fatal_error(
                        _("Error in %s (projection of input coordinate pair)"),
                        "GPJ_transform()");

                if (sh_ll_n < latitude)
                    sh_ll_n = latitude;
                if (sh_ll_s > latitude)
                    sh_ll_s = latitude;

                if (sh_ll_e < longitude)
                    sh_ll_e = longitude;
                if (sh_ll_w > longitude)
                    sh_ll_w = longitude;

                latitude = window->south;
                longitude = window->west + c * window->ew_res;
                if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD, &longitude,
                                  &latitude, NULL) < 0)
                    G_fatal_error(
                        _("Error in %s (projection of input coordinate pair)"),
                        "GPJ_transform()");

                if (sh_ll_n < latitude)
                    sh_ll_n = latitude;
                if (sh_ll_s > latitude)
                    sh_ll_s = latitude;

                if (sh_ll_e < longitude)
                    sh_ll_e = longitude;
                if (sh_ll_w > longitude)
                    sh_ll_w = longitude;
            }

            loc = (sh_ll_e + sh_ll_w) / 2.;
            loc += get_shift(loc);
            sh_ll_w += get_shift(sh_ll_w);
            sh_ll_e += get_shift(sh_ll_e);

            /* print the largest bounding box */
            switch (format) {
            case SHELL:
                fprintf(stdout, "ll_n=%.8f%s", sh_ll_n, sep);
                fprintf(stdout, "ll_s=%.8f%s", sh_ll_s, sep);
                fprintf(stdout, "ll_w=%.8f%s", sh_ll_w, sep);
                fprintf(stdout, "ll_e=%.8f%s", sh_ll_e, sep);
                /* center of the largest bounding box */
                fprintf(stdout, "ll_clon=%.8f%s", loc, sep);
                fprintf(stdout, "ll_clat=%.8f%s", (sh_ll_n + sh_ll_s) / 2.,
                        sep);
                break;
            case PLAIN:
                G_format_northing(sh_ll_n, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s  %s\n", width, "north latitude:", buf);
                G_format_northing(sh_ll_s, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s  %s\n", width, "south latitude:", buf);
                G_format_easting(sh_ll_w, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s  %s\n", width, "west longitude:", buf);
                G_format_easting(sh_ll_e, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s  %s\n", width, "east longitude:", buf);
                /* center of the largest bounding box */
                G_format_easting(loc, buf, PROJECTION_LL);
                fprintf(stdout, "%-*s %s\n", width, "center longitude:", buf);
                G_format_northing((sh_ll_n + sh_ll_s) / 2., buf, PROJECTION_LL);
                fprintf(stdout, "%-*s  %s\n", width, "center latitude:", buf);
                break;
            case JSON:
                json_object_set_number(root_object, "ll_n", sh_ll_n);
                json_object_set_number(root_object, "ll_s", sh_ll_s);
                json_object_set_number(root_object, "ll_w", sh_ll_w);
                json_object_set_number(root_object, "ll_e", sh_ll_e);
                /* center of the largest bounding box */
                json_object_set_number(root_object, "ll_clon", loc);
                json_object_set_number(root_object, "ll_clat",
                                       (sh_ll_n + sh_ll_s) / 2.);
            }

            /*It should be calculated which number of rows and cols we have in
             * LL */
            /*
               fprintf (stdout, "LL_ROWS=%f \n",sh_ll_rows);
               fprintf (stdout, "LL_COLS=%f \n",sh_ll_cols);
             */
        }
        else {
            G_warning(_("Lat/Long calculations are not possible from a simple "
                        "XY system"));
        }
    }

    if (flat_flag)
        fprintf(stdout, "\n");
}
