/////////////////////////////////////////////////////////////////////////////
// Name:        pseudodc.cpp
// Purpose:     Implementation of the gwxPseudoDC Class
// Author:      Paul Lanier
// Modified by: Glynn Clements 2009-01-14
// Created:     05/25/06
// RCS-ID:      $Id: pseudodc.cpp 51090 2008-01-08 04:36:23Z RD $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
//include "wx/wxprec.h"

#undef DEBUG
#include <Python.h>
#include <wx/wxPython/wxPython.h>
#include "pseudodc.h"

// wxList based class definitions
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(gpdcOpList);
WX_DEFINE_LIST(gpdcObjectList);

//----------------------------------------------------------------------------
// Helper functions used for drawing greyed out versions of objects
//----------------------------------------------------------------------------
wxColour &gwxMakeColourGrey(const wxColour &c)
{
    static wxColour rval;
    rval.Set(byte((230-c.Red())*0.7+c.Red()),
             byte((230-c.Green())*0.7+c.Green()),
             byte((230-c.Blue())*0.7+c.Blue()));
    return rval;
}
wxBrush &gwxGetGreyBrush(wxBrush &brush)
{
	static wxBrush b;
	wxColour c;
	b = brush;
	c = gwxMakeColourGrey(brush.GetColour());
	b.SetColour(c);
	return b;
}

wxPen &gwxGetGreyPen(wxPen &pen)
{
	static wxPen p;
	wxColour c;
	p = pen;
	c = gwxMakeColourGrey(pen.GetColour());
	p.SetColour(c);
	return p;
}

void gwxGreyOutImage(wxImage &img)
{
    unsigned char *data = img.GetData();
    unsigned char r,g,b;
    unsigned char mr,mg,mb;
    int i, tst;
    int len = img.GetHeight()*img.GetWidth()*3;
    if (img.HasMask())
    {
        mr = img.GetMaskRed();
        mg = img.GetMaskGreen();
        mb = img.GetMaskBlue();
    }
    tst=0;
    for (i=0;i<len;i+=3)
    {
        r=data[i]; g=data[i+1]; b=data[i+2];
        if (!img.HasMask() || 
            r!=mr || g!=mg || b!=mb)
        {
            if (!tst)
            {
                tst=1;
            }
            r = (unsigned char)((230.0-r)*0.7+r);
            g = (unsigned char)((230.0-g)*0.7+g);
            b = (unsigned char)((230.0-b)*0.7+b);
            data[i]=r; data[i+1]=g; data[i+2]=b;
        }
    }
}

wxIcon &gwxGetGreyIcon(wxIcon &icon)
{
    wxBitmap bmp;
    bmp.CopyFromIcon(icon);
    wxImage img = bmp.ConvertToImage();
    gwxGreyOutImage(img);
    wxBitmap bmp2(img,32);
    static wxIcon rval;
    rval.CopyFromBitmap(bmp2);
    return rval;
}

wxBitmap &gwxGetGreyBitmap(wxBitmap &bmp)
{
    wxImage img = bmp.ConvertToImage();
    gwxGreyOutImage(img);
    static wxBitmap rval(img,32);
    return rval;
}

// ============================================================================
// various gpdcOp class implementation methods
// ============================================================================

// ----------------------------------------------------------------------------
// gpdcDrawPolyPolygonOp constructor
// ----------------------------------------------------------------------------
gpdcDrawPolyPolygonOp::gpdcDrawPolyPolygonOp(int n, int count[], wxPoint points[],
                 wxCoord xoffset, wxCoord yoffset, int fillStyle) 
{
    m_n=n; m_xoffset=xoffset; m_yoffset=yoffset; m_fillStyle=fillStyle;
    int total_n=0;
    if (n)
    {
        m_count = new int[n];
        for(int i=0; i<n; i++) 
        {
            total_n+=count[i];
            m_count[i]=count[i];
        }
        if (total_n)
        {
            m_points = new wxPoint[total_n];
            for(int j=0; j<total_n; j++)
                m_points[j] = points[j];
        }
        else m_points=NULL;
    }
    else
    {
        m_points=NULL;
        m_count=NULL;
    }
    m_totaln = total_n;
}

