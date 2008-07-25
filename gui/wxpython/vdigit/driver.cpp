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
   
   \return
*/
DisplayDriver::DisplayDriver(void *device)
{
    G_gisinit(""); /* GRASS functions */

    mapInfo = NULL;

    dc = (wxPseudoDC *) device;

    points = Vect_new_line_struct();
    pointsScreen = new wxList();
    cats = Vect_new_cats_struct();
    
    selected = Vect_new_list();
    selectedDupl = Vect_new_list();

    drawSegments = false;

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
    Vect_destroy_list(selected);
    Vect_destroy_list(selectedDupl);
}

/**
   \brief Set device for drawing
   
   \param[in,out] PseudoDC device where to draw vector objects

   \return
*/
void DisplayDriver::SetDevice(void *device)
{
    dc = (wxPseudoDC *) device;

    return;
}

/**
   \brief Draw content of the vector map to device
   
   \return number of lines which were drawn
   \return -1 on error
 */
int DisplayDriver::DrawMap(bool force)
{
    if (!mapInfo || !dc)
	return -1;

    int nlines;
    BOUND_BOX mapBox;
    struct ilist *listLines;

    // ids.clear();
    listLines = Vect_new_list();

    ResetTopology();

    /* nlines = Vect_get_num_lines(mapInfo); */

    Vect_get_map_box(mapInfo, &mapBox);

    // draw lines inside of current display region
    nlines = Vect_select_lines_by_box(mapInfo, &(region.box),
     				      GV_POINTS | GV_LINES, // fixme
				      listLines);

    G_debug(3, "wxDriver.DrawMap(): region: w=%f, e=%f, s=%f, n=%f",
	    region.box.W, region.box.E, region.box.S, region.box.N);

    dc->BeginDrawing();

    if (settings.area.enabled) {
	/* draw area fills first */
	int area, centroid, isle;
	int num_isles;
	bool draw;
	struct ilist *listAreas, *listCentroids;
	struct line_pnts *points, *ipoints, **isles;

	wxBrush *fillArea, *fillIsle;

	fillArea = new wxBrush(settings.area.color);
	fillIsle = new wxBrush(*wxWHITE_BRUSH);
	
	listAreas = Vect_new_list();
	listCentroids = Vect_new_list();
	
	points = Vect_new_line_struct();
	ipoints = NULL;

	Vect_select_areas_by_box(mapInfo, &region.box,
				 listAreas);

	for (int i = 0; i < listAreas->n_values; i++) {
	    area = listAreas->value[i];
	    
	    if (!Vect_area_alive (mapInfo, area))
		return -1;

	    /* check for other centroids -- only area with one centroid is valid */
	    centroid = Vect_get_area_centroid(mapInfo, area);

	    if(centroid > 0) {
		/* check for isles */
		num_isles = Vect_get_area_num_isles(mapInfo, area); /* TODO */
		if (num_isles < 1)
		    isles = NULL;
		else
		    isles = (struct line_pnts **) G_malloc(num_isles * sizeof(struct line_pnts *));
		for (int j = 0; j < num_isles; j++) {
		    ipoints = Vect_new_line_struct();
		    isle = Vect_get_area_isle(mapInfo, area, j);

		    if (!Vect_isle_alive (mapInfo, isle))
			return -1;

		    Vect_get_isle_points(mapInfo, isle, ipoints);
		    isles[j] = ipoints;
		}

		Vect_get_area_points(mapInfo, area, points);

		/* avoid processing areas with large number of polygon points (ugly) */
		if (points->n_points < 5000) {
		    Vect_select_lines_by_polygon(mapInfo, points,
						 num_isles, isles, GV_CENTROID, listCentroids);
		}
		else {
		    Vect_reset_list(listCentroids);
		}

		draw = true;
		for (int c = 0; c < listCentroids->n_values; c++) {
		    if(Vect_get_centroid_area(mapInfo, listCentroids->value[c]) < 0) {
			draw = false;
			break;
		    }
		}
		
		if (draw) {
		    dc->SetBrush(*fillArea);
		    dc->SetPen(*wxTRANSPARENT_PEN);
		    DrawArea(points);

		    for (int j = 0; j < num_isles; j++) {
			/* draw isles in white */
			dc->SetBrush(*fillIsle);
			dc->SetPen(*wxTRANSPARENT_PEN);
			DrawArea(isles[j]);
		    }
		}

		if(isles) {
		    for (int j = 0; j < num_isles; j++) {
			Vect_destroy_line_struct(isles[j]);
			isles[j] = NULL;
		    }
		    G_free((void *) isles);
		}
	    }
	}

	delete fillArea;
	delete fillIsle;

	Vect_destroy_line_struct(points);

	Vect_destroy_list(listAreas);
	Vect_destroy_list(listCentroids);
    }

    for (int i = 0; i < listLines->n_values; i++) {
	DrawLine(listLines->value[i]);
    }
    dc->EndDrawing();

    // PrintIds();
    
    Vect_destroy_list(listLines);

    return listLines->n_values;
}	

