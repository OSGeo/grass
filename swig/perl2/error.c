#include <stdlib.h>
#include <stdio.h>

void my_error_routine(char *msg, int fatal) 
{   
    fprintf(stderr,"Grass error: %s\n", msg);
}

void set_my_error_routine()
{
    G_set_error_routine (my_error_routine);
}
