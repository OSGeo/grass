#include "vizual.h"
extern GLuint Material_1_Dlist;
void winset_main();
void winset_colortable();
void do_lights()
{

    GLfloat light_position[] = { -150.0, 100.0, 200.0, 0.0 };
    GLfloat light_position2[] = { 150.0, 40.0, -200.0, 0.0 };
    GLfloat light_diffuse[] = { 0.58, 0.58, 0.58, 1. };
    GLfloat light_diffuse2[] = { 0.40, 0.40, 0.40, 1. };
    GLfloat light_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat light_ambient2[] = { 0.3, 0.3, 0.3, 1.0 };
    /* GLfloat lmodel_ambient[] = { 2.,2.,2., 1.0 }; */


    static float material[] = {
	GL_SPECULAR, 1.0, 1.0, 1.0,
	GL_DIFFUSE, 0.8, 0.8, 0.8,
	GL_AMBIENT, 0.8, 0.8, 0.8,
	GL_SHININESS, 10,
    };
    /*
       glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
     */

    Material_1_Dlist = glGenLists(1);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT1, GL_POSITION, light_position2);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse2);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient2);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    glNewList(Material_1_Dlist, GL_COMPILE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &material[9]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &material[5]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &material[1]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &material[13]);
    glEndList();

    glCallList(Material_1_Dlist);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}


/************************************  make_matcolors  ************************/
/* this code came from GRASS color code written by M.Shapiro USACERL */
void draw_colortable(D_spec, Headfax, Window)
     struct dspec *D_spec;
     file_info *Headfax;
     long Window[];
{
    short t;
    short x1, x2, y1, y2;
    short vert[4][2];		/* the vertices of the colorsquares */
    int yadd;
    short color[3];


    x1 = 10;
    x2 = 90;
    y1 = y2 = 0;

    yadd = (int)1000 / Headfax->linefax.nthres;

    /* draw the colortable */
    for (t = 0; t < Headfax->linefax.nthres; t++) {
	y1 = y2;
	y2 += yadd;

	vert[0][0] = x1;
	vert[0][1] = y1;
	vert[1][0] = x2;
	vert[1][1] = y1;
	vert[2][0] = x2;
	vert[2][1] = y2;
	vert[3][0] = x1;
	vert[3][1] = y2;

	get_cat_color(Headfax->linefax.tvalue[t], D_spec->ctable, color);

	glColor3ub(color[0], color[1], color[2]);


	glBegin(GL_POLYGON);
	glVertex2sv(vert[0]);
	glVertex2sv(vert[1]);
	glVertex2sv(vert[2]);
	glVertex2sv(vert[3]);
	glEnd();
    }
}
