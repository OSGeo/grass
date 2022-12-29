#ifndef HUGE_VAL
#define HUGE_VAL 10000000.0
#endif
#include "vizual.h"

extern GLuint Material_1_Dlist;

void get_default_table();
void get_min_max();

/******************************************************************************
int get_color_table(file, ctable):
reads color table from file & stores values in ctable.

color table files must have the form:

value1:r1:g1:b1
value2:r2:g2:b2
.
.
.
valueN:rN:gN:bN

where values 1-n are floats listed in increasing order,
r, g, & b are integer 0 <= r, g, b <= 255,
number of lines in table N <= 100.
******************************************************************************/
int get_color_table(file, ctable)
     char *file;
     struct color_entry ctable[];
{
    FILE *fp;
    int tmp = 0;
    int i;

    fp = fopen(file, "r");
    if (!fp) {
	fprintf(stderr, "Unable to open color file for reading\n");
	return (-1);
    }

    for (i = 0; i < 100 && EOF != tmp; i++) {
	tmp = fscanf(fp, "%f:%hd:%hd:%hd",
		     &(ctable[i].data),
		     &(ctable[i].color[0]),
		     &(ctable[i].color[1]), &(ctable[i].color[2]));
	if (tmp != EOF && tmp != 4) {
	    fclose(fp);
	    fprintf(stderr, "Unable to read colortable file <%s>\n", file);
	    return (-1);
	}

    }
    if (i == 100 && tmp != EOF) {
	fprintf(stderr, "Number of colortable entries may not exceed 100.\n");
	fprintf(stderr, "First 100 entries will be used.\n");
    }
    else
	i--;

    ctable[i].data = 0.0;
    ctable[i].color[0] = -1;
    ctable[i].color[1] = -1;
    ctable[i].color[2] = -1;

    fclose(fp);
    if (!i) {
	fprintf(stderr,
		"Colortable file is empty, using default colortable\n");
	get_default_table(&Headfax, ctable);
    }
    return (0);
}

/******************************************************************************
get_cat_color (cat, ctable, color):
fills color array with color value for cat calculated from values in ctable.
******************************************************************************/
void get_cat_color(cat, ctable, color)
     float cat;
     struct color_entry ctable[];
     short color[3];
{
    int curr;
    float cat1, cat2;
    short *color1, *color2;
    float delta;

    /*DEBUG
       fprintf (stderr, "TABLE\n");
       for (i = 0; ctable[i].color[0] >= 0; i++)
       fprintf (stderr, "%f:%hd:%hd:%hd\n", 
       (ctable[i].data), 
       (ctable[i].color[0]),
       (ctable[i].color[1]),
       (ctable[i].color[2]));
     */
    color[0] = color[1] = color[2] = 0;
    color1 = ctable[0].color;
    cat1 = ctable[0].data;

    if (ctable[0].color[0] < 0) {
	return;
    }
    /* all cat < the first color table entry use the first color */
    else if ((cat < cat1) || (ctable[1].color[0] < 0)) {
	color[0] = color1[0];
	color[1] = color1[1];
	color[2] = color1[2];
	return;
    }
    /* find the categories in the color table above & below 
       this cat & interpolate the color values *** */

    for (curr = 1; ctable[curr].color[0] >= 0; curr++) {
	cat2 = ctable[curr].data;
	color2 = ctable[curr].color;
	if (cat2 > cat) {
	    delta = (cat - cat1) / (cat2 - cat1);
	    color[0] = delta * color2[0] + (1 - delta) * color1[0];
	    color[1] = delta * color2[1] + (1 - delta) * color1[1];
	    color[2] = delta * color2[2] + (1 - delta) * color1[2];
	    return;
	}
	cat1 = cat2;
	color1 = color2;

    }
    /* for any thresholds greater than last color table entry, use last entry * */
    color[0] = color2[0];
    color[1] = color2[1];
    color[2] = color2[2];
}

