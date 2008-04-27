#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <grass/glocale.h>
#include "global.h"


int write_matrix (int row, int col)
{
    int n;

    select_target_env();

    /* create temp file if it doesn't eexist */
    if (!temp_fd || (fcntl (temp_fd, F_GETFD) == -1))
    {
        temp_name = G_tempfile();
        temp_fd = creat(temp_name,0660);
    }

    for (n=0; n < matrix_rows; n++)
    {
        off_t offset;

        offset = ((off_t) row++ * target_window.cols + col) * G_raster_size(map_type);
        lseek(temp_fd,offset,SEEK_SET);

        if(write(temp_fd,cell_buf[n],G_raster_size(map_type)*matrix_cols) 
                                != G_raster_size(map_type)*matrix_cols)
        {
           unlink(temp_name);
           G_fatal_error (_("Unable to write temp file: %s"), strerror(errno));
        }
    }

    select_current_env();

    return 0;
}


int write_map(char *name)
{
   int fd, row;
   void *rast;

   G_set_window(&target_window); 

   rast = G_allocate_raster_buf(map_type);
   close(temp_fd);
   temp_fd = open (temp_name, F_DUPFD);
   fd = G_open_raster_new(name,map_type);

   if(fd <=0)
       G_fatal_error (_("Unable to open map %s"), name);

   for(row = 0; row < target_window.rows; row++)
   {
       if(read(temp_fd,rast,target_window.cols * G_raster_size(map_type))
                    != target_window.cols * G_raster_size(map_type)) 
          G_fatal_error (_("Unable to write row %d"), row);

       if(G_put_raster_row(fd,rast, map_type) < 0)
       {
          G_fatal_error (_("Unable to write raster map. You might want to check available disk space and write permissions."));
          unlink(temp_name);
       }
   }

   close(temp_fd);
   unlink(temp_name);
   G_close_cell(fd);

   return 0;
}
