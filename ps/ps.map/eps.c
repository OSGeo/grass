#include <math.h>
#include <string.h>
#include <grass/glocale.h>
#include "local_proto.h"

/*  test if file is really EPS file and find bbox 
 *  returns  1 if OK
 *           0 on error               
 */
int eps_bbox(char *eps, double *llx, double *lly, double *urx, double *ury)
{
    char buf[201];
    FILE *fp;
    int v1, v2, v3, v4;

    /* test if file is really eps and find bbox */
    if ((fp = fopen(eps, "r")) == NULL) {
	G_warning(_("Can't open eps file <%s>"), eps);
	return (0);
    }
    /* test if first row contains '%!PS-Adobe-m.n EPSF-m.n' string */
    fgets(buf, 200, fp);
    if (sscanf(buf, "%%!PS-Adobe-%d.%d EPSF-%d.%d", &v1, &v2, &v3, &v4) < 4) {
	fprintf(stderr, "file <%s> is not in EPS format\n", eps);
	fclose(fp);
	return (0);
    }
    /* looking for bbox */
    while (fgets(buf, 200, fp) != NULL) {
	if (sscanf
	    (buf, "%%%%BoundingBox: %lf %lf %lf %lf", llx, lly, urx,
	     ury) == 4) {
	    fclose(fp);
	    return (1);
	}
    }
    G_warning(_("Bounding box in eps file <%s> was not found"), eps);
    fclose(fp);
    return (0);
}

/*  calculate translation for EPS file 
 * rotate is in degrees
 */
int eps_trans(double llx, double lly, double urx, double ury,
	      double x, double y, double scale, double rotate,
	      double *xt, double *yt)
{
    double xc, yc, angle;

    xc = (llx + urx) / 2;
    yc = (lly + ury) / 2;

    angle = M_PI * rotate / 180;
    *xt = x + scale * (yc * sin(angle) - xc * cos(angle));
    *yt = y - scale * (yc * cos(angle) + xc * sin(angle));

    return (1);
}

/* save EPS file into PS file for later use */
int eps_save(FILE * fp, char *epsf, char *name)
{
    char buf[1024];
    FILE *epsfp;

    if ((epsfp = fopen(epsf, "r")) == NULL)
	return (0);

    fprintf(fp, "\n/%s {\n", name);
    while (fgets(buf, 1024, epsfp) != NULL)
	fprintf(fp, "%s", buf);
    fprintf(fp, "} def\n");
    fclose(epsfp);

    return (1);
}

/* draw EPS file saved by eps_save */
int eps_draw_saved(char *name, double x, double y, double scale,
		   double rotate)
{
    fprintf(PS.fp, "\nBeginEPSF\n");
    fprintf(PS.fp, "%.5f %.5f translate\n", x, y);
    fprintf(PS.fp, "%.5f rotate\n", rotate);
    fprintf(PS.fp, "%.5f %.5f scale\n", scale, scale);
    fprintf(PS.fp, "%%BeginDocument: %s\n", name);

    fprintf(PS.fp, "%s\n", name);

    fprintf(PS.fp, "%%EndDocument\n");
    fprintf(PS.fp, "EndEPSF\n");

    return (1);
}


/* write EPS file into PS file */
int eps_draw(FILE * fp, char *eps, double x, double y, double scale,
	     double rotate)
{
    char buf[1024];
    FILE *epsfp;

    if ((epsfp = fopen(eps, "r")) == NULL)
	return (0);

    fprintf(PS.fp, "\nBeginEPSF\n");
    fprintf(PS.fp, "%.5f %.5f translate\n", x, y);
    fprintf(PS.fp, "%.5f rotate\n", rotate);
    fprintf(PS.fp, "%.5f %.5f scale\n", scale, scale);
    fprintf(PS.fp, "%%BeginDocument: %s\n", eps);

    while (fgets(buf, 1024, epsfp) != NULL)
	fprintf(fp, "%s", buf);

    fprintf(PS.fp, "%%EndDocument\n");
    fprintf(PS.fp, "EndEPSF\n");
    fclose(epsfp);

    return (1);
}

/* save EPS patter file into PS file for later use */
/* For pattern we have to remove header comments */
int pat_save(FILE * fp, char *epsf, char *name)
{
    char buf[1024];
    FILE *epsfp;

    if ((epsfp = fopen(epsf, "r")) == NULL)
	return (0);

    fprintf(fp, "\n/%s {\n", name);
    while (fgets(buf, 1024, epsfp) != NULL) {
	if (strncmp(buf, "%!PS-Adobe", 10) == 0 ||
	    strncmp(buf, "%%BoundingBox", 13) == 0)
	    continue;
	fprintf(fp, "%s", buf);
    }
    fprintf(fp, "} def\n");
    fclose(epsfp);

    return (1);
}
