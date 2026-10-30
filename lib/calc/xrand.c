#include <stdlib.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
rand(lo,hi) random values between a and b
****************************************************************/

/* The function set by calc_set_random_state(), or NULL. */
static struct G_random_state *(*random_state)(void);

/*!
 * \brief Set the function which gives rand() the random number state
 *
 * The function must return the state of the calling thread, which the
 * tool places for the row being evaluated. Without a function, that is,
 * with NULL, rand() draws from the generator shared by the whole program.
 *
 * \param get_state function returning the calling thread's state, or NULL
 */
void calc_set_random_state(struct G_random_state *(*get_state)(void))
{
    random_state = get_state;
}

/* Draw from the state, or from the shared generator without one. */
static unsigned int draw_uint(struct G_random_state *state)
{
    if (state)
        return G_random_uint32(state);
    return (unsigned int)G_mrand48();
}

/* Draw from the state, or from the shared generator without one. */
static double draw_double(struct G_random_state *state)
{
    if (state)
        return G_random_double(state);
    return G_drand48();
}

/*!
 * \brief Random value between two values, the upper one excluded
 *
 * Draws one value per column from the state given by the function set
 * with calc_set_random_state(), or from the shared generator.
 */
int f_rand(int argc, const int *argt, void **args)
{
    struct G_random_state *state = random_state ? random_state() : NULL;
    int i;

    if (argc < 2)
        return E_ARG_LO;
    if (argc > 2)
        return E_ARG_HI;

    switch (argt[0]) {
    case CELL_TYPE: {
        CELL *res = args[0];
        CELL *arg1 = args[1];
        CELL *arg2 = args[2];

        for (i = 0; i < columns; i++) {
            unsigned int x = draw_uint(state);
            int lo = arg1[i];
            int hi = arg2[i];

            if (lo > hi) {
                int tmp = lo;

                lo = hi;
                hi = tmp;
            }
            res[i] = (lo == hi) ? lo : (int)(lo + x % (unsigned int)(hi - lo));
        }
        return 0;
    }
    case FCELL_TYPE: {
        FCELL *res = args[0];
        FCELL *arg1 = args[1];
        FCELL *arg2 = args[2];

        for (i = 0; i < columns; i++) {
            double x = draw_double(state);
            FCELL lo = arg1[i];
            FCELL hi = arg2[i];

            if (lo > hi) {
                FCELL tmp = lo;

                lo = hi;
                hi = tmp;
            }
            res[i] = (FCELL)(lo + x * (hi - lo));
        }
        return 0;
    }
    case DCELL_TYPE: {
        DCELL *res = args[0];
        DCELL *arg1 = args[1];
        DCELL *arg2 = args[2];

        for (i = 0; i < columns; i++) {
            double x = draw_double(state);
            DCELL lo = arg1[i];
            DCELL hi = arg2[i];

            if (lo > hi) {
                DCELL tmp = lo;

                lo = hi;
                hi = tmp;
            }
            res[i] = lo + x * (hi - lo);
        }
        return 0;
    }
    default:
        return E_INV_TYPE;
    }
}
