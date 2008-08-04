
#include "pv.h"

void vrml_begin(FILE * vout)
{

#ifdef VRML2
    vrml_putline(0, vout, "#VRML V2.0 utf8");
#else
    vrml_putline(0, vout, "#VRML V1.0 ascii");
    vrml_putline(1, vout, "Separator {");
    vrml_putline(1, vout, "ShapeHints {");
    vrml_putline(0, vout, "vertexOrdering  COUNTERCLOCKWISE");
    vrml_putline(0, vout, "faceType        CONVEX");
    vrml_putline(0, vout, "creaseAngle     0.5");
    vrml_putline(-1, vout, "}");
#endif

}

void vrml_end(FILE * vout)
{
#ifdef VRML2
#else
    vrml_putline(-1, vout, "}");
#endif

}

/* To make it easier to read - uses tabs to increase or
   decrease the level of indentation.  For better storage,
   just comment out the tab printing.
 */

void vrml_putline(int indent, FILE * vout, char *str)
{
    static int ind = 0;
    int i;

    if (indent < 0)
	ind += indent;		/* pre-decrement */

    for (i = 0; i < ind; i++)
	fprintf(vout, "\t");
    fprintf(vout, "%s\n", str);

    if (indent > 0)
	ind += indent;		/* post-increment */
    if (ind < 0)
	ind = 0;
}