/**
   \brief Draw area fill

   \param area boundary points

   \return 1 on success
   \return -1 on failure (vector object is dead, etc.)
*/
int DisplayDriver::DrawArea(const line_pnts* points)
{
    double x, y, z;

    // convert EN -> xy
    wxPoint wxPoints[points->n_points];

    for (int i = 0; i < points->n_points; i++) {
	Cell2Pixel(points->x[i], points->y[i], points->z[i],
		   &x, &y, &z);
	wxPoints[i] = wxPoint((int) x, (int) y);
    }

    // draw polygon
    dc->DrawPolygon(points->n_points, wxPoints);

    return 1;
}

/**
   \brief Draw selected vector objects to the device
 
   \param[in] line id

   \return 1 on success
   \return -1 on failure (vector object is dead, etc.)
*/
int DisplayDriver::DrawLine(int line)
{
    int dcId;       // 0 | 1 | segment id
    int type;       // line type
    double x, y, z; // screen coordinates
    bool draw;      // draw object ?
    wxPen *pen;
    
    pen = NULL;
    draw = false;

    if (!dc || !Vect_line_alive (mapInfo, line))
	return -1;

    // read line
    type = Vect_read_line (mapInfo, points, cats, line);

    // add ids
    // -> node1, line1, vertex1, line2, ..., node2
    // struct lineDesc desc = {points->n_points, dcId};
    // ids[line] = desc;
    // update id for next line
    // dcId += points->n_points * 2 - 1;

    if (IsSelected(line)) { // line selected ?
	if (settings.highlightDupl.enabled && IsDuplicated(line)) {
	    pen = new wxPen(settings.highlightDupl.color, settings.lineWidth, wxSOLID);
	}
	else {
	    pen = new wxPen(settings.highlight, settings.lineWidth, wxSOLID);
	}
	if (drawSelected) {
	    draw = true;
	}
	else {
	    draw = false;
	}
	dcId = 1;
	topology.highlight++;
    }
    else {
	dcId = 0;
	if (type & GV_LINES) {
	    switch (type) {
	    case GV_LINE:
		pen = new wxPen(settings.line.color, settings.lineWidth, wxSOLID);
		topology.line++;
		draw = settings.line.enabled;
		break;
	    case GV_BOUNDARY:
		int left, right;
		Vect_get_line_areas(mapInfo, line,
				    &left, &right);
		if (left == 0 && right == 0) {
		    pen = new wxPen(settings.boundaryNo.color, settings.lineWidth, wxSOLID);
		    topology.boundaryNo++;
		    draw = settings.boundaryNo.enabled;
		}
		else if (left > 0 && right > 0) {
		    pen = new wxPen(settings.boundaryTwo.color, settings.lineWidth, wxSOLID);
		    topology.boundaryTwo++;
		    draw = settings.boundaryTwo.enabled;
		}
		else {
		    pen = new wxPen(settings.boundaryOne.color, settings.lineWidth, wxSOLID);
		    topology.boundaryOne++;
		    draw = settings.boundaryOne.enabled;
		}
		break;
	    default:
		draw = false;
		break;
	    }
	}
	else if (type & GV_POINTS) {
	    if (type == GV_POINT && settings.point.enabled) {
		pen = new wxPen(settings.point.color, settings.lineWidth, wxSOLID);
		topology.point++;
		draw = settings.point.enabled;
	    }
	    else if (type == GV_CENTROID) {
		int cret = Vect_get_centroid_area(mapInfo, line);
		if (cret > 0) { // -> area
		    draw = settings.centroidIn.enabled;
		    pen = new wxPen(settings.centroidIn.color, settings.lineWidth, wxSOLID);
		    topology.centroidIn++;
		}
		else if (cret == 0) {
		    draw = settings.centroidOut.enabled;
		    pen = new wxPen(settings.centroidOut.color, settings.lineWidth, wxSOLID);
		    topology.centroidOut++;
		}
		else {
		    draw = settings.centroidDup.enabled;
		    pen = new wxPen(settings.centroidDup.color, settings.lineWidth, wxSOLID);
		    topology.centroidDup++;
		}
	    }
	}
    }
    
    // clear screen points & convert EN -> xy
    pointsScreen->Clear();
    for (int i = 0; i < points->n_points; i++) {
	Cell2Pixel(points->x[i], points->y[i], points->z[i],
		   &x, &y, &z);
	pointsScreen->Append((wxObject*) new wxPoint((int) x, (int) y)); /* TODO: 3D */
    }
    
    dc->SetId(dcId); /* 0 | 1 (selected) */
    dc->SetPen(*pen);

    if (draw) {
	if (type & GV_POINTS) {
	    DrawCross(line, (const wxPoint *) pointsScreen->GetFirst()->GetData());
	}
	else {
	    // long int startId = ids[line].startId + 1;
	    if (dcId > 0 && drawSegments) {
		dcId = 2; // first segment
		for (size_t i = 0; i < pointsScreen->GetCount() - 1; dcId += 2) {
		    wxPoint *point_beg = (wxPoint *) pointsScreen->Item(i)->GetData();
		    wxPoint *point_end = (wxPoint *) pointsScreen->Item(++i)->GetData();
		    
		    // set bounds for line
		    // wxRect rect (*point_beg, *point_end);
		    // dc->SetIdBounds(startId, rect);
		    
		    dc->SetId(dcId); // set unique id & set bbox for each segment
		    dc->SetPen(*pen);
		    wxRect rect (*point_beg, *point_end);
		    dc->SetIdBounds(dcId, rect);
		    dc->DrawLine(point_beg->x, point_beg->y,
				 point_end->x, point_end->y);
		}
	    }
	    else {
		wxPoint wxPoints[pointsScreen->GetCount()];
		for (size_t i = 0; i < pointsScreen->GetCount(); i++) {
		    wxPoint *point_beg = (wxPoint *) pointsScreen->Item(i)->GetData();
		    wxPoints[i] = *point_beg;
		}
		
		dc->DrawLines(pointsScreen->GetCount(), wxPoints);

		if (!IsSelected(line) && settings.direction.enabled) {
		    DrawDirectionArrow();
		    // restore pen
		    dc->SetPen(*pen);
		}
	    }
	}
    }

    if (type & GV_LINES) {
	DrawLineVerteces(line); // draw vertices
	DrawLineNodes(line);    // draw nodes
    }

    delete pen;

    return 1;
}

