#include "flag.h"

int flag_unset(FLAG * flags, int row, int col)
{
    flags->array[row][col >> 3] &= ~(1 << (col & 7));

    return 0;
}