// ----------------------------------------------------------------------------
// gpdcDrawPolyPolygonOp destructor
// ----------------------------------------------------------------------------
gpdcDrawPolyPolygonOp::~gpdcDrawPolyPolygonOp()
{
    if (m_points) delete m_points;
    if (m_count) delete m_count;
    m_points=NULL;
    m_count=NULL;
}
        
// ----------------------------------------------------------------------------
// gpdcDrawLinesOp constructor
// ----------------------------------------------------------------------------
gpdcDrawLinesOp::gpdcDrawLinesOp(int n, wxPoint points[],
             wxCoord xoffset, wxCoord yoffset)
{
    m_n=n; m_xoffset=xoffset; m_yoffset=yoffset;
    if (n)
    {
        m_points = new wxPoint[n];
        for (int i=0; i<n; i++)
            m_points[i] = points[i];
    }
    else m_points=NULL;
}

// ----------------------------------------------------------------------------
// gpdcDrawLinesOp destructor
// ----------------------------------------------------------------------------
gpdcDrawLinesOp::~gpdcDrawLinesOp()
{
    if (m_points) delete m_points;
    m_points=NULL;
}
        
// ----------------------------------------------------------------------------
// gpdcDrawPolygonOp constructor
// ----------------------------------------------------------------------------
gpdcDrawPolygonOp::gpdcDrawPolygonOp(int n, wxPoint points[],
             wxCoord xoffset, wxCoord yoffset, int fillStyle)
{
    m_n=n; m_xoffset=xoffset; m_yoffset=yoffset; m_fillStyle=fillStyle;
    if (n)
    {
        m_points = new wxPoint[n];
        for (int i=0; i<n; i++)
            m_points[i] = points[i];
    }
    else m_points=NULL;
}

// ----------------------------------------------------------------------------
// gpdcDrawPolygonOp destructor
// ----------------------------------------------------------------------------
gpdcDrawPolygonOp::~gpdcDrawPolygonOp()
{
    if (m_points) delete m_points;
    m_points=NULL;
}

#if wxUSE_SPLINES
// ----------------------------------------------------------------------------
// gpdcDrawSplineOp constructor
// ----------------------------------------------------------------------------
gpdcDrawSplineOp::gpdcDrawSplineOp(int n, wxPoint points[])
{
    m_n=n;
    if (n)
    {
        m_points = new wxPoint[n];
        for(int i=0; i<n; i++)
            m_points[i] = points[i];
    }
    else m_points=NULL;
}

// ----------------------------------------------------------------------------
// gpdcDrawSplineOp destructor
// ----------------------------------------------------------------------------
gpdcDrawSplineOp::~gpdcDrawSplineOp()
{
    if (m_points) delete m_points;
    m_points=NULL;
}
#endif // wxUSE_SPLINES

// ============================================================================
// gpdcObject implementation
// ============================================================================
// ----------------------------------------------------------------------------
// DrawToDC - play back the op list to the DC 
// ----------------------------------------------------------------------------
void gpdcObject::DrawToDC(wxDC *dc)
{
    gpdcOpList::compatibility_iterator node = m_oplist.GetFirst(); 
    while(node)
    {
        node->GetData()->DrawToDC(dc, m_greyedout);
        node = node->GetNext();
    }
}

// ----------------------------------------------------------------------------
// Translate - translate all the operations by some dx,dy
// ----------------------------------------------------------------------------
void gpdcObject::Translate(wxCoord dx, wxCoord dy)
{
    gpdcOpList::compatibility_iterator node = m_oplist.GetFirst(); 
    while(node)
    {
        node->GetData()->Translate(dx,dy);
        node = node->GetNext();
    }
    if (m_bounded) 
    {
        m_bounds.x += dx;
        m_bounds.y += dy;
    }
}

// ----------------------------------------------------------------------------
// SetGreyedOut - set the greyout member and cache grey versions of everything
// if greyout is true
// ----------------------------------------------------------------------------
void gpdcObject::SetGreyedOut(bool greyout) 
{
    m_greyedout=greyout;
    if (greyout)
    {
        gpdcOpList::compatibility_iterator node = m_oplist.GetFirst(); 
        gpdcOp *obj;
        while(node)
        {
            obj = node->GetData();
            obj->CacheGrey();
            node = node->GetNext();
        }
    }
}

