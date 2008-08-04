/* Standard papers */

typedef struct
{
    char *name;
    double page_width, page_height;
    double left_marg, right_marg, top_marg, bot_marg;
} PAPER;

PAPER papers[] = {
    {"a4", 8.268, 11.693, 0.5, 0.5, 1.0, 1.0},
    {"a3", 11.693, 16.535, 0.5, 0.5, 1.0, 1.0},
    {"a2", 16.54, 23.39, 1.0, 1.0, 1.0, 1.0},
    {"a1", 23.39, 33.07, 1.0, 1.0, 1.0, 1.0},
    {"a0", 33.07, 46.77, 1.0, 1.0, 1.0, 1.0},
    {"us-legal", 8.5, 14.0, 1.0, 1.0, 1.0, 1.0},
    {"us-letter", 8.5, 11.0, 1.0, 1.0, 1.0, 1.0},
    {"us-tabloid", 11.0, 17.0, 1.0, 1.0, 1.0, 1.0},
    {NULL, 0, 0, 0, 0, 0, 0}
};
