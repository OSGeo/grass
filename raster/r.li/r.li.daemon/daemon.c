/**
 * \file daemon.c
 *
 * \brief Implementation of the server for parallel
 * computing of r.li raster analysis
 *
 * \author Claudio Porta & Lucio Davide Spano
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 * \version 1.0
 *
 * \include
 * 
 */
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>

#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>
#include "daemon.h"


int calculateIndex(char *file, int f(int, char **, area_des, double *),\
 char** parameters, char *raster, char *output){
	
	char pathSetup[150],out[150], parsed;
	char * reportChannelName, * random_access_name;
	struct History history;
	g_areas g;
	int receiveChannel; 
	int res;
	wd child[WORKERS];
	int i, mypid, doneDir, withoutJob, mv_fd, random_access;
	/*int mv_rows, mv_cols;*/
	list l;
	msg m,doneJob;
	/* int perc=0; */
	
	g = (g_areas) G_malloc(sizeof(struct generatore));
	l = (list) G_malloc(sizeof(struct lista));
	mypid = getpid();
	/* create report pipe */
	reportChannelName = G_tempfile();
	if (mkfifo(reportChannelName, 0644) == -1)
		G_fatal_error("Error in pipe creation");
	
	
	/*###############################################
	  --------------create childs-------------------
	  ###############################################*/
	
	i = 0;
	while(i<WORKERS){
		int childpid;
		/*creating pipe*/
		child[i].pipe = G_tempfile();
		if( mkfifo(child[i].pipe, 0755)== -1)
			G_fatal_error( _("Error in pipe creation"));
		childpid = fork();
		if(childpid) {
			/*father process*/
			child[i].pid = childpid;
			child[i].channel = open(child[i].pipe, O_WRONLY|O_CREAT, 0755);
			
			if ( child[i].channel == -1){
				G_fatal_error( _("Error opening channel %i"), i);
			}
			i++;
		}
		else {
		/*child process*/
			worker(raster, f, reportChannelName, child[i].pipe, parameters);
			exit(0);
		}
	}
	
	/*open reportChannel*/
	receiveChannel = open(reportChannelName, O_RDONLY, 0755);
	
	/*########################################################	
	  -----------------create area queue----------------------
	  #########################################################*/

	/* TODO: check if this path is portable */
	sprintf(pathSetup, "%s/.r.li/history/%s", getenv("HOME"), file);
	parsed = parseSetup(pathSetup, l, g, raster);
	
	/*########################################################
      -----------------open output file ---------------------
	  #######################################################*/
	
	if (parsed == MVWIN)
	{
		/* struct Cell_head cellhd_r, cellhd_new;
		   char *mapset; */
		/*creating new raster file*/
		mv_fd = G_open_raster_new(output, DCELL_TYPE);
		if (mv_fd < 0)
			G_fatal_error( _("Unable to create raster map <%s>"), output);
			
		random_access_name= G_tempfile();
		random_access = open(random_access_name, O_RDWR|O_CREAT, 0755);
		if(random_access == -1)
			G_fatal_error( _("Cannot create random access file"));
	}
	else {
		/*check if ~/.r.li/output exist*/
		sprintf(out, "%s/.r.li/", getenv("HOME"));	
		
		doneDir = G_mkdir(out);
		if (doneDir == -1 && errno != EEXIST)
			G_fatal_error( _("Cannot create %s/.r.li/ directory"), getenv("HOME"));
		sprintf(out, "%s/.r.li/output", getenv("HOME"));
		doneDir = G_mkdir(out);
		if (doneDir == -1 && errno != EEXIST)
			G_fatal_error( _("Cannot create %s/.r.li/output/ directory"), getenv("HOME"));
		sprintf(out, "%s/.r.li/output/%s", getenv("HOME"), output);
		res = open(out, O_WRONLY|O_CREAT| O_TRUNC, 0755);
	}
	i = 0;
	
	/*#######################################################
	  ------------------analysis loop----------------------
	  #######################################################*/
	/*first job scheduling*/
	while ((i<WORKERS) && next_Area(parsed, l, g, &m)!=0){
			send(child[i].channel, &m);
			i++;
	}

	
	/*body*/
	while(next_Area(parsed, l, g, &m)!=0){
		int j=0, donePid;
		
		receive(receiveChannel, &doneJob);
		/*perc++;*/
		/*G_percent (perc, WORKERS, 1);*/
		if (doneJob.type == DONE){
			double result;
			donePid = doneJob.f.f_d.pid;
			result = doneJob.f.f_d.res;
			/*output*/
			if(parsed != MVWIN){
				print_Output(res, doneJob);
			}
			else{
				/*raster output*/
				raster_Output(random_access, doneJob.f.f_d.aid, g, doneJob.f.f_d.res);
			}
		}
		else {
			donePid = doneJob.f.f_e.pid;
			if(parsed != MVWIN){
				error_Output(res, doneJob);
			}
			else{
				/*printf("todo ");fflush(stdout); */ /* TODO scrivere su raster NULL ??? */
			}
		}
		j=0;
		
		
		while (j<WORKERS && donePid!= child[j].pid)
			j++;
		send(child[j].channel, &m);	
			
	}
	
	/*kill childs*/
	withoutJob = i;
	while (i > 0) {
		int j=0, donePid, status;
		receive(receiveChannel, &doneJob);
		if (doneJob.type == DONE){
			double result;
			donePid = doneJob.f.f_d.pid;
			result = doneJob.f.f_d.res;
			if(parsed != MVWIN){
				print_Output(res, doneJob);
			}
			else{
				/* raster */
				raster_Output(random_access, doneJob.f.f_d.aid, g, doneJob.f.f_d.res);
			}
		}
		else {
			donePid = doneJob.f.f_e.pid;
			if(parsed != MVWIN){
				error_Output(res, doneJob);
			}
			else{
				/*printf("todo2 ");fflush(stdout);*/ /*TODO scrivere su raster*/
			}
		}
		i--;
		while (j<WORKERS && donePid!= child[j].pid)
			j++;
			
		m.type = TERM;
		m.f.f_t.pid = mypid;
		send(child[j].channel, &m);
		wait(&status);
		if (! (WIFEXITED(status)))
			G_message( _("r.li.worker (pid %i) exited with abnormal status %i"), \
				donePid, status);
		else
			G_message( _("r.li.worker (pid %i) terminated"), donePid);
		
		/*remove pipe*/
		if (close(child[j].channel) != 0)
			G_message( _("Cannot close %s file (PIPE)"), child[j].pipe);
		if (unlink(child[j].pipe) != 0)
			G_message( _("Cannot delete %s file (PIPE)"), child[j].pipe);
	}
	
	/*kill childs without Job*/
	for (i = withoutJob; i<WORKERS; i++){
		int status;
		m.type = TERM;
		m.f.f_t.pid = mypid;
		send(child[i].channel, &m);
		wait(&status);
		if (! (WIFEXITED(status)))
			G_message( _("r.li.worker (pid %i) exited with abnormal status %i"), \
				child[i].pid, status);
		else
			G_message( _("r.li.worker (pid %i) terminated"), child[i].pid);
		/*remove pipe*/
		if (close(child[i].channel) != 0)
			G_message( _("Cannot close %s file (PIPE2)"), child[i].pipe);
		if (unlink(child[i].pipe) != 0)
			G_message( _("Cannot delete %s file (PIPE2)"), child[i].pipe);
	}
	/*################################################
	  --------------delete tmp files------------------
	  ################################################*/
	
	
	if(parsed == MVWIN){
		write_raster(mv_fd, random_access, g);
		close(random_access);
		unlink(random_access_name);
		G_close_cell(mv_fd);
		G_short_history(output, "raster", &history);
		G_command_history(&history);
		G_write_history(output, &history);
	}
	
	
	if (close(receiveChannel) != 0)
			G_message( _("Cannot close receive channel file"));
	if (unlink(reportChannelName) != 0)
			G_message( _("Cannot delete %s file"), child[i].pipe);
	return 1;
}

