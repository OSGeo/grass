#include "simlib.h"
/**
 * \brief Computes dx and dy slope components
 *
 * Computes dx and dy slope components using Horn's (1981) weighted 3x3 method.
 * Derived from r.slope.aspect implementation.
 *
 * \param geometry Grid dimensions and cell spacing
 * \param[in] elevation 2D elevation array (UNDEF = -9999 for null)
 * \param[out] dx East-west partial derivatives
 * \param[out] dy North-south partial derivatives
 */
void derivatives(const Geometry *geometry, float **elevation, double **dx,
                 double **dy)
{
    int row, col;
    double c1, c2, c3, c4, c5, c6, c7, c8, c9;
    double H, V;

    H = (geometry->stepx / geometry->conv) * 8.0;
    V = (geometry->stepy / geometry->conv) * 8.0;

    for (row = 0; row < geometry->my; row++) {
        for (col = 0; col < geometry->mx; col++) {

            c5 = elevation[row][col];

            /* If center is null, derivatives are null */
            if (c5 == UNDEF) {
                dx[row][col] = UNDEF;
                dy[row][col] = UNDEF;
                continue;
            }

            /*
             * Get 3x3 neighborhood
             * At edges or for null values, use center value
             *  ____________________________
             * |c1      |c2      |c3      |
             * |        |        |        |
             * | NW     | north  | NE     |
             * |________|________|________|
             * |c4      |c5      |c6      |
             * |        |        |        |
             * | west   | center | east   |
             * |________|________|________|
             * |c7      |c8      |c9      |
             * |        |        |        |
             * | SW     | south  | SE     |
             * |________|________|________|
             */

            c1 = (row < geometry->my - 1 && col > 0)
                     ? elevation[row + 1][col - 1]
                     : c5;
            if (c1 == UNDEF)
                c1 = c5;

            c2 = (row < geometry->my - 1) ? elevation[row + 1][col] : c5;
            if (c2 == UNDEF)
                c2 = c5;

            c3 = (row < geometry->my - 1 && col < geometry->mx - 1)
                     ? elevation[row + 1][col + 1]
                     : c5;
            if (c3 == UNDEF)
                c3 = c5;

            /* Center row */
            c4 = (col > 0) ? elevation[row][col - 1] : c5;
            if (c4 == UNDEF)
                c4 = c5;

            c6 = (col < geometry->mx - 1) ? elevation[row][col + 1] : c5;
            if (c6 == UNDEF)
                c6 = c5;

            /* Southern row (row-1 because array is stored S to N) */
            c7 = (row > 0 && col > 0) ? elevation[row - 1][col - 1] : c5;
            if (c7 == UNDEF)
                c7 = c5;

            c8 = (row > 0) ? elevation[row - 1][col] : c5;
            if (c8 == UNDEF)
                c8 = c5;

            c9 = (row > 0 && col < geometry->mx - 1)
                     ? elevation[row - 1][col + 1]
                     : c5;
            if (c9 == UNDEF)
                c9 = c5;

            dx[row][col] = ((c1 + c4 + c4 + c7) - (c3 + c6 + c6 + c9)) / H;
            dy[row][col] = ((c7 + c8 + c8 + c9) - (c1 + c2 + c2 + c3)) / V;
        }
    }
}