/**
   \brief Draw line verteces to the device
 
   Except of first and last vertex, see DrawLineNodes().

   \param line id

   \return number of verteces which were drawn
   \return -1 if drawing vertices is disabled
*/
int DisplayDriver::DrawLineVerteces(int line)
{
    int dcId;
    wxPoint *point;
    wxPen *pen;

    if (!IsSelected(line) && !settings.vertex.enabled)
	return -1;

    // determine color
    if (!IsSelected(line)) {
	pen = new wxPen(settings.vertex.color, settings.lineWidth, wxSOLID);
	dcId = 0;
    }
    else {
	if (!drawSelected) {
	    return -1;
	}
	if (settings.highlightDupl.enabled && IsDuplicated(line)) {
	    pen = new wxPen(settings.highlightDupl.color, settings.lineWidth, wxSOLID);
	}
	else {
	    pen = new wxPen(settings.highlight, settings.lineWidth, wxSOLID);
	}
	if (drawSegments) {
	    dcId = 3; // first vertex
	}
	else {
	    dcId = 1;
	}
    }

    dc->SetId(dcId); /* 0 | 1 (selected) */
    dc->SetPen(*pen);

    for (size_t i = 1; i < pointsScreen->GetCount() - 1; i++, dcId += 2) {
	point = (wxPoint*) pointsScreen->Item(i)->GetData();

	if (IsSelected(line) && drawSegments) {
	    dc->SetId(dcId);
	    dc->SetPen(*pen);
	    wxRect rect (*point, *point);
	    dc->SetIdBounds(dcId, rect);
	}
	
	if (settings.vertex.enabled) {
	    DrawCross(line, (const wxPoint*) pointsScreen->Item(i)->GetData());
	    topology.vertex++;
	}
    }

    delete pen;

    return pointsScreen->GetCount() - 2;
}