int parseSetup(char *path, list l, g_areas g, char *raster){
	struct stat s;
	struct Cell_head cellhd;
	char * buf, *token, *mapset;/* g_region[350]; */
	int setup;
	int letti;
	double rel_x, rel_y, rel_rl, rel_cl;
	double sf_n, sf_s, sf_e, sf_w;
	int sf_x, sf_y, sf_rl, sf_cl;
	int size;
	
	if (stat(path, &s) != 0)
		G_fatal_error( _("Cannot make stat of %s configuration file"), path);
	size = s.st_size * sizeof(char);
	buf = G_malloc(size);
	setup = open(path, O_RDONLY, 0755);
	if (setup == -1)
		G_fatal_error( _("Cannot read setup file"));
	letti = read(setup, buf, s.st_size);
	if (letti < s.st_size)
		G_fatal_error( _("Cannot read setup file"));
	
	
	token = strtok(buf, " ");
	if (strcmp("SAMPLINGFRAME", token) != 0)
		G_fatal_error( _("Illegal configuration file"));
	rel_x=atof(strtok(NULL, "|"));
	rel_y=atof(strtok(NULL, "|"));
	rel_rl=atof(strtok(NULL,"|"));
	rel_cl=atof(strtok(NULL,"\n"));
	
	/*finding raster map*/
	mapset = G_find_cell(raster, "");
	if (G_get_cellhd(raster, mapset, &cellhd) == - 1)
		G_fatal_error( _("Cannot read raster header file"));
	/*calculating absolute sampling frame definition*/
	sf_x = (int) rint(cellhd.cols * rel_x);
	sf_y = (int) rint(cellhd.rows * rel_y);
	sf_rl = (int) rint(cellhd.rows * rel_rl);
	sf_cl = (int) rint(cellhd.cols * rel_cl);
	
	/*calculating sample frame boundaries*/
	sf_n = cellhd.north - (cellhd.ns_res * sf_y);
	sf_s = sf_n - (cellhd.ns_res * sf_rl);
	sf_w = cellhd.west + (cellhd.ew_res * sf_x);
	sf_e = sf_w + (cellhd.ew_res * sf_cl);
	
	/* parsing configuration file */
	token = strtok(NULL, " ");
	
	if (strcmp("SAMPLEAREA", token) == 0){
		double rel_sa_x, rel_sa_y, rel_sa_rl, rel_sa_cl;
		int aid = 1, toReturn;
		do{
			rel_sa_x = atof(strtok(NULL, "|"));
			rel_sa_y = atof(strtok(NULL, "|"));
			rel_sa_rl = atof(strtok(NULL, "|"));
			rel_sa_cl = atof(strtok(NULL, "\n"));
			
			if (rel_sa_x == -1.0 && rel_sa_y == -1.0) {
				
				/* runtime disposition */
				
				int sa_rl, sa_cl;
				sa_rl = (int) rint(cellhd.rows * rel_sa_rl);
				sa_cl = (int) rint(cellhd.cols * rel_sa_cl);
				g->rows = sf_rl;
				g->cols = sf_cl;
				g->rl = sa_rl;
				g->cl = sa_cl;
				g->count = 1;
				g->sf_x = sf_x;
				g->sf_y = sf_y;
				g->x=sf_x;
				g->y = sf_y;
				return disposeAreas(l, g, strtok(NULL, "\n"));
			}
			else {
				msg m;
				toReturn = NORMAL;
				/*read file and create list*/
				m.type = AREA;
				m.f.f_a.x = (int) rint(cellhd.cols * rel_sa_x);
				m.f.f_a.y = (int) rint(cellhd.rows * rel_sa_y);
				m.f.f_a.rl = (int) rint(cellhd.rows * rel_sa_rl);
				m.f.f_a.cl = (int) rint(cellhd.cols * rel_sa_cl);
				m.f.f_a.aid = aid;
				aid++;
				insertNode(l, m);
			}
		}
		while ( (token = strtok(NULL, " ")) != NULL && \
				strcmp(token, "SAMPLEAREA") == 0);
		close (setup);
		return toReturn;		
	} 
	else if (strcmp("MASKEDSAMPLEAREA", token) == 0){
		double rel_sa_x, rel_sa_y, rel_sa_rl, rel_sa_cl;
		int aid = 1;
		char maskname[150];
		do{
			rel_sa_x = atof(strtok(NULL, "|"));
			rel_sa_y = atof(strtok(NULL, "|"));
			rel_sa_rl = atof(strtok(NULL, "|"));
			rel_sa_cl = atof(strtok(NULL, "|"));
			strcpy(maskname, strtok(NULL,"\n"));
			
			if (rel_sa_x == -1 && rel_sa_y == -1) {
				/* runtime disposition */
				int sa_rl, sa_cl;
				sa_rl = (int) rint(cellhd.rows * rel_sa_rl);
				sa_cl = (int) rint(cellhd.cols * rel_sa_cl);
				g->rows = sf_rl;
				g->cols = sf_cl;
				g->rl = sa_rl;
				g->cl = sa_cl;
				g->count = 1;
				g->x = sf_x;
				g->y = sf_y;
				g->maskname = maskname;
				return disposeAreas(l, g, strtok(NULL, "\n"));
			}
			else {
				/*read file and create list*/
				msg m;
				m.type = MASKEDAREA;
				m.f.f_ma.x = (int) rint(cellhd.cols * rel_sa_x);
				m.f.f_ma.y = (int) rint(cellhd.rows * rel_sa_y);
				m.f.f_ma.rl = (int) rint(cellhd.rows * rel_sa_rl);
				m.f.f_ma.cl = (int) rint(cellhd.cols * rel_sa_cl);
				m.f.f_ma.aid = aid;
				strcpy(m.f.f_ma.mask, maskname);
				aid++;
				insertNode(l, m);
			}
		}
		while ( (token = strtok(NULL, " ")) != NULL && \
				strcmp(token, "MASKEDSAMPLEAREA") == 0); 
		close(setup);
		return NORMAL;
	}
	else if (strcmp("MASKEDOVERLAYAREA", token) == 0){
		double sa_n, sa_s, sa_w, sa_e;
		int aid = 1;
		char maskname[150];
		msg m;
		do {
			strcpy(maskname, strtok(NULL,"|"));
			sa_n = atof(strtok(NULL, "|"));
			sa_s = atof(strtok(NULL, "|"));
			sa_e = atof(strtok(NULL, "|"));
			sa_w = atof(strtok(NULL, "\n"));
			
			m.type = MASKEDAREA;
			m.f.f_ma.x = (int) rint((cellhd.north - sa_n) * cellhd.ns_res);
			m.f.f_ma.y = (int) rint((cellhd.west + sa_w) * cellhd.ew_res);
			m.f.f_ma.rl = (int) rint((sa_n -sa_s) * cellhd.ns_res);
			m.f.f_ma.cl = (int) rint((sa_e - sa_w) * cellhd.ew_res);
			m.f.f_ma.aid = aid;
			strcpy(m.f.f_ma.mask, maskname);
			aid++;
			insertNode(l, m);
		}
		while( (token = strtok(NULL, " ")) != NULL && \
				(strcmp(token,"MASKEDOVERLAYAREA") == 0));
		if (strcmp(token, "RASTERMAP") != 0)
			G_fatal_error( _("Irregular maskedoverlay areas definition"));
		token = strtok(NULL, "\n");
		if(strcmp(token, raster) != 0)
			G_fatal_error( _("The configuration file can be used only with \
			%s rasterfile"), token);
		close(setup);
		return NORMAL;
		
	}
	else
		G_fatal_error( _("Illegal configuration file (sample area)"));
	close(setup);
	return ERROR;
}

