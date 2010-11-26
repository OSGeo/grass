/*
 * \file index.h
 *
 * \brief declaration of functions for r.li raster analysis
 *  
 * \author Claudio Porta, Lucio Davide Spano, Serena Pallecchi students of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
  *        Luca Delucchi and Duccio Rocchini, Fondazione Edmund Mach, Italy: r.li.pielou, r.li.renyi
 *
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 * 
 * \version 1.1
 *
 */


 /* #################################################
    ADD HERE INDEX DECLARATIONS
    ################################################# */



 /**
  * \brief calculate patch density index on selected area
  * the abstract function is patch_density= patch_number / area
  */
int patch_density(int fd, char **par, area_des ad, double *result);

int patch_number(int fd, char **par, area_des ad, double *result);
int shape_index(int fd, char **par, area_des ad, double *result);
int shannon(int fd, char **par, area_des ad, double *result);

int pielou(int fd, char **par, area_des ad, double *result);
int renyi(int fd, char **par, area_des ad, double *result);

int simpson(int fd, char **par, area_des ad, double *result);
int meanPatchSize(int fd, char **par, area_des ad, double *result);
int meanPixelAttribute(int fd, char **par, area_des ad, double *result);
int contrastWeightedEdgeDensity(int fd, char **par, area_des ad,
				double *result);
int edgedensity(int fd, char **valore, area_des ad, double *result);
int patchAreaDistributionCV(int fd, char **par, area_des ad, double *result);
int patchAreaDistributionMN(int fd, char **par, area_des ad, double *result);
int patchAreaDistributionSD(int fd, char **par, area_des ad, double *result);
int patchAreaDistributionRANGE(int fd, char **par, area_des ad,
			       double *result);
int dominance(int fd, char **par, area_des ad, double *result);
int richness(int fd, char **par, area_des ad, double *result);
