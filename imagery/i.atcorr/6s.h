#ifndef _6S_H
#define _6S_H

/* The 6s computation has been broken up into 4 separate parts.

    1. Parse the input conditions and do setup of global objects by using these values

    loop over every value in input raster
        (optional step, used if elevation map is supplied)
        2. Use new input conditions (currently only the height can be varied, but more could be added)
           to re-initialized only those objects that are affected for the main computation.

        (called every time either step 1 or step 2 has been run)
        3. Compute parameters for the transformation stage.

        4. Do transformation of input value.
*/


/* initialize global variables from the input conditions file 

return:
 < 0  : error
 >= 0 : all is fine
 */
extern int init_6S(char* icnd_name);

/* Initialize computations with a different height and/or visibility.
  This requires lots of computations and therefore can be very
  time consuming.
*/
extern void pre_compute_hv(const double height, const double vis);
extern void pre_compute_v(const double vis);
extern void pre_compute_h(const double height);

struct TransformInput;
/* Compute the input parameters used to do atmospheric correction on input values
   None of the global objects are change in this stage.
*/
extern TransformInput compute();

#endif /* _6S_H */
