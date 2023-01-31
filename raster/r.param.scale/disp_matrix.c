/* This file and its content is not used. */
/* Could be removed at some point.       */
#if 0
/****************************************************************/
/* disp_matrix()        Function to display the contents of     */
/*                      the three matrices used in solving      */
/*                      the set of linear equations.            */
/*                      V.1.0, Jo Wood, 9th December, 1994.     */

/****************************************************************/

#include "param.h"

void disp_matrix(double **a, double *x, double *z, int n)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                        /* Displays matrices used to solve a
                           set of linear equations in the form

                           _                        _      _  _      _  _
=======
                        /* Displays matrices used to solve a 
                           set of linear equations in the form 

                           _                        _      _  _      _  _ 
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
                           | a(0,0) a(0,1) ... a(0,n) |    | x0 |    | z0 |
                           | a(1,0) a(1,1) ... a(1,n) |    | x1 |    | z1 |
                           |    :           :   ...   :    | .  | :  | =  | :  |
                           |    :           :   ...   :    |    | :  |    | :  |
                           | a(n,0) a(n,1) ... a(n,n) |    | xn |    | zn |
                           -                        -      -  -      -  -

=======
                        /* Displays matrices used to solve a 
                           set of linear equations in the form 
=======
                        /* Displays matrices used to solve a
                           set of linear equations in the form
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))

                           _                        _      _  _      _  _
                           | a(0,0) a(0,1) ... a(0,n) |    | x0 |    | z0 |
                           | a(1,0) a(1,1) ... a(1,n) |    | x1 |    | z1 |
                           |    :           :   ...   :    | .  | :  | =  | :  |
                           |    :           :   ...   :    |    | :  |    | :  |
                           | a(n,0) a(n,1) ... a(n,n) |    | xn |    | zn |
                           -                        -      -  -      -  -

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
                         */
{

    int row, col;               /* Counts through the matrix */
    char dummy[128];            /* Kewboard input (pause) */

    for (row = 0; row < n; row++) {
        fprintf(stdout, "[ ");

        for (col = 0; col < n; col++)
            fprintf(stdout, "%.3f\t", a[row][col] / 1);

        fprintf(stdout, "]\t[ %.0f\t]\t[ %.0f\t]\n", x[row], z[row]);
    }
    fprintf(stdout, "\n\n");

    fgets(dummy, 70, stdin);
}

void disp_wind(CELL * z)
{                               /* Local window */

    int row, col;               /* Count through local window. */
    char dummy[128];


    for (row = 0; row < wsize; row++) {
        for (col = 0; col < wsize; col++)
            fprintf(stdout, "%d\t", *(z + (row * wsize) + col));

        fprintf(stdout, "\n");
    }

    fgets(dummy, 70, stdin);
}
#endif
<<<<<<< HEAD
<<<<<<< HEAD
extern int dummy_for_iso_compilers; /* suppress -Wempty-translation-unit */
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