// ============================================================================
// gwxPseudoDC implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
gwxPseudoDC::~gwxPseudoDC()
{
    // delete all the nodes in the list
	RemoveAll();
	
}

// ----------------------------------------------------------------------------
// ClearAll - remove all nodes from list
// ----------------------------------------------------------------------------
void gwxPseudoDC::RemoveAll(void)
{
    m_objectlist.Clear();
    m_objectIndex.clear();
    m_currId = -1;
	m_lastObject = NULL;
    
}

// ----------------------------------------------------------------------------
// GetLen - return the number of operations in the current op list
// ----------------------------------------------------------------------------
int gwxPseudoDC::GetLen(void)
{
    gpdcObjectList::compatibility_iterator pt = m_objectlist.GetFirst();
    int len=0;
    while (pt) 
    {
        len += pt->GetData()->GetLen();
        pt = pt->GetNext();
    }
    return len;
}

// ----------------------------------------------------------------------------
// FindObject - find and return an object node by id.  If node doesn't exist
//               and create is true then create one and return it.  Otherwise
//               return NULL.
// ----------------------------------------------------------------------------
gpdcObject *gwxPseudoDC::FindObject(int id, bool create)
{
    // see if last operation was for same id
    //~ if (m_lastObject && m_lastObject->GetId() == id)
        //~ return m_lastObject;
    // if not then search for it    
    gpdcObjectHash::iterator lookup = m_objectIndex.find(id);
    if (lookup == m_objectIndex.end()) {//not found
        if (create) {
            m_lastObject = new gpdcObject(id);
            m_objectlist.Append(m_lastObject);
			gpdcObjectHash::value_type insert(id, m_lastObject);
            m_objectIndex.insert(insert);
            return m_lastObject;
        } else {
            return NULL;
        }
    } else { //found
        return lookup->second;
    }
}

// ----------------------------------------------------------------------------
// AddToList - Add a node to the list at the end (preserve draw order)
// ----------------------------------------------------------------------------
void gwxPseudoDC::AddToList(gpdcOp *newOp)
{
    gpdcObject *obj = FindObject(m_currId, true);
    obj->AddOp(newOp);
}

// ----------------------------------------------------------------------------
// ClearID - remove all the operations associated with a single ID
// ----------------------------------------------------------------------------
void gwxPseudoDC::ClearId(int id)
{
    gpdcObject *obj = FindObject(id);
    if (obj) obj->Clear();
}

// ----------------------------------------------------------------------------
// RemoveID - Remove the object node (and all operations) associated with an id
// ----------------------------------------------------------------------------
void gwxPseudoDC::RemoveId(int id)
{
    gpdcObject *obj = FindObject(id);
    if (obj) 
    {
        if (m_lastObject == obj)
            m_lastObject = obj;
        m_objectlist.DeleteObject(obj);
    }
    m_objectIndex.erase(id);
}

// ----------------------------------------------------------------------------
// SetIdBounds - Set the bounding rect for a given id
// ----------------------------------------------------------------------------
void gwxPseudoDC::SetIdBounds(int id, wxRect& rect)
{
    gpdcObject *obj = FindObject(id, true);
    obj->SetBounds(rect);
}

// ----------------------------------------------------------------------------
// GetIdBounds - Get the bounding rect for a given id
// ----------------------------------------------------------------------------
void gwxPseudoDC::GetIdBounds(int id, wxRect& rect)
{
    gpdcObject *obj = FindObject(id);
	if (obj && obj->IsBounded())
		rect = obj->GetBounds();
	else
		rect.x = rect.y = rect.width = rect.height = 0;
}

// ----------------------------------------------------------------------------
// TranslateId - Translate all the operations of a single id
// ----------------------------------------------------------------------------
void gwxPseudoDC::TranslateId(int id, wxCoord dx, wxCoord dy)
{
    gpdcObject *obj = FindObject(id);
    if (obj) obj->Translate(dx,dy);
}

