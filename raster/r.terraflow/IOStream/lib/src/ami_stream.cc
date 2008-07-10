/****************************************************************************
 * 
 *  MODULE:	r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <ami_stream.h>

/**********************************************************************/
/* creates a random file name, opens the file for reading and writing
   and and returns a file descriptor */
int
ami_single_temp_name(const std::string& base, char* tmp_path) {
 
  char *base_dir;
  int fd;

  // get the dir
  base_dir = getenv(STREAM_TMPDIR);
  assert(base_dir);

  sprintf(tmp_path, "%s/%s_XXXXXX", base_dir, base.c_str());
#ifdef __MINGW32__
  fd = mktemp(tmp_path) ? open(tmp_path, O_CREAT|O_EXCL|O_RDWR, 0600) : -1;
#else
  fd  = mkstemp(tmp_path);
#endif

  if (fd == -1) {
    cerr <<  "ami_single_temp_name: ";
    perror("mkstemp failed: ");
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
  case AMI_APPEND_STREAM:
    fp = fopen(pathname, "ab+");
    assert(fp);
    if (fseek (fp, 0, SEEK_END) == -1) {
      perror("AMI_STREAM: fseek failed ");
    }
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
    perror("cannot open stream");
    assert(0);
    exit(1);
  }
  assert(fp);
  return fp;
}

