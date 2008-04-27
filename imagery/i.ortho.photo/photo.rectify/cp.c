#include <stdlib.h>
#include <string.h>
#include "global.h"

int get_conz_points (void)
{
    char msg[200];
 
    if(!I_get_con_points (group.name, &group.control_points))
	exit(0);

    /* compute photo coordinates of image control points  */
    /* I_convert_con_points (group.name, &group.control_points, &group.photo_points,  group.E12, group.N12); */


    sprintf (msg, "Control Z Point file for group [%s] in [%s] \n \n",
	group.name,G_mapset());

    fprintf (stderr,"Computing equations...\n\n");

    Compute_ortho_equation();

    switch (group.con_equation_stat)
    {
    case -1:
	strcat (msg, "Poorly placed Control Points!\n");
	strcat (msg, "Can not generate the transformation equation.\n");
	strcat (msg, "Run OPTION 7 again!\n");
	break;
    case 0:
	strcat (msg, "No active Control Points!\n");
	strcat (msg, "Can not generate the transformation equation.\n");
	strcat (msg, "Run OPTION 7 !\n");
	break;
    default:
	return 1;
    }
    G_fatal_error (msg);
}

int get_ref_points (void)
{
    char msg[200];
    /* struct Ref_Points cp; */

    if(!I_get_ref_points (group.name, &group.photo_points))
	exit(0);

    sprintf (msg, "Reference Point file for group [%s] in [%s] \n \n",
      group.name, G_mapset());

    Compute_ref_equation();
    switch (group.ref_equation_stat)
    {
    case -1: 
	strcat (msg, "Poorly placed Reference Points!\n");
	strcat (msg, "Can not generate the transformation equation.\n");
	strcat (msg, "Run OPTION 5 again!\n");     
	break;
        
    case 0:
        strcat (msg, "No active Reference Points!\n");
        strcat (msg, "Can not generate the transformation equation.\n");
        strcat (msg, "Run OPTION 5!\n");
        break;
    default:
	E12a = E12[0]; E12b = E12[1]; E12c = E12[2];
	N12a = N12[0]; N12b = N12[1]; N12c = N12[2];
	E21a = E21[0]; E21b = E21[1]; E21c = E21[2];
	N21a = N21[0]; N21b = N21[1]; N21c = N21[2];
	return 1;
    }
    G_fatal_error (msg);
    /* exit(1);   shouldn't get here */
}