/**
   \brief Draw line nodes to the device
 
   \param line id

   \return 1
   \return -1 if no nodes were drawn
*/
int DisplayDriver::DrawLineNodes(int line)
{
    int dcId;
    int node;
    double east, north, depth;
    double x, y, z;
    int nodes [2];
    bool draw;

    wxPen *pen;

    // draw nodes??
    if (!settings.nodeOne.enabled && !settings.nodeTwo.enabled)
	return -1;

    // get nodes
    Vect_get_line_nodes(mapInfo, line, &(nodes[0]), &(nodes[1]));
        
    for (size_t i = 0; i < sizeof(nodes) / sizeof(int); i++) {
	node = nodes[i];
	// get coordinates
	Vect_get_node_coor(mapInfo, node,
			   &east, &north, &depth);

	// convert EN->xy
	Cell2Pixel(east, north, depth,
		   &x, &y, &z);

	// determine color
	if (IsSelected(line)) {
	    if (!drawSelected) {
		return -1;
	    }
	    if (settings.highlightDupl.enabled && IsDuplicated(line)) {
		pen = new wxPen(settings.highlightDupl.color, settings.lineWidth, wxSOLID);
	    }
	    else {
		pen = new wxPen(settings.highlight, settings.lineWidth, wxSOLID);
	    }
	    draw = true;
	    if (!drawSegments) {
		dcId = 1;
	    }
	    else {
		// node1, line1, vertex1, line2, vertex2, ..., node2
		if (i == 0) // first node
		    dcId = 1; 
		else // last node
		    dcId = 2 * points->n_points - 1;
	    }
	}
	else {
	    dcId = 0;
	    if (Vect_get_node_n_lines(mapInfo, node) == 1) {
		pen = new wxPen(settings.nodeOne.color, settings.lineWidth, wxSOLID);
		topology.nodeOne++;
		draw = settings.nodeOne.enabled;
	    }
	    else {
		pen = new wxPen(settings.nodeTwo.color, settings.lineWidth, wxSOLID);
		topology.nodeTwo++;
		draw = settings.nodeTwo.enabled;
	    }
	}
	
	wxPoint point((int) x, (int) y);
	if (IsSelected(line) && drawSegments) {
	    wxRect rect (point, point);
	    dc->SetIdBounds(dcId, rect);
	}

	// draw node if needed
	if (draw) {
	    dc->SetId(dcId);
	    dc->SetPen(*pen);
	    DrawCross(line, &point);
	}
    }
    
    delete pen;

    return 1;
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
	    Vect_build_partial(mapInfo, GV_BUILD_NONE, NULL);
	    Vect_build(mapInfo, NULL);
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

/**
   \brief Draw cross symbol of given size to device content
   
   Used for points, nodes, vertices

   \param[in] point coordinates of center
   \param[in] size size of the cross symbol
   
   \return 1 on success
   \return -1 on failure
*/
int DisplayDriver::DrawCross(int line, const wxPoint* point, int size)
{
    if (!dc || !point)
	return -1;

    dc->DrawLine(point->x - size, point->y, point->x + size, point->y);
    dc->DrawLine(point->x, point->y - size, point->x, point->y + size);
    return 1;
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
				   int lineWidth)
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
			    100); /* transparency */
    
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

    for (int i = 0; i < selected->n_values; i++) {
	std::cerr << selected->value[i] << " ";
    }
    std::cerr << std::endl;

    return;
}