int disposeAreas(list l, g_areas g, char *def){
	char *token;
	token = strtok(def, " \n");
	if (strcmp(token, "MOVINGWINDOW") == 0){
		g->count = 0;
		g->dist=0;
		g->add_row=1;
		g->add_col=1;
		if(g->rl != 1)
			g->rows = g->rows - g->rl+1;
		else 
			g->rows = g->rows;
		if(g->cl != 1)
			g->cols = g->cols - g->cl+1;
		return MVWIN;
	}
	else if (strcmp(token, "RANDOMNONOVERLAPPING") == 0){
		int units, sf_rl, sf_cl, sa_rl, sa_cl, max_units, i;
		int *assigned;
		sscanf(strtok(NULL, "\n"), "%i", &units);
		sf_rl = g->rows;
		sf_cl = g->cols;
		sa_rl = g->rl;
		sa_cl = g->cl;
		max_units = (int) rint((sf_rl/sa_rl) * (sf_cl/sa_cl));
		if (units > max_units)
			G_fatal_error( _("Too many units to place"));
		assigned = G_malloc(units*sizeof(int));
		i =0;
		srandom(getpid());
		while(i<units){
			int j, position, found=FALSE;
			position = random() % max_units;
			for (j=0; j<i; j++){
				if (assigned[j] == position)
					found = TRUE;
			}
			if (!found){
				msg m;
				assigned[i] = position;
				i++;
				if(g->maskname == NULL){
					int n_col = rint(sf_cl / sa_cl);
					m.type = AREA;
					m.f.f_a.aid = i;
					m.f.f_a.x = g->sf_x +(position % n_col) * sa_cl;	
					m.f.f_a.y = g->sf_y + (position / n_col) * sa_rl;
					m.f.f_a.rl = sa_rl;
					m.f.f_a.cl = sa_cl;
					insertNode(l, m);
				}
				else {
					int n_col = sf_cl / sa_cl;
					m.type = MASKEDAREA;
					m.f.f_ma.aid = i;
					m.f.f_a.x = g->sf_x +(position % n_col) * sa_cl;	
					m.f.f_a.y = g->sf_y + (position / n_col) * sa_rl;
					m.f.f_ma.rl = sa_rl;
					m.f.f_ma.cl = sa_cl;
					strcpy(m.f.f_ma.mask,g->maskname);
					insertNode(l, m);
				}
			}
		}
		return NORMAL;
	}
	else if (strcmp(token, "SYSTEMATICCONTIGUOUS") == 0){
		g->dist=0;
		g->add_row= g->rl;
		g->add_col= g->cl;
		return GEN;
	}
	else if (strcmp(token, "SYSTEMATICNONCONTIGUOUS") == 0){
		int dist;
		dist=atoi(strtok(NULL, "\n"));
		g->dist = dist;
		g->add_row = g->rl + dist;
		g->add_col = g->cl + dist;
		g->x = g->sf_x + dist;
		g->y = g->sf_y + dist;
		return GEN;		
	}
	else if (strcmp(token, "STRATIFIEDRANDOM") == 0){
		int r_strat, c_strat, r_strat_len, c_strat_len, loop, i;
		r_strat=atoi(strtok(NULL, "|"));
		c_strat = atoi(strtok(NULL, "\n"));
		r_strat_len = (int) rint(g->rows/r_strat);
		c_strat_len = (int) rint(g->cols/c_strat);
		if (r_strat_len < g->rl || c_strat_len < g->cl)
			G_fatal_error( _("Too many strats for raster map"));
		loop = r_strat * c_strat;
		srandom(getpid());
		for (i=0; i<loop ; i++){
			msg m;
			if( g->maskname == NULL){
				m.type = AREA;
				m.f.f_a.aid = i;
				m.f.f_a.x =  (int) g->sf_x + ((i%c_strat) * c_strat_len) + \
					(random() % (c_strat_len - g->cl));
				m.f.f_a.y = (int) g->sf_y + (rint(i / c_strat) * r_strat_len) + \
					(random() % (r_strat_len - g->rl));
				m.f.f_a.rl = g->rl;
				m.f.f_a.cl = g->cl;
				insertNode(l, m);
			}
			else {
				m.type = MASKEDAREA;
				m.f.f_ma.aid = i;
				m.f.f_ma.x =  (int) g->sf_x + ((i%c_strat) * c_strat_len) + \
					(random() % (c_strat_len - g->cl));
				m.f.f_ma.y = (int) g->sf_y + (rint(i / c_strat) * r_strat_len) + \
					(random() % (r_strat_len - g->rl));
				m.f.f_ma.rl = g->rl;
				m.f.f_ma.cl = g->cl;
				strcpy(m.f.f_ma.mask,g->maskname);
				insertNode(l, m);
			}
		}
		return NORMAL;
	}
	else {
		G_fatal_error( _("Illegal areas disposition"));
		return NORMAL;
	}
	return ERROR;
}