// ----------------------------------------------------------------------------
// DrawIdToDC - Draw a specific id to the dc passed in
// ----------------------------------------------------------------------------
void gwxPseudoDC::DrawIdToDC(int id, wxDC *dc)
{
    gpdcObject *obj = FindObject(id);
    if (obj) obj->DrawToDC(dc);
}

// ----------------------------------------------------------------------------
// SetIdGreyedOut - Set the greyedout member of id
// ----------------------------------------------------------------------------
void gwxPseudoDC::SetIdGreyedOut(int id, bool greyout)
{
    gpdcObject *obj = FindObject(id);
    if (obj) obj->SetGreyedOut(greyout);
}

// ----------------------------------------------------------------------------
// GetIdGreyedOut - Get the greyedout member of id
// ----------------------------------------------------------------------------
bool gwxPseudoDC::GetIdGreyedOut(int id)
{
    gpdcObject *obj = FindObject(id);
    if (obj) return obj->GetGreyedOut();
	else return false;
}

// ----------------------------------------------------------------------------
// FindObjectsByBBox - Return a list of all the ids whose bounding boxes
//                     contain (x,y)
// ----------------------------------------------------------------------------
PyObject *gwxPseudoDC::FindObjectsByBBox(wxCoord x, wxCoord y)
{
    //wxPyBlock_t blocked = wxPyBeginBlockThreads();
    gpdcObjectList::compatibility_iterator pt = m_objectlist.GetFirst();
    gpdcObject *obj;
    PyObject* pyList = NULL;
    pyList = PyList_New(0);
    wxRect r;
    while (pt) 
    {
        obj = pt->GetData();
        r = obj->GetBounds();
        if (obj->IsBounded() && r.Contains(x,y))
        {
            PyObject* pyObj = PyInt_FromLong((long)obj->GetId());
            PyList_Insert(pyList, 0, pyObj);
            Py_DECREF(pyObj);
        }
        pt = pt->GetNext();
    }
    //wxPyEndBlockThreads(blocked);
    return pyList;
}

