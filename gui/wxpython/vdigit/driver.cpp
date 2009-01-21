/**
   \file driver.cpp
   
   \brief Experimental C++ wxWidgets display driver

   This driver is designed for wxPython GRASS GUI (digitization tool).
   Draw vector map layer to PseudoDC.

   (C) by the GRASS Development Team
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com>

   \date 2007-2008 
*/

#include <cmath>

#include "driver.h"

/**
   \brief Initialize driver

   Allocate given structures.
   
   \param[in,out] PseudoDC device where to draw vector objects
   \param[in,out] PseudoDC device where to draw vector objects (tmp, selected)
   
   \return
*/
DisplayDriver::DisplayDriver(void *device, void *deviceTmp)
{
    G_gisinit(""); /* GRASS functions */

    mapInfo = NULL;

    dc = (gwxPseudoDC *) device;
    dcTmp = (gwxPseudoDC *) deviceTmp;

    points = Vect_new_line_struct();
    pointsScreen = new wxList();
    cats = Vect_new_cats_struct();

    selected.field = -1;
    selected.cats = Vect_new_list();
    selected.ids = Vect_new_list();
    selected.idsDupl = Vect_new_list();

    drawSegments = false;

    G_set_verbose(0);
    
    // avoid GUI crash when G_fatal_error() is called (opening the vector map)
    // Vect_set_fatal_error(GV_FATAL_PRINT);
    // G_set_error_routine(print_error);
}

/**
   \brief Destroy driver

   Close the map, deallocate given structures.

   \param

   \return
*/
DisplayDriver::~DisplayDriver()
{
    if (mapInfo)
	CloseMap();

    Vect_destroy_line_struct(points);
    delete pointsScreen;
    Vect_destroy_cats_struct(cats);
    Vect_destroy_list(selected.cats);
    Vect_destroy_list(selected.ids);
    Vect_destroy_list(selected.idsDupl);
}

/**
   \brief Set device for drawing
   
   \param[in,out] PseudoDC device where to draw vector objects

   \return
*/
void DisplayDriver::SetDevice(void *device)
{
    dc = (gwxPseudoDC *) device;

    return;
}

/*
  \brief Close vector map layer
  
  \param void

  \return 0 on success
  \return non-zero on error
*/
int DisplayDriver::CloseMap()
{
    int ret;

    ret = -1;
    if (mapInfo) {
	if (mapInfo->mode == GV_MODE_RW) {
	    /* rebuild topology */
	    Vect_build_partial(mapInfo, GV_BUILD_NONE);
	    Vect_build(mapInfo);
	}
	/* close map and store topo/cidx */
	ret = Vect_close(mapInfo); 
	G_free ((void *) mapInfo);
	mapInfo = NULL;
    }
    
    return ret;
}

/**
   \brief Open vector map layer
 
   \param[in] mapname name of vector map
   \param[in] mapset name of mapset where the vector map layer is stored
   
   \return topo level
   \return -1 on error
*/
int DisplayDriver::OpenMap(const char* mapname, const char *mapset, bool update)
{
    int ret;

    if (!mapInfo)
	mapInfo = (struct Map_info *) G_malloc (sizeof (struct Map_info));

    // define open level (level 2: topology)
    Vect_set_open_level(2);

    // avoid GUI crash when G_fatal_error() is called (opening the vector map)
    Vect_set_fatal_error(GV_FATAL_PRINT);

    // open existing map
    if (!update) {
	ret = Vect_open_old(mapInfo, (char*) mapname, (char *) mapset);
    }
    else {
	ret = Vect_open_update(mapInfo, (char*) mapname, (char *) mapset);
    }

    if (ret == -1) { // error
	G_free((void *) mapInfo);
	mapInfo = NULL;
    }

    return ret;
}

/**
   \brief Reload vector map layer

   Close and open again. Needed for modification using v.edit.

   TODO: Get rid of that...

   \param
   
   \return
*/
void DisplayDriver::ReloadMap()
{
    // char* name   = G_store(Vect_get_map_name(mapInfo)); ???
    char* name   = G_store(mapInfo->name);
    char* mapset = G_store(Vect_get_mapset(mapInfo));

    Vect_close(mapInfo);
    mapInfo = NULL;

    OpenMap(name, mapset, false); // used only for v.edit
    //Vect_build_partial(mapInfo, GV_BUILD_NONE, stderr);
    //Vect_build(mapInfo, stderr);

    return;
}

