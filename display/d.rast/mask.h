typedef struct _interval
{
    double low, high;
    int inf;
    struct _interval *next;
} Interval;

typedef struct _mask
{
    Interval *list;
} Mask;

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
extern Mask mask;
