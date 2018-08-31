typedef struct _d_interval
{
    double low, high;
    int inf;
    struct _d_interval *next;
} d_Interval;

typedef struct _d_mask
{
    d_Interval *list;
} d_Mask;

extern d_Mask d_mask;
extern DCELL new_null;
