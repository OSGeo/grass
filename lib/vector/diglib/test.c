/*
* $Id$
*
****************************************************************************
*
* MODULE:       Vector library 
*   	    	
* AUTHOR(S):    Original author CERL, probably Dave Gerdes.
*               Update to GRASS 5.7 Radim Blazek.
*
* PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdio.h>
#include <grass/Vect.h>

/* Test portable r/w functions */

#define D_TEST 1.3333
#define L_TEST 123456789
#define S_TEST 12345
#define C_TEST 123

int 
main (int argc, char **argv)
{
  int   i, j;
  int   err = 0;
  int   byte_order;
  struct Port_info port;
  GVFILE  fp;
  
  double db, td[] = { -(PORT_DOUBLE_MAX), -(D_TEST), -(PORT_DOUBLE_MIN),
	             0, PORT_DOUBLE_MIN,    D_TEST,   PORT_DOUBLE_MAX }; 
  float fb, tf[]  = { -(PORT_FLOAT_MAX),  -(D_TEST), -(PORT_FLOAT_MIN),
	             0, PORT_FLOAT_MIN,     D_TEST,    PORT_FLOAT_MAX }; 
  long  lb, tl[]  = {   PORT_LONG_MIN,  -(L_TEST), 0, L_TEST, PORT_LONG_MAX  };
  int   ib, ti[]  = {   PORT_INT_MIN,   -(L_TEST), 0, L_TEST, PORT_INT_MAX   };
  short sb, ts[]  = {   PORT_SHORT_MIN, -(S_TEST), 0, S_TEST, PORT_SHORT_MAX };
  char  cb, tc[]  = {   PORT_CHAR_MIN,  -(C_TEST), 0, C_TEST, PORT_CHAR_MAX  };
		    

    if (NULL == (fp.file = fopen ("test.tmp", "wb+")))
      { 
        fprintf (stderr, "ERROR, cannot open test.tmp file.\n");
        return (1);
      }  
    fp.loaded = 0;

    dig_set_cur_port (&port);
      
    byte_order = ENDIAN_LITTLE;
    for ( i = 0; i < 2; i++ )
      {
        dig_init_portable ( &(port), byte_order );
        for (j=0; j < 7; j++)
          {
            dig_fseek (&fp, 0, SEEK_CUR);
	    fprintf (fp.file, "double  ");	  
            dig__fwrite_port_D ( &(td[j]), 1, &fp);
            dig_fseek (&fp, -(PORT_DOUBLE), SEEK_CUR);
	    dig__fread_port_D (&db, 1, &fp);
	    dig_fflush(&fp);
	    if ( db != td[j] )
	      {
                fprintf (stderr, "ERROR in read/write portable double, byte_order = %d\n", byte_order);
                fprintf (stderr, "  Written: %.16e3E\n  Read   : %.16e3E\n", td[j], db);
		err = 1;
	      }
	  }    
        for (j=0; j < 7; j++)
          {
            dig_fseek (&fp, 0, SEEK_CUR);
	    fprintf (fp.file, "float       ");	  
	    dig__fwrite_port_F ( &(tf[j]), 1, &fp);
            dig_fseek (&fp, -(PORT_FLOAT), SEEK_CUR);
	    dig__fread_port_F (&fb, 1, &fp);
	    dig_fflush(&fp);
	    if ( fb != tf[j] )
	      {
                fprintf (stderr, "ERROR in read/write portable float, byte_order = %d\n", byte_order);
                fprintf (stderr, "  Written: %.8e3E\n  Read   : %.8e3E\n", tf[j], fb);
		err = 1;
	      }
          }

        for (j=0; j < 5; j++)
          {
            dig_fseek (&fp, 0, SEEK_CUR);
	    fprintf (fp.file, "long        ");	  
            dig__fwrite_port_L ( &(tl[j]), 1, &fp);
            dig_fseek (&fp, -(PORT_LONG), SEEK_CUR);
	    dig__fread_port_L (&lb, 1, &fp);
	    dig_fflush(&fp);
	    if ( lb != tl[j] )
	      {
                fprintf (stderr, "ERROR in read/write portable long, byte_order = %d\n", byte_order);
                fprintf (stderr, "  Written: %ld\n  Read   : %ld\n", tl[j], lb);
		err = 1;
	      }
          }
	    
        for (j=0; j < 5; j++)
          {
            dig_fseek (&fp, 0, SEEK_CUR);
	    fprintf (fp.file, "int         ");	  
            dig__fwrite_port_I ( &(ti[j]), 1, &fp);
            dig_fseek (&fp, -(PORT_INT), SEEK_CUR);
	    dig__fread_port_I (&ib, 1, &fp);
	    dig_fflush(&fp);
	    if ( ib != ti[j] )
	      {
                fprintf (stderr, "ERROR in read/write portable int, byte_order = %d\n", byte_order);
                fprintf (stderr, "  Written: %d\n  Read   : %d\n", ti[j], ib);
		err = 1;
	      }
          }
	    
        for (j=0; j < 5; j++)
          {
            dig_fseek (&fp, 0, SEEK_CUR);
	    fprintf (fp.file, "short         ");	  
            dig__fwrite_port_S ( &(ts[j]), 1, &fp);
            dig_fseek (&fp, -(PORT_SHORT), SEEK_CUR);
	    dig__fread_port_S (&sb, 1, &fp);
	    dig_fflush(&fp);
	    if ( sb != ts[j] )
	      {
                fprintf (stderr, "ERROR in read/write portable short, byte_order = %d\n", byte_order);
                fprintf (stderr, "  Written: %d\n  Read   : %d\n", ts[j], sb);
		err = 1;
	      }
          }
        for (j=0; j < 5; j++)
          {
            dig_fseek (&fp, 0, SEEK_CUR);
	    fprintf (fp.file, "char           ");	  
            dig__fwrite_port_C ( &(tc[j]), 1, &fp);
            dig_fseek (&fp, -(PORT_CHAR), SEEK_CUR);
	    dig__fread_port_C (&cb, 1, &fp);
	    dig_fflush(&fp);
	    if ( cb != tc[j] )
	      {
                fprintf (stderr, "ERROR in read/write portable char, byte_order = %d\n", byte_order);
                fprintf (stderr, "  Written: %d\n  Read   : %d\n", tc[j], cb);
		err = 1;
	      }

	    
	  }  
        byte_order = ENDIAN_BIG;
      }	  

  fclose ( fp.file );
      
  return ( err );
}


