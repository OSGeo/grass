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


#ifndef STATS_H
#define STATS_H

#include <sys/types.h>

#include <fstream>
#include <iostream>

#include <grass/iostream/ami.h>




int noclobberFile(char *);

class statsRecorder : public ofstream {
private:
  Rtimer tm;
  void *bss;
public:
  statsRecorder(char *fname);
  ~statsRecorder() { 
	this->flush(); 
  }
  char *freeMem(char *);
  long freeMem();
  char *timestamp();
  void timestamp(const char *s);
  void comment(const char *s, const int verbose=1);
  void comment(const char *s1, const char *s2);
  void comment(const int n);
  void recordTime(const char *label, long secs);
  void recordTime(const char *label, Rtimer rt);
  void recordLength(const char *label, off_t len, int siz=0, char *sname=NULL);
  template<class T> void recordLength(const char *label, AMI_STREAM<T> *str) {
	recordLength(label, str->stream_len(), sizeof(T), str->sprint());
  }
};

char *formatNumber(char *buf, off_t val);

#endif
