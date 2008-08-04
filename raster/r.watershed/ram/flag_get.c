#include "flag.h"

int flag_get(FLAG * flags, int row, int col)
{
    return (flags->array[row][col >> 3] & (1 << (col & 7)));
}