/*
  \brief Conversion from geographic coordinates (east, north)
  to screen (x, y)
  
  TODO: 3D stuff...

  \param[in] east,north,depth geographical coordinates
  \param[out] x, y, z screen coordinates
  
  \return 
*/
void DisplayDriver::Cell2Pixel(double east, double north, double depth,
			       double *x, double *y, double *z)
{
    double n, w;
    /*
    *x = int((east  - region.map_west) / region.map_res);
    *y = int((region.map_north - north) / region.map_res);
    */
    w = region.center_easting  - (region.map_width / 2)  * region.map_res;
    n = region.center_northing + (region.map_height / 2) * region.map_res;

    /*
    *x = int((east  - w) / region.map_res);
    *y = int((n - north) / region.map_res);
    */
    if (x)
	*x = (east  - w) / region.map_res;
    if (y)
	*y = (n - north) / region.map_res;
    if (z)
	*z = 0.;

    return;
}

/**
   \brief Calculate distance in pixels

   \todo LL projection

   \param dist real distance
*/
double DisplayDriver::DistanceInPixels(double dist)
{
    double x;
    
    Cell2Pixel(region.map_west + dist, region.map_north, 0.0, &x, NULL, NULL);

    return std::sqrt(x * x);
}

/**
   \brief Set geographical region
 
   Region must be upgraded because of Cell2Pixel().
   
   \param[in] north,south,east,west,ns_res,ew_res region settings
 
   \return
*/
void DisplayDriver::SetRegion(double north, double south, double east, double west,
			      double ns_res, double ew_res,
			      double center_easting, double center_northing,
			      double map_width, double map_height)
{
    region.box.N  = north;
    region.box.S  = south;
    region.box.E  = east;
    region.box.W  = west;
    region.box.T  = PORT_DOUBLE_MAX;
    region.box.B  = -PORT_DOUBLE_MAX;
    region.ns_res = ns_res;
    region.ew_res = ew_res;

    region.center_easting = center_easting;
    region.center_northing = center_northing;

    region.map_width  = map_width;
    region.map_height = map_height;

    // calculate real region
    region.map_res = (region.ew_res > region.ns_res) ? region.ew_res : region.ns_res;

    region.map_west  = region.center_easting - (region.map_width / 2.) * region.map_res;
    region.map_north = region.center_northing + (region.map_height / 2.) * region.map_res;

    return;
}

/*
  \brief Set settings for displaying vector feature
 
  E.g. line width, color, ...
  
  \param[in] lineWidth,... settgings
  
  \return 
*/
void DisplayDriver::UpdateSettings(unsigned long highlight,
				   bool ehighlightDupl, unsigned long chighlightDupl,
				   bool ePoint,       unsigned long cPoint, /* enabled, color */
				   bool eLine,        unsigned long cLine,
				   bool eBoundaryNo,  unsigned long cBoundaryNo,
				   bool eBoundaryOne, unsigned long cBoundaryOne,
				   bool eBoundaryTwo, unsigned long cBoundaryTwo,
				   bool eCentroidIn,  unsigned long cCentroidIn,
				   bool eCentroidOut, unsigned long cCentroidOut,
				   bool eCentroidDup, unsigned long cCentroidDup,
				   bool eNodeOne,     unsigned long cNodeOne,
				   bool eNodeTwo,     unsigned long cNodeTwo,
				   bool eVertex,      unsigned long cVertex,
				   bool eArea,        unsigned long cArea,
				   bool eDirection,   unsigned long cDirection,
				   int lineWidth, int alpha)
{
    settings.highlight.Set(highlight);

    settings.highlightDupl.enabled = ehighlightDupl;
    settings.highlightDupl.color.Set(chighlightDupl);

    settings.point.enabled = ePoint;
    settings.point.color.Set(cPoint);
    
    settings.line.enabled = eLine;
    settings.line.color.Set(cLine);

    settings.boundaryNo.enabled = eBoundaryNo;
    settings.boundaryNo.color.Set(cBoundaryNo);
    settings.boundaryOne.enabled = eBoundaryOne;
    settings.boundaryOne.color.Set(cBoundaryOne);
    settings.boundaryTwo.enabled = eBoundaryTwo;
    settings.boundaryTwo.color.Set(cBoundaryTwo);


    settings.centroidIn.enabled = eCentroidIn;
    settings.centroidIn.color.Set(cCentroidIn);
    settings.centroidOut.enabled = eCentroidOut;
    settings.centroidOut.color.Set(cCentroidOut);
    settings.centroidDup.enabled = eCentroidDup;
    settings.centroidDup.color.Set(cCentroidDup);

    settings.nodeOne.enabled = eNodeOne;
    settings.nodeOne.color.Set(cNodeOne);
    settings.nodeTwo.enabled = eNodeTwo;
    settings.nodeTwo.color.Set(cNodeTwo);

    settings.vertex.enabled = eVertex;
    settings.vertex.color.Set(cVertex);

    settings.area.enabled = eArea;
    settings.area.color.Set(cArea);
    settings.area.color.Set(settings.area.color.Red(),
			    settings.area.color.Green(),
			    settings.area.color.Blue(),
			    alpha);
    
    settings.direction.enabled = eDirection;
    settings.direction.color.Set(cDirection);

    settings.lineWidth = lineWidth;
    
    return;
}