// ----------------------------------------------------------------------------
// FindObjects - Return a list of all the ids that draw to (x,y)
// ----------------------------------------------------------------------------
PyObject *gwxPseudoDC::FindObjects(wxCoord x, wxCoord y, 
                                  wxCoord radius, const wxColor& bg)
{
    //wxPyBlock_t blocked = wxPyBeginBlockThreads();
    gpdcObjectList::compatibility_iterator pt = m_objectlist.GetFirst();
    gpdcObject *obj;
    PyObject* pyList = NULL;
    pyList = PyList_New(0);
    wxBrush bgbrush(bg);
    wxPen bgpen(bg);
    // special case radius = 0
    if (radius == 0)
    {
        wxBitmap bmp(4,4,24);
        wxMemoryDC memdc;
        wxColor pix;
        wxRect viewrect(x-2,y-2,4,4);
        // setup the memdc for rendering
        memdc.SelectObject(bmp);
        memdc.SetBackground(bgbrush);
        memdc.Clear();
        memdc.SetDeviceOrigin(2-x,2-y);
        while (pt) 
        {
            obj = pt->GetData();
            if (obj->IsBounded() && obj->GetBounds().Contains(x,y))
            {
                // start clean
                memdc.SetBrush(bgbrush);
                memdc.SetPen(bgpen);
                memdc.DrawRectangle(viewrect);
                // draw the object
                obj->DrawToDC(&memdc);
                memdc.GetPixel(x,y,&pix);
                // clear and update rgn2
                if (pix != bg)
                {
                    PyObject* pyObj = PyInt_FromLong((long)obj->GetId());
                    PyList_Insert(pyList, 0, pyObj);
                    Py_DECREF(pyObj);
                }
            }
            pt = pt->GetNext();
        }
        memdc.SelectObject(wxNullBitmap);
    }
    else
    {
        wxRect viewrect(x-radius,y-radius,2*radius,2*radius);
        wxBitmap maskbmp(2*radius,2*radius,24);
        wxMemoryDC maskdc;
        // create bitmap with circle for masking
        maskdc.SelectObject(maskbmp);
        maskdc.SetBackground(*wxBLACK_BRUSH);
        maskdc.Clear();
        maskdc.SetBrush(*wxWHITE_BRUSH);
        maskdc.SetPen(*wxWHITE_PEN);
        maskdc.DrawCircle(radius,radius,radius);
        // now setup a memdc for rendering our object
        wxBitmap bmp(2*radius,2*radius,24);
        wxMemoryDC memdc;
        memdc.SelectObject(bmp);
        // set the origin so (x,y) is in the bmp center
        memdc.SetDeviceOrigin(radius-x,radius-y);
        // a region will be used to see if the result is empty
        wxRegion rgn2;
        while (pt) 
        {
            obj = pt->GetData();
            if (obj->IsBounded() && viewrect.Intersects(obj->GetBounds()))
            {
                // start clean
                //memdc.Clear();
                memdc.SetBrush(bgbrush);
                memdc.SetPen(bgpen);
                memdc.DrawRectangle(viewrect);
                // draw the object
                obj->DrawToDC(&memdc);
                // remove background color
                memdc.SetLogicalFunction(wxXOR);
                memdc.SetBrush(bgbrush);
                memdc.SetPen(bgpen);
                memdc.DrawRectangle(viewrect);
                memdc.SetLogicalFunction(wxCOPY);
#ifdef __WXMAC__
                // wxAND is not supported on wxMac, but it doesn't seem to
                // hurt anything to use wxCOPY instead...
                memdc.Blit(x-radius,y-radius,2*radius,2*radius,&maskdc,0,0,wxCOPY);
#else
                // AND with circle bitmap
                memdc.Blit(x-radius,y-radius,2*radius,2*radius,&maskdc,0,0,wxAND);
#endif
                // clear and update rgn2
                memdc.SelectObject(wxNullBitmap);
                rgn2.Clear();
                rgn2.Union(bmp, *wxBLACK);
                //rgn2.Intersect(rgn);
                memdc.SelectObject(bmp);
                if (!rgn2.IsEmpty())
                {
                    PyObject* pyObj = PyInt_FromLong((long)obj->GetId());
                    PyList_Insert(pyList, 0, pyObj);
                    Py_DECREF(pyObj);
                }
            }
            pt = pt->GetNext();
        }
        maskdc.SelectObject(wxNullBitmap);
        memdc.SelectObject(wxNullBitmap);
    }
    //wxPyEndBlockThreads(blocked);
    return pyList;
}

// ----------------------------------------------------------------------------
// DrawToDCClipped - play back the op list to the DC but clip any objects
//                   known to be not in rect.  This is a coarse level of 
//                   clipping to speed things up when lots of objects are off 
//                   screen and doesn't affect the dc level clipping
// ----------------------------------------------------------------------------
void gwxPseudoDC::DrawToDCClipped(wxDC *dc, const wxRect& rect)
{
    gpdcObjectList::compatibility_iterator pt = m_objectlist.GetFirst();
    gpdcObject *obj;
    while (pt) 
    {
        obj = pt->GetData();
        if (!obj->IsBounded() || rect.Intersects(obj->GetBounds()))
            obj->DrawToDC(dc);
        pt = pt->GetNext();
    }
}
void gwxPseudoDC::DrawToDCClippedRgn(wxDC *dc, const wxRegion& region)
{
    gpdcObjectList::compatibility_iterator pt = m_objectlist.GetFirst();
    gpdcObject *obj;
    while (pt) 
    {
        obj = pt->GetData();
        if (!obj->IsBounded() || 
		    (region.Contains(obj->GetBounds()) != wxOutRegion))
            obj->DrawToDC(dc);
        pt = pt->GetNext();
    }
}

// ----------------------------------------------------------------------------
// DrawToDC - play back the op list to the DC 
// ----------------------------------------------------------------------------
void gwxPseudoDC::DrawToDC(wxDC *dc)
{
    gpdcObjectList::compatibility_iterator pt = m_objectlist.GetFirst();
    while (pt) 
    {
        pt->GetData()->DrawToDC(dc);
        pt = pt->GetNext();
    }
}
        