int next_Area(int parsed, list l, g_areas g, msg *m){
	if (parsed == NORMAL){
		if ( l->size == 0) 
			return 0;
		else{
			msg tmp;
			memcpy(&tmp, l->head->m, sizeof(msg));
			*m = tmp;
			removeNode(l);
			return 1;
		}
	}
	else{
		return next(g, m);
	}
}

int print_Output(int out, msg m){
	if (m.type != DONE)
		return 0;
	else {
		char s[100];
		int len;
		sprintf(s, "RESULT %i|%f\n", m.f.f_d.aid, m.f.f_d.res);
		len = strlen(s);
		if(write(out, s, len) == len)
			return 1;
		else 
			return 0;
	}
}

int error_Output(int out, msg m){
	if (m.type != ERROR)
		return 0;
	else {
		char s[100];
		sprintf(s, "ERROR %i", m.f.f_d.aid);
		if(write(out, s, strlen(s)) == strlen(s))
			return 1;
		else 
			return 0;
	}
}

int raster_Output(int fd, int aid, g_areas g, double res){
	double toPut = res;
	off_t offset = (off_t) aid * sizeof(double);
	if (lseek(fd, offset, SEEK_SET) != offset){
		G_message( _("Cannot make lseek"));
		return -1;
	}
	if (write(fd, &toPut, sizeof(double)) == 0)
		return 1;
	else 
		return 0;
}
int write_raster(int mv_fd, int random_access, g_areas g){
	int i=0,j=0, letti=0;
	double * file_buf;
	DCELL * cell_buf;
	int cols, rows, center;
	
	cols = g->cols;
	rows = g->rows;
	center = g->sf_x + ((int)g->cl/2);

	file_buf = malloc(cols * sizeof(double));
	lseek(random_access, 0, SEEK_SET);
	cell_buf = G_allocate_d_raster_buf();
	G_set_d_null_value(cell_buf, G_window_cols() + 1);
	for(i= 0; i< g->sf_y + ((int) g->rl / 2); i++){
		G_put_raster_row(mv_fd, cell_buf, DCELL_TYPE);
	}
	for(i = 0; i<rows; i++){
		letti = read(random_access, file_buf, (cols * sizeof(double)));
		if (letti == -1 )
			G_message( "%s", strerror(errno));
		for (j=0; j<cols; j++){
			cell_buf[j+center]= file_buf[j];
		}
		G_put_raster_row(mv_fd, cell_buf, DCELL_TYPE);
	}
	G_set_d_null_value(cell_buf, G_window_cols() + 1);
	for( i=0; i< G_window_rows() - g->sf_y - g->rows; i++)
		G_put_raster_row(mv_fd, cell_buf, DCELL_TYPE);
	return 1;
}
