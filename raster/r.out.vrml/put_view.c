
#include "pv.h"

/* Not yet implemented - just defaults */

void vrml_put_view(FILE * vout, struct G_3dview *v3d)
{
    char tbuf[512];


#ifdef VRML2

    vrml_putline(vout, "view\n");

#else
    /*
       vrml_putline(1, vout, "DirectionalLight {");
       sprintf(tbuf,"intensity %f",1.0);
       vrml_putline(0, vout, tbuf);
       sprintf(tbuf,"color %f %f %f",1.0, 1.0, 1.0);
       vrml_putline(0, vout, tbuf);
       sprintf(tbuf,"direction %f %f %f",0.0, 0.0, -1.0);
       vrml_putline(0, vout, tbuf);
       vrml_putline(-1, vout,"}");
     */
    vrml_putline(1, vout, "PerspectiveCamera {");
    sprintf(tbuf, "position %f %f %f", 0.0, 0.0, 1.0);
    vrml_putline(0, vout, tbuf);
    /*
       sprintf(tbuf,"orientation %f %f %f  %f", 1.0, 0.0, 0.0, 4.3175);
     */
    sprintf(tbuf, "orientation %f %f %f  %f", 0.0, 0.0, 1.0, 0.0);
    vrml_putline(0, vout, tbuf);
    sprintf(tbuf, "focalDistance %f", 3.0);
    vrml_putline(0, vout, tbuf);
    sprintf(tbuf, "heightAngle %f", 0.785398);	/* 45 degrees */
    vrml_putline(0, vout, tbuf);
    vrml_putline(-1, vout, "}");

#endif

}
