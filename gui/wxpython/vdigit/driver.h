#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <iostream> // debug
#include <vector>
#include <map>
#include <cmath>

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include <wx/dc.h>
#include <wx/list.h>

#include <Python.h>
#include <wx/wxPython/pseudodc.h>

extern "C" {
#include <grass/gis.h>
#include <grass/Vect.h>
}

class DisplayDriver
{
private:
    friend class Digit;
    wxPseudoDC *dc;  // device content

    /* disabled due to expensive calling dc->SetId()
     *
     * currently all objects are drawn without id
     *
     * only selected lines with id '1'
     *
     * segments with unique id (starting with '1')
     * are drawn only when line was selected using SelectLineByPoint()
     */
    /*
    struct lineDesc {
	int      npoints;
	long int startId;
    };
    
    typedef std::map<int, lineDesc> ids_map;
    
    ids_map ids; // gId : {dcIds, ...}
    */

    struct ilist *selected;
    struct ilist *selectedDupl;

    bool drawSegments;         // draw segments of selected line

    struct Map_info  *mapInfo;
    struct line_pnts *points;       // east, north, depth
    wxList           *pointsScreen; // x, y, z
    struct line_cats *cats;
    
    struct _region {
	// GRASS region section
	BOUND_BOX box; // W,E,N,S,T,B
	double ns_res;
	double ew_res;
	double center_easting;
	double center_northing;

	// map window section
	double map_width;  // px
	double map_height;
	double map_west;
	double map_north;
	double map_res;
    } region;

    struct symbol {
	bool enabled;
	wxColor color;
    };

    struct _settings {
	wxColor highlight;
	symbol highlightDupl;

	symbol point;
	symbol line;
	
	symbol boundaryNo;
	symbol boundaryOne;
	symbol boundaryTwo;

	symbol centroidIn;
	symbol centroidOut;
	symbol centroidDup;
	
	symbol nodeOne;
	symbol nodeTwo;

	symbol vertex;

	int lineWidth;    // screen units 
    } settings;

    struct _topology {
	long int highlight;

	long int point;
	long int line;

	long int boundaryNo;
	long int boundaryOne;
	long int boundaryTwo;

	long int centroidIn;
	long int centroidOut;
	long int centroidDup;

	long int nodeOne;
	long int nodeTwo;

	long int vertex;
    } topology;

    void Cell2Pixel (double, double, double,
		     double *, double *, double *);
    
    int DrawCross(int, const wxPoint *, int size=5);

    int DrawLine(int);
    int DrawLineVerteces(int);
    int DrawLineNodes(int);

    /* debug */
    void PrintIds();

    /* select feature */
    bool IsSelected(int);
    bool IsDuplicated(int);

    std::vector<int> ListToVector(struct ilist *);
    int VectorToList(struct ilist *, const std::vector<int>&);

    void ResetTopology();

public:
    /* constructor */
    DisplayDriver(void *);
    /* destructor */
    ~DisplayDriver();

    /* display */
    int DrawMap(bool);

    /* select */
    int SelectLinesByBox(double, double, double, double, double, double, int);
    std::vector<double> SelectLineByPoint(double, double, double,
					  double, int, int);

    std::vector<int> GetSelected(bool);
    std::map<int, std::vector <int> > GetDuplicates();
    int SetSelected(std::vector<int>);
    int UnSelect(std::vector<int>);
    std::vector<int> GetSelectedVertex(double, double, double);

    /* general */
    int CloseMap();
    int OpenMap(const char *, const char *, bool);
    void ReloadMap();
    void SetDevice(void *);

    /* misc */
    std::vector<double> GetMapBoundingBox();

    /* set */
    void SetRegion(double, double, double, double,
		   double, double,
		   double, double,
		   double, double);

    void UpdateSettings(unsigned long,
			bool, unsigned long,
			bool, unsigned long, /* enabled, color */
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			bool, unsigned long,
			int);
};

int print_error(const char *, int);

#endif /* __DRIVER_H__ */