/**
   \brief Select vector objects by given bounding box
   
   If line id is already in the list of selected lines, then it will
   be excluded from this list.


   \param[in] x1,y1,z1,x2,y2,z3 bounding box definition
   \param[in] type feature type

   \return number of selected features
   \return -1 on error
*/
int DisplayDriver::SelectLinesByBox(double x1, double y1, double z1, 
				    double x2, double y2, double z2,
				    int type)
{
    if (!mapInfo)
	return -1;

    int line;

    struct ilist *list;
    struct line_pnts *bbox;

    drawSegments = false;
    drawSelected = true;

    list = Vect_new_list();
    bbox = Vect_new_line_struct();

    Vect_append_point(bbox, x1, y1, z1);
    Vect_append_point(bbox, x2, y1, z2);
    Vect_append_point(bbox, x2, y2, z1);
    Vect_append_point(bbox, x1, y2, z2);
    Vect_append_point(bbox, x1, y1, z1);
        
    Vect_select_lines_by_polygon(mapInfo, bbox,
				 0, NULL, /* isles */
				 type, list);

    for (int i = 0; i < list->n_values; i++) {
	line = list->value[i];
	if (!IsSelected(line)) {
	    Vect_list_append(selected, line);
	}
	else {
	    Vect_list_delete(selected, line);
	}
    }

    Vect_destroy_line_struct(bbox);
    Vect_destroy_list(list);

    return list->n_values;
}

/**
   \brief Select vector feature by given point in given
   threshold
   
   Only one vector object can be selected. Bounding boxes of
   all segments are stores.

   \param[in] x,y point of searching
   \param[in] thresh threshold value where to search
   \param[in] type select vector object of given type

   \return point on line if line found
*/
std::vector<double> DisplayDriver::SelectLineByPoint(double x, double y, double z,
						     double thresh, int type, int with_z)
{
    long int line;
    double px, py, pz;

    std::vector<double> p;

    drawSelected = true;

    line = Vect_find_line(mapInfo, x, y, z,
			  type, thresh, with_z, 0);

    if (line > 0) {
	if (!IsSelected(line)) {
	    Vect_list_append(selected, line);
	}
	else {
	    Vect_list_delete(selected, line);
	}

	type = Vect_read_line (mapInfo, points, cats, line);
	Vect_line_distance (points, x, y, z, with_z,
			    &px, &py, &pz,
			    NULL, NULL, NULL);
	p.push_back(px);
	p.push_back(py);
	if (with_z) {
	    p.push_back(pz);
	}
    }

    drawSegments = true;

    return p;
}

/**
   \brief Is vector object selected?
   
   \param[in] line id

   \return true if vector object is selected
   \return false if vector object is not selected
*/
bool DisplayDriver::IsSelected(int line)
{
    if (Vect_val_in_list(selected, line))
	return true;

    return false;
}

/**
   \brief Get ids of selected objects

   \param[in] grassId if true return GRASS line ids
   if false return PseudoDC ids
   
   \return list of ids of selected vector objects
*/
std::vector<int> DisplayDriver::GetSelected(bool grassId)
{
    if (grassId)
	return ListToVector(selected);

    std::vector<int> dc_ids;

    if (!drawSegments) {
	dc_ids.push_back(1);
    }
    else {
	int npoints;
	Vect_read_line(mapInfo, points, NULL, selected->value[0]);
	npoints = points->n_points;
	for (int i = 1; i < 2 * npoints; i++) {
	  dc_ids.push_back(i);
	}
    }

    return dc_ids;
}

