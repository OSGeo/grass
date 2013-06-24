#ifndef _LOCAL_PROTO_H
#define _LOCAL_PROTO_H

#include <string.h>
#include "landsat.h"

void lsat_metadata(char *, lsat_data *);

void set_MSS1(lsat_data *);
void set_MSS2(lsat_data *);
void set_MSS3(lsat_data *);
void set_MSS4(lsat_data *);
void set_MSS5(lsat_data *);

void set_TM4(lsat_data *);
void set_TM5(lsat_data *);

void set_ETM(lsat_data *, char[]);

void set_LDCM(lsat_data *);

#endif
