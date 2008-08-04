#ifndef CELL_PTRHA_H
#define CELL_PTRHA_H
struct cell_ptrHa
{
    float angle;
    int row, col;
    struct cell_ptrHa *next;
};
#endif