/******************************************************************************
new_color_file (file, cfile, D_spec)
gets new color values for D_spec->ctable from file cfile.
If unable to open file (grid3 file) or cfile (colortable file),
prints error message & uses original (per threshold) color table 
(i.e. gets out of per cube color mode).
******************************************************************************/

int new_color_file(file, cfile, D_spec)
     char *file, *cfile;
     struct dspec *D_spec;
{
    if (D_spec->cfile != NULL)
	fclose(D_spec->cfile);
    D_spec->cfile = fopen(file, "r");
    if (D_spec->cfile == NULL) {
	fprintf(stderr, "Unable to open <%s>\n", file);
	return (-1);
    }
    else if (0 > get_color_table(cfile, D_spec->ctable)) {
	fprintf(stderr, "Unable to read color table\n");
	fclose(D_spec->cfile);
	D_spec->cfile = NULL;
	return (-1);
    }
    return (0);

}

/******************************************************************************
no_color_file (D_spec, cfile)
Gets new color values for D_spec->ctable from file cfile,
which is the original (per threshold) color table 
(i.e. gets out of per cube color mode).
If unable to open cfile (colortable file), prints error message 
and uses default color table.
******************************************************************************/
void no_color_file(D_spec, cfile)
     struct dspec *D_spec;
     char *cfile;
{
    if (D_spec->cfile != NULL)
	fclose(D_spec->cfile);
    D_spec->cfile = NULL;
    if (0 > get_color_table(cfile, D_spec->ctable)) {
	fprintf(stderr, "Using default color table\n");
	get_default_table(&Headfax, D_spec->ctable);
    }
}

/******************************************************************************
get_default_table (head, ctable):
Calculates values for ctable to produce a rainbow color table from
range of  threshold values  stored in head.
******************************************************************************/
void get_default_table(head, ctable)
     file_info *head;
     struct color_entry ctable[];
{
    float min, max;

    get_min_max(head, &min, &max);

    ctable[0].data = min;
    ctable[0].color[0] = 255;
    ctable[0].color[1] = 255;
    ctable[0].color[2] = 255;

    ctable[1].data = (min + max) / 3.0;
    ctable[1].color[0] = 255;
    ctable[1].color[1] = 0;
    ctable[1].color[2] = 0;

    ctable[2].data = (min + max) * 2.0 / 3.0;
    ctable[2].color[0] = 0;
    ctable[2].color[1] = 0;
    ctable[2].color[2] = 255;

    ctable[3].data = max;
    ctable[3].color[0] = 0;
    ctable[3].color[1] = 255;
    ctable[3].color[2] = 0;

    ctable[4].data = 0;
    ctable[4].color[0] = -1;
    ctable[4].color[1] = -1;
    ctable[4].color[2] = -1;
}

/******************************************************************************
get_min_max (head, min, max)
assigns min & max the minimum & maximum threshold values stored in head.
******************************************************************************/
void get_min_max(head, min, max)
     file_info *head;
     float *min, *max;
{

    int i;

    *max = -1 * HUGE_VAL;
    *min = HUGE_VAL;

    for (i = 0; i < head->linefax.nthres; i++) {
	if (head->linefax.tvalue[i] < *min)
	    *min = head->linefax.tvalue[i];
	if (head->linefax.tvalue[i] > *max)
	    *max = head->linefax.tvalue[i];
    }
}

void change_spec(spec)
     float spec;
{
    static float material[] = {
	GL_SPECULAR, 1.0, 1.0, 1.0,
	GL_DIFFUSE, 0.8, 0.8, 0.8,
	GL_AMBIENT, 0.8, 0.8, 0.8,
	GL_SHININESS, 10,
	0
    };
    material[1] = material[2] = material[3] = spec;
    glNewList(Material_1_Dlist, GL_COMPILE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &material[9]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &material[5]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &material[1]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &material[13]);
    glEndList();

    glCallList(Material_1_Dlist);
    glEnable(GL_LIGHTING);
}
