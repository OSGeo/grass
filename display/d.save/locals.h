/* main.c */
struct list_struct {
	char *string;
	struct list_struct *ptr;
};

int main(int, char **);
int in_frame_list(struct Option *, char *);
int init_globals(void);
int which_item(char *);
int set_item(char *, char **);
int process_list(char *, char **, int);
int process_items(char **, int);
int process_pad(char ***, int *);
int list_alloc(int, struct list_struct*, char *);