/**
   \brief Get feature (grass) ids of duplicated objects

   \return list of ids
*/
std::map<int, std::vector <int> > DisplayDriver::GetDuplicates()
{
    std::map<int, std::vector<int> > ids;

    struct line_pnts *APoints, *BPoints;
 
    int line;

    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();

    Vect_reset_list(selectedDupl);

    for (int i = 0; i < selected->n_values; i++) {
	line = selected->value[i];
	if (IsDuplicated(line))
	    continue;
	
	Vect_read_line(mapInfo, APoints, NULL, line);
	
	for (int j = 0; j < selected->n_values; j++) {
	    if (i == j || IsDuplicated(selected->value[j]))
		continue;
	    
	    Vect_read_line(mapInfo, BPoints, NULL, selected->value[j]);
	    
	    if (Vect_line_check_duplicate (APoints, BPoints, WITHOUT_Z)) {
		if (ids.find(i) == ids.end()) {
		    ids[i] = std::vector<int> ();
		    ids[i].push_back(selected->value[i]);
		    Vect_list_append(selectedDupl, selected->value[i]);
		}
		ids[i].push_back(selected->value[j]);
		Vect_list_append(selectedDupl, selected->value[j]);
	    }
	}
    }
    
    Vect_destroy_line_struct(APoints);
    Vect_destroy_line_struct(BPoints);
    
    return ids;
}

/**
   \brief Check for already marked duplicates

   \param line line id

   \return 1 line already marked as duplicated
   \return 0 not duplicated
*/
bool DisplayDriver::IsDuplicated(int line)
{
    if (Vect_val_in_list(selectedDupl, line))
	return true;
    
    return false;
}

/**
   \brief Set selected vector objects
   
   \param[in] list of GRASS ids to be set

   \return 1
*/
int DisplayDriver::SetSelected(std::vector<int> id)
{
    drawSelected = true;

    VectorToList(selected, id);

    if (selected->n_values <= 0)
	drawSegments = false;

    return 1;
}

/**
   \brief Unselect selected vector features
   
   \param[in] list of GRASS feature ids

   \return number of selected features
*/
int DisplayDriver::UnSelect(std::vector<int> id)
{
    bool checkForDupl;

    checkForDupl = false;

    for (std::vector<int>::const_iterator i = id.begin(), e = id.end();
	 i != e; ++i) {
	if (IsSelected(*i)) {
	    Vect_list_delete(selected, *i);
	}
	if (settings.highlightDupl.enabled && IsDuplicated(*i)) {
	    checkForDupl = true;
	}
    }

    if (checkForDupl) {
	GetDuplicates();
    }

    return selected->n_values;
}