/**
   \brief Prints gId: dcIds

   Useful for debugging purposes.

   \param

   \return
*/
void DisplayDriver::PrintIds()
{
    std::cerr << "topology.highlight: " << topology.highlight << std::endl;

    std::cerr << "topology.point: " << topology.point << std::endl;
    std::cerr << "topology.line: " << topology.line << std::endl;

    std::cerr << "topology.boundaryNo: " << topology.boundaryNo << std::endl;
    std::cerr << "topology.boundaryOne: " << topology.boundaryOne << std::endl;
    std::cerr << "topology.boundaryTwo: " << topology.boundaryTwo << std::endl;

    std::cerr << "topology.centroidIn: " << topology.centroidIn << std::endl;
    std::cerr << "topology.centroidOut: " << topology.centroidOut << std::endl;
    std::cerr << "topology.centroidDup: " << topology.centroidDup << std::endl;

    std::cerr << "topology.nodeOne: " << topology.nodeOne << std::endl;
    std::cerr << "topology.nodeTwo: " << topology.nodeTwo << std::endl;

    std::cerr << "topology.vertex: " << topology.vertex << std::endl;

    std::cerr << std::endl << "nobjects: "
	      << topology.point * 2 + // cross
      topology.line + 
      topology.boundaryNo +
      topology.boundaryOne +
      topology.boundaryTwo +
      topology.centroidIn * 2 +
      topology.centroidOut * 2 +
      topology.centroidDup * 2 +
      topology.nodeOne * 2 +
      topology.nodeTwo * 2 +
      topology.vertex * 2 << std::endl;

    std::cerr << "selected: ";

    for (int i = 0; i < selected.ids->n_values; i++) {
	std::cerr << selected.ids->value[i] << " ";
    }
    std::cerr << std::endl;

    return;
}

/**
   \brief Reset topology structure.

   \return
*/
void DisplayDriver::ResetTopology()
{
    topology.highlight = 0;
    
    topology.point = 0;
    topology.line = 0;
    
    topology.boundaryNo = 0;
    topology.boundaryOne = 0;
    topology.boundaryTwo = 0;
    
    topology.centroidIn = 0;
    topology.centroidOut = 0;
    topology.centroidDup = 0;
    
    topology.nodeOne = 0;
    topology.nodeTwo = 0;
    
    topology.vertex = 0;

    return;
}

/**
   \brief Convert vect list to std::vector

   \param list vect list

   \return std::vector
*/
std::vector<int> DisplayDriver::ListToVector(struct ilist *list)
{
    std::vector<int> vect;

    if (!list)
	return vect;

    for (int i = 0; i < list->n_values; i++) {
	vect.push_back(list->value[i]);
    }

    return vect;
}

/**
   \brief Convert std::vector to vect list

   \param list vect list
   \param vec  std::vector instance

   \return number of items
   \return -1 on error
*/
int DisplayDriver::VectorToList(struct ilist *list, const std::vector<int>& vec)
{
    if (!list)
	return -1;

    Vect_reset_list(list);

    for (std::vector<int>::const_iterator i = vec.begin(), e = vec.end();
	 i != e; ++i) {
	Vect_list_append(list, *i);
    }

    return list->n_values;
}

/**
   \brief Get bounding box of (opened) vector map layer

   \return (w,s,b,e,n,t)
*/
std::vector<double> DisplayDriver::GetMapBoundingBox()
{
    std::vector<double> region;
    BOUND_BOX bbox;

    if (!mapInfo) {
	return region;
    }
    
    Vect_get_map_box(mapInfo, &bbox);

    region.push_back(bbox.W);
    region.push_back(bbox.S);
    region.push_back(bbox.B);

    region.push_back(bbox.E);
    region.push_back(bbox.N);
    region.push_back(bbox.T);

    return region;
}

/**
   \brief Error messages handling 

   \param msg message
   \param type type message (MSG, WARN, ERR)

   \return 0
*/
int print_error(const char *msg, int type)
{
    fprintf(stderr, "%s", msg);
    
    return 0;
}
