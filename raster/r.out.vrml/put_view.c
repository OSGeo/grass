#include "pv.h"

/* Not yet implemented - just defaults */

void vrml_put_view(FILE *vout, struct G_3dview *v3d UNUSED)
{
    char tbuf[512];

#ifdef VRML2

    vrml_putline(vout, "view\n");

#else
    /*
       vrml_putline(1, vout, "DirectionalLight {");
       snprintf(tbuf, sizeof(tbuf),"intensity %f",1.0);
       vrml_putline(0, vout, tbuf);
       snprintf(tbuf, sizeof(tbuf),"color %f %f %f",1.0, 1.0, 1.0);
       vrml_putline(0, vout, tbuf);
       snprintf(tbuf, sizeof(tbuf),"direction %f %f %f",0.0, 0.0, -1.0);
       vrml_putline(0, vout, tbuf);
       vrml_putline(-1, vout,"}");
     */
    vrml_putline(1, vout, "PerspectiveCamera {");
    snprintf(tbuf, sizeof(tbuf), "position %f %f %f", 0.0, 0.0, 1.0);
    vrml_putline(0, vout, tbuf);
    /*
       snprintf(tbuf, sizeof(tbuf),"orientation %f %f %f  %f", 1.0, 0.0,
       0.0, 4.3175);
     */
    snprintf(tbuf, sizeof(tbuf), "orientation %f %f %f  %f", 0.0, 0.0, 1.0,
             0.0);
    vrml_putline(0, vout, tbuf);
    snprintf(tbuf, sizeof(tbuf), "focalDistance %f", 3.0);
    vrml_putline(0, vout, tbuf);
    snprintf(tbuf, sizeof(tbuf), "heightAngle %f", 0.785398); /* 45 degrees */
    vrml_putline(0, vout, tbuf);
    vrml_putline(-1, vout, "}");

#endif
}
