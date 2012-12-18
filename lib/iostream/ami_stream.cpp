/****************************************************************************
 * 
 *  MODULE:     iostream
 *

 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 * 

 *  Iostream is a library that implements streams, external memory
 *  sorting on streams, and an external memory priority queue on
 *  streams. These are the fundamental components used in external
 *  memory algorithms.  

 * Credits: The library was developed by Laura Toma.  The kernel of
 * class STREAM is based on the similar class existent in the GPL TPIE
 * project developed at Duke University. The sorting and priority
 * queue have been developed by Laura Toma based on communications
 * with Rajiv Wickremesinghe. The library was developed as part of
 * porting Terraflow to GRASS in 2001.  PEARL upgrades in 2003 by
 * Rajiv Wickremesinghe as part of the Terracost project.

 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.  *
 *  **************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

//#include <ami_stream.h>
#include <grass/iostream/ami_stream.h>


const char *ami_str_error[] = {
  "AMI_ERROR_NO_ERROR",
  "AMI_ERROR_IO_ERROR",
  "AMI_ERROR_END_OF_STREAM",
  "AMI_ERROR_OUT_OF_RANGE",
  "AMI_ERROR_READ_ONLY",
  "AMI_ERROR_OS_ERROR",
  "AMI_ERROR_MM_ERROR",
  "AMI_ERROR_OBJECT_INITIALIZATION",
  "AMI_ERROR_PERMISSION_DENIED",
  "AMI_ERROR_INSUFFICIENT_MAIN_MEMORY",
  "AMI_ERROR_INSUFFICIENT_AVAILABLE_STREAMS",
  "AMI_ERROR_ENV_UNDEFINED",
  "AMI_ERROR_NO_MAIN_MEMORY_OPERATION",
};

/**********************************************************************/
/* creates a random file name, opens the file for reading and writing
   and and returns a file descriptor */
int
ami_single_temp_name(const std::string& base, char* tmp_path) {
 
  char *base_dir;
  int fd;

  // get the dir
  base_dir = getenv(STREAM_TMPDIR);
  if(!base_dir) {
	fprintf(stderr, "ami_stream: %s not set\n", STREAM_TMPDIR);
	assert(base_dir);
	exit(1);
  }
  sprintf(tmp_path, "%s/%s_XXXXXX", base_dir, base.c_str());

#ifdef __MINGW32__
  fd = mktemp(tmp_path) ? open(tmp_path, O_CREAT|O_EXCL|O_RDWR, 0600) : -1;
#else
  fd = mkstemp(tmp_path);
#endif

  if (fd == -1) {
    cerr <<  "ami_single_temp_name: ";
#ifdef __MINGW32__
    perror("mktemp failed: ");
#else
    perror("mkstemp failed: ");
#endif
    assert(0);
    exit(1);
  }
  return fd;
}


/**********************************************************************/
/* given fd=fide descriptor, associates with it a stream aopened in
   access_mode and returns it */
FILE* 
open_stream(int fd, AMI_stream_type st) {
  FILE* fp = NULL;
  
  assert(fd > -1);   
  switch (st) {
  case   AMI_READ_STREAM:
    fp = fdopen(fd, "rb");
    break;
  case   AMI_WRITE_STREAM:
    fp = fdopen(fd, "wb");
    break;
  case AMI_APPEND_WRITE_STREAM:
    fp = fdopen(fd, "ab");
    break;
  case AMI_APPEND_STREAM:
    fp = fdopen(fd, "ab+");
    break;
  case AMI_READ_WRITE_STREAM: 
	fp = fdopen(fd, "rb+");
	if (!fp) {
	  //if file does not exist, create it
	  fp = fdopen(fd, "wb+");
	}
	break;
  }
  if(!fp) {
    perror("fdopen");
  }
  assert(fp);

  return fp;
}


/**********************************************************************/
/* open the file whose name is pathname in access mode */
FILE* 
open_stream(char* pathname, AMI_stream_type st) {

  FILE* fp = NULL;
  assert(pathname);

  switch (st) {
  case   AMI_READ_STREAM:
    fp = fopen(pathname, "rb");
    break;
  case   AMI_WRITE_STREAM:
    fp = fopen(pathname, "wb");
    break;
  case AMI_APPEND_WRITE_STREAM:
    fp = fopen(pathname, "ab");
    break;
  case AMI_APPEND_STREAM:
    fp = fopen(pathname, "ab+");
    assert(fp);
    G_fseek (fp, 0, SEEK_END);
    break;
  case AMI_READ_WRITE_STREAM: 
      fp = fopen(pathname, "rb+");
      if (!fp) {
	//if file does not exist, create it
      fp = fopen(pathname, "wb+");
      }
      break;
  }
  if (!fp) {
    perror(pathname);
    assert(0);
    exit(1);
  }
  assert(fp);
  return fp;
}

