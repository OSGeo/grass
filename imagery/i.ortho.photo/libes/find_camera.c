
/**************************************************************
* I_find_camera (camera)
*
* Look for the named camera reference file in the current mapset
**************************************************************/
#include <grass/gis.h>

int I_find_camera(char *camera)
{
    if (camera == NULL || *camera == 0)
	return 0;

    return G_find_file("camera", camera, G_mapset()) != NULL;
}

int I_find_camera_file(char *camera, char *file)
{
    char element[100];

    if (camera == NULL || *camera == 0)
	return 0;
    if (file == NULL || *file == 0)
	return 0;

    sprintf(element, "camera");

    return G_find_file(element, camera, G_mapset()) != NULL;
}
