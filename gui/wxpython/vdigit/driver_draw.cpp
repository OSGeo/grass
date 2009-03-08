/**
   \file vdigit/driver_draw.cpp
   
   \brief wxvdigit - Display driver (draw methods)

   This driver is designed for wxGUI (vector digitizer) - to draw
   vector map layer to PseudoDC.

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008-2009 by Martin Landa, and the GRASS development team

   \author Martin Landa <landa.martin gmail.com>
*/

#include <cmath>

#include "driver.h"


/**
   \brief Draw content of the vector map to device
   
   \return number of drawn features
   \return -1 on error
 */
int DisplayDriver::DrawMap(bool force)
{
    if (!mapInfo || !dc || !dcTmp)
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
    dcTmp->BeginDrawing();

    if (settings.area.enabled) {
	/* draw area fills first */
	int area, centroid, isle;
	int num_isles;
	bool draw;
	struct ilist *listAreas, *listCentroids;
	struct line_pnts *points, *ipoints, **isles;

	wxBrush *fillArea, *fillAreaSelected, *fillIsle;

	fillArea = new wxBrush(settings.area.color);
	fillAreaSelected = new wxBrush(settings.highlight);
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
		    int cat;
		    cat = Vect_get_area_cat(mapInfo, area, 1); /* TODO: field */
		    if (cat > -1 && IsSelected(cat, true)) {
			dc->SetBrush(*fillAreaSelected);
		    }
		    else {
			dc->SetBrush(*fillArea);
		    }
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

    dcTmp->EndDrawing();
    dc->EndDrawing();
    
    /* reset list of selected features by cat 
       -> list of ids - see IsSelected()
    */
    selected.field = -1;
    Vect_reset_list(selected.cats);
	
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
    wxPoint *wxPoints = new wxPoint[points->n_points];

    for (int i = 0; i < points->n_points; i++) {
	Cell2Pixel(points->x[i], points->y[i], points->z[i],
		   &x, &y, &z);
	wxPoints[i] = wxPoint((int) x, (int) y);
    }

    // draw polygon
    dc->DrawPolygon(points->n_points, wxPoints);

    delete [] wxPoints;

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
    gwxPseudoDC *pdc;

    pen = NULL;
    draw = false;

    if (!dc || !dcTmp || !Vect_line_alive (mapInfo, line))
	return -1;

    // read line
    type = Vect_read_line (mapInfo, points, cats, line);

    pdc = NULL;

    if (IsSelected(line)) { // line selected ?
	pdc = dcTmp;

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
	pdc = dc;
	
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
    
    pdc->SetId(dcId); /* 0 | 1 (selected) */
   
    if (draw) {
	pdc->SetPen(*pen);
	if (type & GV_POINTS) {
	    DrawCross(pdc, line, (const wxPoint *) pointsScreen->GetFirst()->GetData());
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
		    
		    pdc->SetId(dcId); // set unique id & set bbox for each segment
		    pdc->SetPen(*pen);
		    wxRect rect (*point_beg, *point_end);
		    pdc->SetIdBounds(dcId, rect);
		    pdc->DrawLine(point_beg->x, point_beg->y,
				  point_end->x, point_end->y);
		}
	    }
	    else {
		wxPoint *wxPoints = new wxPoint[pointsScreen->GetCount()];
		for (size_t i = 0; i < pointsScreen->GetCount(); i++) {
		    wxPoint *point_beg = (wxPoint *) pointsScreen->Item(i)->GetData();
		    wxPoints[i] = *point_beg;
		}
		
		pdc->DrawLines(pointsScreen->GetCount(), wxPoints);

		delete [] wxPoints;

		if (!IsSelected(line) && settings.direction.enabled) {
		    DrawDirectionArrow();
		    // restore pen
		    pdc->SetPen(*pen);
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
    gwxPseudoDC *pdc;

    if (!IsSelected(line) && !settings.vertex.enabled)
	return -1;

    pdc = NULL;

    // determine color
    if (!IsSelected(line)) {
	pdc = dc;

	pen = new wxPen(settings.vertex.color, settings.lineWidth, wxSOLID);
	dcId = 0;
    }
    else {
	pdc = dcTmp;
	
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

    pdc->SetId(dcId); /* 0 | 1 (selected) */
    pdc->SetPen(*pen);

    for (size_t i = 1; i < pointsScreen->GetCount() - 1; i++, dcId += 2) {
	point = (wxPoint*) pointsScreen->Item(i)->GetData();

	if (IsSelected(line) && drawSegments) {
	    pdc->SetId(dcId);
	    pdc->SetPen(*pen);
	    wxRect rect (*point, *point);
	    pdc->SetIdBounds(dcId, rect);
	}
	
	if (settings.vertex.enabled) {
	    DrawCross(pdc, line, (const wxPoint*) pointsScreen->Item(i)->GetData());
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
    gwxPseudoDC *pdc;

    pdc = NULL;

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
	    pdc = dcTmp;
	    
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
	    pdc = dc;
	    
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
	    pdc->SetIdBounds(dcId, rect);
	}

	// draw node if needed
	if (draw) {
	    pdc->SetId(dcId);
	    pdc->SetPen(*pen);
	    DrawCross(pdc, line, &point);
	}
    }
    
    delete pen;

    return 1;
}
/**
   \brief Draw cross symbol of given size to device content
   
   Used for points, nodes, vertices

   \param[in,out] PseudoDC where to draw
   \param[in] point coordinates of center
   \param[in] size size of the cross symbol
   
   \return 1 on success
   \return -1 on failure
*/
int DisplayDriver::DrawCross(gwxPseudoDC *pdc, int line, const wxPoint* point, int size)
{
    if (!pdc || !point)
	return -1;

    pdc->DrawLine(point->x - size, point->y, point->x + size, point->y);
    pdc->DrawLine(point->x, point->y - size, point->x, point->y + size);
    
    return 1;
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
    x = x1 + size * cos(angle_symb);
    y = y1 - size * sin(angle_symb);
    dc->DrawLine((wxCoord) x, (wxCoord) y, (wxCoord) x0, (wxCoord) y0);
    
    angle_symb = M_PI / 2. + angle;
    x = x1 + size * cos(angle_symb);
    y = y1 - size * sin(angle_symb);
    dc->DrawLine((wxCoord) x0, (wxCoord) y0, (wxCoord) x, (wxCoord) y);

    return 1;
}