/**
   \brief Get PseudoDC vertex id of selected line

   Set bounding box for vertices of line.

   \param[in] x,y coordinates of click
   \param[in] thresh threshold value

   \return id of center, left and right vertex

   \return 0 no line found
   \return -1 on error
*/
std::vector<int> DisplayDriver::GetSelectedVertex(double x, double y, double thresh)
{
    int startId;
    int line, type;
    int Gid, DCid;
    double vx, vy, vz;      // vertex screen coordinates

    double dist, minDist;

    std::vector<int> returnId;

    // only one object can be selected
    if (selected->n_values != 1 || !drawSegments) 
	return returnId;

    startId = 1;
    line = selected->value[0];

    type = Vect_read_line (mapInfo, points, cats, line);

    minDist = 0.0;
    Gid = -1;
    // find the closest vertex (x, y)
    DCid = 1;
    for(int idx = 0; idx < points->n_points; idx++) {
	dist = Vect_points_distance(x, y, 0.0,
				    points->x[idx], points->y[idx], points->z[idx], 0);
	
	if (idx == 0) {
	    minDist = dist;
	    Gid  = idx;
	}
	else {
	    if (minDist > dist) {
		minDist = dist;
		Gid = idx;
	    }
	}

	Cell2Pixel(points->x[idx], points->y[idx], points->z[idx],
		   &vx, &vy, &vz);
	wxRect rect (wxPoint ((int) vx, (int) vy), wxPoint ((int) vx, (int) vy));
	dc->SetIdBounds(DCid, rect);
	DCid+=2;
    }	

    if (minDist > thresh)
	return returnId;

    // desc = &(ids[line]);

    // translate id
    DCid = Gid * 2 + 1;

    // add selected vertex
    returnId.push_back(DCid);
    // left vertex
    if (DCid == startId) {
	returnId.push_back(-1);
    }
    else {
	returnId.push_back(DCid - 2);
    }

    // right vertex
    if (DCid == (points->n_points - 1) * 2 + startId) {
	returnId.push_back(-1);
    }
    else {
	returnId.push_back(DCid + 2);
    }

    return returnId;
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

/**
   \brief Draw selected features

   \param draw if true draw selected features
*/
void DisplayDriver::DrawSelected(bool draw)
{
    drawSelected = draw;

    return;
}

/**
   \brief Draw line direction arrow

   \return number of drawn arrows
*/
int DisplayDriver::DrawDirectionArrow()
{
    int narrows;
    int size; // arrow length in pixels
    int limit; // segment length limit for drawing symbol (in pixels)
    double dist, angle, pos;
    double e, n, d, x0, y0, z0, x1, y1, z1;
    struct line_pnts *points_seg;
    wxPen *pen_arrow;
    
    narrows = 0;
    size = 5;
    limit = 5; // 5px for line segment

    points_seg = Vect_new_line_struct();
    pen_arrow = new wxPen(settings.direction.color, settings.lineWidth, wxSOLID);

    dc->SetPen(*pen_arrow);

    dist = Vect_line_length(points);
    
    if (DistanceInPixels(dist) >= limit) {
	while (1) {
	    pos = (narrows + 1) * 8 * limit * region.map_res;

	    if (Vect_point_on_line(points, pos,
				   &e, &n, &d, NULL, NULL) < 1) {
		break;
	    }
	    
	    Cell2Pixel(e, n, d, &x0, &y0, &z0);
	    
	    if (Vect_point_on_line(points, pos - 3 * size * region.map_res,
				   &e, &n, &d, &angle, NULL) < 1) {
		break;
	    }
	    
	    Cell2Pixel(e, n, d, &x1, &y1, &z1);
	    
	    DrawArrow(x0, y0, x1, y1, angle, size);

	    if(narrows > 1e2) // low resolution, break
		break;

	    narrows++;
	}

	// draw at least one arrow in the middle of line
	if (narrows < 1) {
	    dist /= 2.;
	    if (Vect_point_on_line(points, dist,
				   &e, &n, &d, NULL, NULL) > 0) {
	    
		Cell2Pixel(e, n, d, &x0, &y0, &z0);
		
		if (Vect_point_on_line(points, dist - 3 * size * region.map_res,
				       &e, &n, &d, &angle, NULL) > 0) {
		    
		    Cell2Pixel(e, n, d, &x1, &y1, &z1);
		    
		    DrawArrow(x0, y0, x1, y1, angle, size);
		}
	    }
	}
    }

    Vect_destroy_line_struct(points_seg);
    
    return narrows;
}

/**
   \brief Draw arrow symbol on line

   \param x0,y0 arrow origin
   \param x1,x1 arrow ending point (on line)
   \param angle point ending point angle
   \param size arrow size

   \return 1
*/
int DisplayDriver::DrawArrow(double x0, double y0,
			     double x1, double y1, double angle,
			     int size)
{
    double x, y;
    double angle_symb;

    angle_symb = angle - M_PI / 2.;
    x = x1 + size * std::cos(angle_symb);
    y = y1 - size * std::sin(angle_symb);
    dc->DrawLine((wxCoord) x, (wxCoord) y, (wxCoord) x0, (wxCoord) y0);
    
    angle_symb = M_PI / 2. + angle;
    x = x1 + size * std::cos(angle_symb);
    y = y1 - size * std::sin(angle_symb);
    dc->DrawLine((wxCoord) x0, (wxCoord) y0, (wxCoord) x, (wxCoord) y);

    return 1;
}
