/* row/col list */

struct rc {
    struct rc *next;
    int row;
    int col;
};

struct rclist {
    struct rc *tail, *head;
};

/* rclist.c */
void rclist_init(struct rclist *);
void rclist_add(struct rclist *, int, int);
int rclist_drop(struct rclist *, struct rc *);
void rclist_destroy(struct rclist *);
