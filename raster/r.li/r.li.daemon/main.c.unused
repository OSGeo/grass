/****************************************************************************
 *
 * MODULE:       r.li.daemon
 * AUTHOR(S):    Claudio Porta, Lucio D. Spano, Serena Pallecchi (original contributors)
 *                students of Computer Science University of Pisa (Italy)
 *               Commission from Faunalia Pontedera (PI) www.faunalia.it
 *               Fixes: Markus Neteler <neteler itc.it>
 *               
 * PURPOSE:      r.li.daemon with a simple index for library debug
 * COPYRIGHT:    (C) 2006-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "daemon.h"

/**
 * main with a simple index for library debug
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 */
int main(int argc, char *argv[]){
	struct Option *raster, *conf, *output;
	struct GModule *module;
	
	G_gisinit(argv[0]);
	module = G_define_module();
	module->description =_("Calculates <simple> index on a raster map");
	module->keywords = _("raster, landscape structure analysis, job launcher");

	/* define options */
	
	raster = G_define_standard_option(G_OPT_R_MAP);
	
	conf = G_define_option();
	conf->key = "conf";
	conf->description = "Areas configuration file";
	conf->gisprompt = "old_file,file,input";
	conf->type = TYPE_STRING;
	conf->required = YES;
	
	output = G_define_standard_option(G_OPT_R_OUTPUT);
	
	/** add other options for index parameters here */
	
	if (G_parser(argc, argv))
	   exit(EXIT_FAILURE);

	return calculateIndex(conf->answer, simple_index, NULL, raster->answer, output->answer);
	
}

int simple_index(int fd, char ** par, area_des ad, double *result){
	CELL *buf, *sup;
	int count, i,j, connected=0, complete_line=1;
	double area;
	char *mapset, c[150];
	struct Cell_head hd;
	CELL complete_value;
	
	G_set_c_null_value(&complete_value, 1);
	mapset = G_find_cell(ad->raster, "");
	if (G_get_cellhd(ad->raster, mapset, &hd) == - 1)
		return 0;
	sup = G_allocate_cell_buf();
	if(ad->mask == -1){
		double meters_row, meters_col;
		/* no mask */
		
		/*calculate area size */
		meters_row = G_distance(hd.north,hd.west, hd.south, hd.west)\
		/hd.rows;
		meters_col = G_distance(hd.north, hd.west, hd.north, hd.east)\
		/hd.cols;
		area = meters_row * ad->rl * meters_col * ad->cl;
		
		/*calculate number of patch*/
		count =0;
		for(i = 0; i<ad->rl; i++){
			connected = 0;
			buf = RLI_get_cell_raster_row(fd, i+ad->y, ad);
			if(i > 0){
				sup = RLI_get_cell_raster_row(fd, i-1+ad->y, ad);
			}
			
			if (complete_line){
				if (! G_is_null_value( &(buf[ad->x]), CELL_TYPE) &&\
					buf[ad->x] != complete_value)
					count++;
				for(j=0; j<ad->cl; j++){
					if (buf[j+ad->x] != buf[j+1+ad->x]){
						complete_line=0;
						if(!G_is_null_value( &(buf[j+ad->x]), CELL_TYPE) &&\
							buf[j+ad->x] != complete_value)
							count++;
						}
				}
				if (complete_line){
					complete_value = buf[ad->x];
				}
			}
			else{
				complete_line = 1;
				for(j=0; j<ad->cl; j++){
					if (sup[j+ad->x] == buf[j+ad->x]){
						connected = 1;
					}
					if (buf[j+ad->x] != buf[j+1+ad->x]){
						complete_line =0;
						if (!connected && \
							!G_is_null_value( &(buf[j+ad->x]), CELL_TYPE)){
							count++;
							connected =0;									
						}
						else{
							connected=0;
						}
					}
				}
				if (complete_line)
					complete_value = buf[ad->x];
			}
		}

		#ifdef DEBUG
		printf("number of patch = %i\n", count);
		#endif
		#ifdef DEBUG
		printf("area = %f\n", area);
		#endif
		*result= (count/area) * 10000;
		return 1;
	}
	else{
		double meters_row, meters_col;
		int cell_dim = 0, mask_fd, *mask_buf;
		CELL null_value;
		
		G_set_c_null_value(&null_value, 1);
		/* mask */
		if ((mask_fd = open(ad->mask_name, O_RDONLY, 0755)) < 0)
			return 0;
		mask_buf = malloc(ad->cl * sizeof(int));
		/*calculate area size */
		meters_row = G_distance(hd.north,hd.west, hd.south, hd.west)\
		/hd.rows;
		meters_col = G_distance(hd.north, hd.west, hd.north, hd.east)\
		/hd.cols;
		
		/*calculate number of patch*/
		count =0;
		for(i = 0; i<ad->rl; i++){
			G_get_raster_row(fd, buf, i+ad->y, CELL_TYPE);
			if (read(mask_fd, mask_buf, (ad->cl * sizeof(int))) < 0)
				return 0;
			if(i>0){
				G_get_raster_row(fd, sup, i-1, CELL_TYPE);
			}
			/* mask values */
			for (j=0; j<ad->cl; j++){
				if(mask_buf[j] == 0)
					buf[ad->x +j] = null_value;
			}
			if (complete_line){
				if (! G_is_null_value( &(buf[ad->x]), CELL_TYPE) &&\
					buf[ad->x] != complete_value)
					count++;
				for(j=0; j<ad->cl; j++){
					if (buf[j+ad->x] != buf[j+1+ad->x]){
						complete_line=0;
						if(!G_is_null_value( &(buf[j+ad->x]), CELL_TYPE)&&\
							buf[j+ad->x] != complete_value)
							count++;
						}
				}
				if (complete_line){
					complete_value = buf[ad->x];
				}
			}
			else{
				complete_line = 1;
				for(j=0; j<ad->cl; j++){
					if (sup[j+ad->x] == buf[j+ad->x]){
						connected = 1;
					}
					if (buf[j+ad->x] != buf[j+1+ad->x]){
						complete_line =0;
						if (!connected && \
							!G_is_null_value( &(buf[j+ad->x]), CELL_TYPE)){
							count++;
							connected =0;									
						}
						else{
							connected=0;
						}
					}
				}
				if (complete_line)
					complete_value = buf[ad->x];
			}
		}
		#ifdef DEBUG
		printf("number of patch = %i\n", count);
		#endif
		#ifdef DEBUG
		printf("area = %f\n", area);
		#endif
		area = cell_dim * meters_row * meters_col;
		*result= (count/area)*10000;
		free(buf);
		free(sup);
		free(mask_buf);
		return 1;
	}
}



