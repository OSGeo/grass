#include <stdlib.h>
#include <string.h>
#include "global.h"

int get_psuedo_control_pt (int tie_row, int tie_col) 
{
    char msg[200];
    struct Ortho_Photo_Points ps_cp; 
    int i,j,k;

#ifdef DEBUG3
    fprintf (Bugsr,"In ps_cp \n");
    fflush  (Bugsr);
#endif

    /*  allocate psuedo struct, max points are four */
    ps_cp.count = 4;
    ps_cp.e1 = (double *) G_malloc (4*sizeof(double)); 
    ps_cp.n1 = (double *) G_malloc (4*sizeof(double));
    ps_cp.e2 = (double *) G_malloc (4*sizeof(double));
    ps_cp.n2 = (double *) G_malloc (4*sizeof(double));
    ps_cp.status = (int *) G_malloc (4*sizeof(int));

#ifdef DEBUG3
    fprintf (Bugsr,"ps_cp allocated \n");
    fflush  (Bugsr);
#endif


    /*  pseudo points are four corners taken from T_Points */
    k = 0;
    for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 2; j++)
      {
         ps_cp.e1[k] = T_Point[tie_row+i][tie_col+j].xt;
         ps_cp.n1[k] = T_Point[tie_row+i][tie_col+j].yt;
         ps_cp.e2[k] = (j*((T_Point[tie_row][tie_col+1].XT - 
                         T_Point[tie_row][tie_col].XT)/target_window.ew_res));
         ps_cp.n2[k] = (i*((T_Point[tie_row][tie_col].YT - 
                       T_Point[tie_row+1][tie_col].YT)/target_window.ns_res));
         ps_cp.status[k] = 1;

#ifdef DEBUG3
         fprintf (Bugsr,"\t k = %d\t i = %d\t j = %d \n",k,i,j);
         fprintf (Bugsr,"\t\t e1[k] = %f \n",ps_cp.e1[k]);
         fprintf (Bugsr,"\t\t n1[k] = %f \n",ps_cp.n1[k]);
         fprintf (Bugsr,"\t\t e2[k] = %f \n",ps_cp.e2[k]);
         fprintf (Bugsr,"\t\t n2[k] = %f \n",ps_cp.n2[k]);
         fflush  (Bugsr);
#endif

         k++;
      }
    }

#ifdef DEBUG3
    fprintf (Bugsr,"ps_cp initialized \n");
    fflush  (Bugsr);
#endif

    
    switch (I_compute_ref_equations (&ps_cp,E12,N12,E21,N21)) 
    {
    case -1:
#ifdef DEBUG3
        fprintf (Bugsr,"\tref_equ: case -1\n");
        fflush  (Bugsr);
#endif
	strcat  (msg,"Poorly placed psuedo control points.\n");
	strcat  (msg,"Can not generate the transformation equation.\n");
	break;
    case 0:
#ifdef DEBUG3
        fprintf (Bugsr,"\tref_equ: case 0 \n");
        fflush  (Bugsr);
#endif
	strcat  (msg, "No active psuedo control points\n");
	break;
    default:
#ifdef DEBUG3
        fprintf (Bugsr,"\tref equ: case good\n");
        fflush  (Bugsr);
#endif
	E12a = E12[0]; E12b = E12[1]; E12c = E12[2];
	N12a = N12[0]; N12b = N12[1]; N12c = N12[2];
	E21a = E21[0]; E21b = E21[1]; E21c = E21[2];
	N21a = N21[0]; N21b = N21[1]; N21c = N21[2];
#ifdef DEBUG3
        fprintf (Bugsr,"\t\tE21 = %f\t %f\t %f \n",E21a,E21b,E21c);
        fprintf (Bugsr,"\t\tN21 = %f\t %f\t %f \n",N21a,N21b,N21c);
        fflush  (Bugsr);
#endif
	return 1;
    }
    G_fatal_error (msg);
    exit(1);
}


