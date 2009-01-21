/////////////////////////////////////////////////////////////////////////////
// Name:        pseudodc.h
// Purpose:     gwxPseudoDC class
// Author:      Paul Lanier
// Modified by: Glynn Clements 2009-01-14
// Created:     05/25/06
// RCS-ID:      $Id: pseudodc.h 49047 2007-10-05 18:08:39Z RD $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#ifndef _GWX_PSUEDO_DC_H_BASE_
#define _GWX_PSUEDO_DC_H_BASE_

//----------------------------------------------------------------------------
// Base class for all gpdcOp classes
//----------------------------------------------------------------------------
class gpdcOp
{
    public:
        // Constructor and Destructor
        gpdcOp() {}
        virtual ~gpdcOp() {}

        // Virtual Drawing Methods
        virtual void DrawToDC(wxDC *dc, bool grey=false)=0;
        virtual void Translate(wxCoord dx, wxCoord dy) {}
        virtual void CacheGrey() {}
};

//----------------------------------------------------------------------------
// declare a list class for list of gpdcOps
//----------------------------------------------------------------------------
WX_DECLARE_LIST(gpdcOp, gpdcOpList);


//----------------------------------------------------------------------------
// Helper functions used for drawing greyed out versions of objects
//----------------------------------------------------------------------------
wxColour &gwxMakeColourGrey(const wxColour &c);
wxBrush &gwxGetGreyBrush(wxBrush &brush);
wxPen &gwxGetGreyPen(wxPen &pen);
wxIcon &gwxGetGreyIcon(wxIcon &icon);
wxBitmap &gwxGetGreyBitmap(wxBitmap &bmp);
void gwxGreyOutImage(wxImage &img);

//----------------------------------------------------------------------------
// Classes derived from gpdcOp
// There is one class for each method mirrored from wxDC to gwxPseudoDC
//----------------------------------------------------------------------------
class gpdcSetFontOp : public gpdcOp
{
    public:
        gpdcSetFontOp(const wxFont& font) 
            {m_font=font;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->SetFont(m_font);}
    protected:
        wxFont m_font;
};

class gpdcSetBrushOp : public gpdcOp
{
    public:
        gpdcSetBrushOp(const wxBrush& brush) 
            {m_greybrush=m_brush=brush;} 
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
        {
            if (!grey) dc->SetBrush(m_brush);
            else dc->SetBrush(m_greybrush);
        }
        virtual void CacheGrey() {m_greybrush=gwxGetGreyBrush(m_brush);}
    protected:
        wxBrush m_brush;
        wxBrush m_greybrush;
};

class gpdcSetBackgroundOp : public gpdcOp
{
    public:
        gpdcSetBackgroundOp(const wxBrush& brush) 
            {m_greybrush=m_brush=brush;} 
        virtual void DrawToDC(wxDC *dc, bool grey=false)
        {
            if (!grey) dc->SetBackground(m_brush);
            else dc->SetBackground(m_greybrush);
        }
        virtual void CacheGrey() {m_greybrush=gwxGetGreyBrush(m_brush);}
    protected:
        wxBrush m_brush;
        wxBrush m_greybrush;
};

class gpdcSetPenOp : public gpdcOp
{
    public:
        gpdcSetPenOp(const wxPen& pen) 
            {m_greypen=m_pen=pen;}
        virtual void DrawToDC(wxDC *dc, bool grey=false)
        {
            if (!grey) dc->SetPen(m_pen);
            else dc->SetPen(m_greypen);
        }
        virtual void CacheGrey() {m_greypen=gwxGetGreyPen(m_pen);}
    protected:
        wxPen m_pen;
        wxPen m_greypen;
};

class gpdcSetTextBackgroundOp : public gpdcOp
{
    public:
        gpdcSetTextBackgroundOp(const wxColour& colour) 
            {m_colour=colour;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
        {
            if (!grey) dc->SetTextBackground(m_colour);
            else dc->SetTextBackground(gwxMakeColourGrey(m_colour));
        }
    protected:
        wxColour m_colour;
};

class gpdcSetTextForegroundOp : public gpdcOp
{
    public:
        gpdcSetTextForegroundOp(const wxColour& colour) 
            {m_colour=colour;}
        virtual void DrawToDC(wxDC *dc, bool grey=false)
        {
            if (!grey) dc->SetTextForeground(m_colour);
            else dc->SetTextForeground(gwxMakeColourGrey(m_colour));
        }
    protected:
        wxColour m_colour;
};

class gpdcDrawRectangleOp : public gpdcOp
{
    public:
        gpdcDrawRectangleOp(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
            {m_x=x; m_y=y; m_w=w; m_h=h;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->DrawRectangle(m_x,m_y,m_w,m_h);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx;m_y+=dy;}
    protected:
        wxCoord m_x,m_y,m_w,m_h;
};

class gpdcDrawLineOp : public gpdcOp
{
    public:
        gpdcDrawLineOp(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2)
            {m_x1=x1; m_y1=y1; m_x2=x2; m_y2=y2;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->DrawLine(m_x1,m_y1,m_x2,m_y2);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x1+=dx; m_y1+=dy; m_x2+=dx; m_y2+=dy;}
    protected:
        wxCoord m_x1,m_y1,m_x2,m_y2;
};

class gpdcSetBackgroundModeOp : public gpdcOp
{
    public:
        gpdcSetBackgroundModeOp(int mode) {m_mode=mode;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->SetBackgroundMode(m_mode);}
    protected:
        int m_mode;
};

class gpdcDrawTextOp : public gpdcOp
{
    public:
        gpdcDrawTextOp(const wxString& text, wxCoord x, wxCoord y)
            {m_text=text; m_x=x; m_y=y;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->DrawText(m_text, m_x, m_y);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxString m_text;
        wxCoord m_x, m_y;
};

class gpdcClearOp : public gpdcOp
{
    public:
        gpdcClearOp() {}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->Clear();}
};

class gpdcBeginDrawingOp : public gpdcOp
{
    public:
        gpdcBeginDrawingOp() {}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {}
};

class gpdcEndDrawingOp : public gpdcOp
{
    public:
        gpdcEndDrawingOp() {}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {}
};

class gpdcFloodFillOp : public gpdcOp
{
    public:
        gpdcFloodFillOp(wxCoord x, wxCoord y, const wxColour& col,
                   int style) {m_x=x; m_y=y; m_col=col; m_style=style;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
        {
            if (!grey) dc->FloodFill(m_x,m_y,m_col,m_style);
            else dc->FloodFill(m_x,m_y,gwxMakeColourGrey(m_col),m_style);
        }
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxCoord m_x,m_y;
        wxColour m_col;
        int m_style;
};

class gpdcCrossHairOp : public gpdcOp
{
    public:
        gpdcCrossHairOp(wxCoord x, wxCoord y) {m_x=x; m_y=y;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->CrossHair(m_x,m_y);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxCoord m_x,m_y;
};

class gpdcDrawArcOp : public gpdcOp
{
    public:
        gpdcDrawArcOp(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2,
                         wxCoord xc, wxCoord yc) 
            {m_x1=x1; m_y1=y1; m_x2=x2; m_y2=y2; m_xc=xc; m_yc=yc;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawArc(m_x1,m_y1,m_x2,m_y2,m_xc,m_yc);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x1+=dx; m_x2+=dx; m_y1+=dy; m_y2+=dy;}
    protected:
        wxCoord m_x1,m_x2,m_xc;
        wxCoord m_y1,m_y2,m_yc;
};

class gpdcDrawCheckMarkOp : public gpdcOp
{
    public:
        gpdcDrawCheckMarkOp(wxCoord x, wxCoord y,
                       wxCoord width, wxCoord height) 
            {m_x=x; m_y=y; m_w=width; m_h=height;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawCheckMark(m_x,m_y,m_w,m_h);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxCoord m_x,m_y,m_w,m_h;
};

class gpdcDrawEllipticArcOp : public gpdcOp
{
    public:
        gpdcDrawEllipticArcOp(wxCoord x, wxCoord y, wxCoord w, wxCoord h,
                         double sa, double ea) 
            {m_x=x; m_y=y; m_w=w; m_h=h; m_sa=sa; m_ea=ea;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawEllipticArc(m_x,m_y,m_w,m_h,m_sa,m_ea);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxCoord m_x,m_y,m_w,m_h;
        double m_sa,m_ea;
};

class gpdcDrawPointOp : public gpdcOp
{
    public:
        gpdcDrawPointOp(wxCoord x, wxCoord y) 
            {m_x=x; m_y=y;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->DrawPoint(m_x,m_y);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxCoord m_x,m_y;
};

class gpdcDrawRoundedRectangleOp : public gpdcOp
{
    public:
        gpdcDrawRoundedRectangleOp(wxCoord x, wxCoord y, wxCoord width, 
                                  wxCoord height, double radius) 
            {m_x=x; m_y=y; m_w=width; m_h=height; m_r=radius;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawRoundedRectangle(m_x,m_y,m_w,m_h,m_r);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxCoord m_x,m_y,m_w,m_h;
        double m_r;
};

class gpdcDrawEllipseOp : public gpdcOp
{
    public:
        gpdcDrawEllipseOp(wxCoord x, wxCoord y, wxCoord width, wxCoord height) 
            {m_x=x; m_y=y; m_w=width; m_h=height;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->DrawEllipse(m_x,m_y,m_w,m_h);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxCoord m_x,m_y,m_w,m_h;
};

class gpdcDrawIconOp : public gpdcOp
{
    public:
        gpdcDrawIconOp(const wxIcon& icon, wxCoord x, wxCoord y) 
            {m_icon=icon; m_x=x; m_y=y;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
        {
            if (grey) dc->DrawIcon(m_greyicon,m_x,m_y);
            else dc->DrawIcon(m_icon,m_x,m_y);
        }
        virtual void CacheGrey() {m_greyicon=gwxGetGreyIcon(m_icon);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxIcon m_icon;
        wxIcon m_greyicon;
        wxCoord m_x,m_y;
};

class gpdcDrawLinesOp : public gpdcOp
{
    public:
        gpdcDrawLinesOp(int n, wxPoint points[],
               wxCoord xoffset = 0, wxCoord yoffset = 0);
        virtual ~gpdcDrawLinesOp();
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawLines(m_n,m_points,m_xoffset,m_yoffset);}
        virtual void Translate(wxCoord dx, wxCoord dy)
        { 
            for(int i=0; i<m_n; i++)
            {
                m_points[i].x+=dx; 
                m_points[i].y+=dy;
            }
        }
    protected:
        int m_n;
        wxPoint *m_points;
        wxCoord m_xoffset,m_yoffset;
};

class gpdcDrawPolygonOp : public gpdcOp
{
    public:
        gpdcDrawPolygonOp(int n, wxPoint points[],
                     wxCoord xoffset = 0, wxCoord yoffset = 0,
                     int fillStyle = wxODDEVEN_RULE);
        virtual ~gpdcDrawPolygonOp();
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawPolygon(m_n,m_points,m_xoffset,m_yoffset,m_fillStyle);}
        
        virtual void Translate(wxCoord dx, wxCoord dy)
        { 
            for(int i=0; i<m_n; i++)
            {
                m_points[i].x+=dx; 
                m_points[i].y+=dy;
            }
        }
    protected:
        int m_n;
        wxPoint *m_points;
        wxCoord m_xoffset,m_yoffset;
        int m_fillStyle;
};

class gpdcDrawPolyPolygonOp : public gpdcOp
{
    public:
        gpdcDrawPolyPolygonOp(int n, int count[], wxPoint points[],
                         wxCoord xoffset = 0, wxCoord yoffset = 0,
                         int fillStyle = wxODDEVEN_RULE);
        virtual ~gpdcDrawPolyPolygonOp();
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawPolyPolygon(m_n,m_count,m_points,
                    m_xoffset,m_yoffset,m_fillStyle);}
        virtual void Translate(wxCoord dx, wxCoord dy)
        { 
            for(int i=0; i<m_totaln; i++)
            {
                m_points[i].x += dx; 
                m_points[i].y += dy;
            }
        }
    protected:
        int m_n;
        int m_totaln;
        int *m_count;
        wxPoint *m_points;
        wxCoord m_xoffset, m_yoffset;
        int m_fillStyle;
};

class gpdcDrawRotatedTextOp : public gpdcOp
{
    public:
        gpdcDrawRotatedTextOp(const wxString& text, wxCoord x, wxCoord y, double angle) 
            {m_text=text; m_x=x; m_y=y; m_angle=angle;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawRotatedText(m_text,m_x,m_y,m_angle);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxString m_text;
        wxCoord m_x,m_y;
        double m_angle;
};

class gpdcDrawBitmapOp : public gpdcOp
{
    public:
        gpdcDrawBitmapOp(const wxBitmap &bmp, wxCoord x, wxCoord y,
                        bool useMask = false) 
            {m_bmp=bmp; m_x=x; m_y=y; m_useMask=useMask;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
        {
            if (grey) dc->DrawBitmap(m_greybmp,m_x,m_y,m_useMask);
            else dc->DrawBitmap(m_bmp,m_x,m_y,m_useMask);
        }
        virtual void CacheGrey() {m_greybmp=gwxGetGreyBitmap(m_bmp);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_x+=dx; m_y+=dy;}
    protected:
        wxBitmap m_bmp;
        wxBitmap m_greybmp;
        wxCoord m_x,m_y;
        bool m_useMask;
};

class gpdcDrawLabelOp : public gpdcOp
{
    public:
        gpdcDrawLabelOp(const wxString& text,
                           const wxBitmap& image,
                           const wxRect& rect,
                           int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                           int indexAccel = -1)
            {m_text=text; m_image=image; m_rect=rect; 
             m_align=alignment; m_iAccel=indexAccel;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) 
            {dc->DrawLabel(m_text,m_image,m_rect,m_align,m_iAccel);}
        virtual void Translate(wxCoord dx, wxCoord dy) 
            {m_rect.x+=dx; m_rect.y+=dy;}
    protected:
        wxString m_text;
        wxBitmap m_image;
        wxRect m_rect;
        int m_align;
        int m_iAccel;
};

#if wxUSE_SPLINES
class gpdcDrawSplineOp : public gpdcOp
{
    public:
        gpdcDrawSplineOp(int n, wxPoint points[]);
        virtual ~gpdcDrawSplineOp();
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->DrawSpline(m_n,m_points);}
        virtual void Translate(wxCoord dx, wxCoord dy)
        {
            int i;
            for(i=0; i<m_n; i++)
                m_points[i].x+=dx; m_points[i].y+=dy;
        }
    protected:
        wxPoint *m_points;
        int m_n;
};
#endif // wxUSE_SPLINES

#if wxUSE_PALETTE
class gpdcSetPaletteOp : public gpdcOp
{
    public:
        gpdcSetPaletteOp(const wxPalette& palette) {m_palette=palette;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->SetPalette(m_palette);}
    protected:
        wxPalette m_palette;
};
#endif // wxUSE_PALETTE

class gpdcSetLogicalFunctionOp : public gpdcOp
{
    public:
        gpdcSetLogicalFunctionOp(int function) {m_function=function;}
        virtual void DrawToDC(wxDC *dc, bool grey=false) {dc->SetLogicalFunction(m_function);}
    protected:
        int m_function;
};

//----------------------------------------------------------------------------
// gpdcObject type to contain list of operations for each real (Python) object
//----------------------------------------------------------------------------
class gpdcObject
{
    public:
        gpdcObject(int id) 
            {m_id=id; m_bounded=false; m_oplist.DeleteContents(true);
             m_greyedout=false;}

        virtual ~gpdcObject() {m_oplist.Clear();}
        
        // Protected Member Access
        void SetId(int id) {m_id=id;}
        int  GetId() {return m_id;}
        void   SetBounds(wxRect& rect) {m_bounds=rect; m_bounded=true;}
        wxRect GetBounds() {return m_bounds;}
        void SetBounded(bool bounded) {m_bounded=bounded;}
        bool IsBounded() {return m_bounded;}
        void SetGreyedOut(bool greyout=true);
        bool GetGreyedOut() {return m_greyedout;}
    
        // Op List Management Methods
        void Clear() {m_oplist.Clear();}
        void AddOp(gpdcOp *op) 
        {
            m_oplist.Append(op);
            if (m_greyedout) op->CacheGrey();
        }
        int  GetLen() {return m_oplist.GetCount();}
        virtual void Translate(wxCoord dx, wxCoord dy);
        
        // Drawing Method
        virtual void DrawToDC(wxDC *dc);
    protected:
        int m_id; // id of object (associates this gpdcObject
                  //               with a Python object with same id)
        wxRect m_bounds;  // bounding rect of this object
        bool m_bounded;   // true if bounds is valid, false by default
        gpdcOpList m_oplist; // list of operations for this object
        bool m_greyedout; // if true then draw this object in greys only
};


//----------------------------------------------------------------------------
// Declare a wxList to hold all the objects.  List order reflects drawing
// order (Z order) and is the same order as objects are added to the list
//----------------------------------------------------------------------------
class gpdcObjectList;
WX_DECLARE_LIST(gpdcObject, gpdcObjectList);

//Declare a hashmap that maps from ids to nodes in the object list.
WX_DECLARE_HASH_MAP(
    int,
    gpdcObject *,
    wxIntegerHash,
    wxIntegerEqual,
    gpdcObjectHash
);


// ----------------------------------------------------------------------------
// gwxPseudoDC class
// ----------------------------------------------------------------------------
// This is the actual PseudoDC class
// This class stores a list of recorded dc operations in m_list
// and plays them back to a real dc using DrawToDC or DrawToDCClipped.
// Drawing methods are mirrored from wxDC but add nodes to m_list 
// instead of doing any real drawing.
// ----------------------------------------------------------------------------
class gwxPseudoDC : public wxObject
{
public:
    gwxPseudoDC() 
        {m_currId=-1; m_lastObject=NULL; m_objectlist.DeleteContents(true);m_objectIndex.clear();}
    ~gwxPseudoDC();
    // ------------------------------------------------------------------------
    // List managment methods
    // 
    void RemoveAll();
    int GetLen();
    
    // ------------------------------------------------------------------------
    // methods for managing operations by ID
    // 
    // Set the Id for all subsequent operations (until SetId is called again)
    void SetId(int id) {m_currId = id;}
    // Remove all the operations associated with an id so it can be redrawn
    void ClearId(int id);
    // Remove the object node (and all operations) associated with an id
    void RemoveId(int id);
    // Set the bounding rect of a given object
    // This will create an object node if one doesn't exist
    void SetIdBounds(int id, wxRect& rect);
    void GetIdBounds(int id, wxRect& rect);
    // Translate all the operations for this id
    void TranslateId(int id, wxCoord dx, wxCoord dy);
    // Grey-out an object
    void SetIdGreyedOut(int id, bool greyout=true);
    bool GetIdGreyedOut(int id);
    // Find Objects at a point.  Returns Python list of id's
    // sorted in reverse drawing order (result[0] is top object)
    // This version looks at drawn pixels
    PyObject *FindObjects(wxCoord x, wxCoord y, 
                          wxCoord radius=1, const wxColor& bg=*wxWHITE);
    // This version only looks at bounding boxes
    PyObject *FindObjectsByBBox(wxCoord x, wxCoord y);

    // ------------------------------------------------------------------------
    // Playback Methods
    //
    // draw to dc but skip objects known to be outside of rect
    // This is a coarse level of clipping to speed things up 
    // when lots of objects are off screen and doesn't affect the dc level 
    // clipping
    void DrawToDCClipped(wxDC *dc, const wxRect& rect);
        void DrawToDCClippedRgn(wxDC *dc, const wxRegion& region);
    // draw to dc with no clipping (well the dc will still clip)
    void DrawToDC(wxDC *dc);
    // draw a single object to the dc
    void DrawIdToDC(int id, wxDC *dc);

    // ------------------------------------------------------------------------
    // Hit Detection Methods
    //
    // returns list of object with a drawn pixel within radius pixels of (x,y)
    // the list is in reverse draw order so last drawn is first in list
    // PyObject *HitTest(wxCoord x, wxCoord y, double radius)
    // returns list of objects whose bounding boxes include (x,y)
    // PyObject *HitTestBB(wxCoord x, wxCoord y)
    
        
    // ------------------------------------------------------------------------
    // Methods mirrored from wxDC
    //
    void FloodFill(wxCoord x, wxCoord y, const wxColour& col,
                   int style = wxFLOOD_SURFACE)
        {AddToList(new gpdcFloodFillOp(x,y,col,style));}
    void FloodFill(const wxPoint& pt, const wxColour& col,
                   int style = wxFLOOD_SURFACE)
        { FloodFill(pt.x, pt.y, col, style); }

    void DrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2)
        {AddToList(new gpdcDrawLineOp(x1, y1, x2, y2));}
    void DrawLine(const wxPoint& pt1, const wxPoint& pt2)
        { DrawLine(pt1.x, pt1.y, pt2.x, pt2.y); }

    void CrossHair(wxCoord x, wxCoord y)
        {AddToList(new gpdcCrossHairOp(x,y));}
    void CrossHair(const wxPoint& pt)
        { CrossHair(pt.x, pt.y); }

    void DrawArc(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2,
                 wxCoord xc, wxCoord yc)
        {AddToList(new gpdcDrawArcOp(x1,y1,x2,y2,xc,yc));}
    void DrawArc(const wxPoint& pt1, const wxPoint& pt2, const wxPoint& centre)
        { DrawArc(pt1.x, pt1.y, pt2.x, pt2.y, centre.x, centre.y); }

    void DrawCheckMark(wxCoord x, wxCoord y,
                       wxCoord width, wxCoord height)
        {AddToList(new gpdcDrawCheckMarkOp(x,y,width,height));}
    void DrawCheckMark(const wxRect& rect)
        { DrawCheckMark(rect.x, rect.y, rect.width, rect.height); }

    void DrawEllipticArc(wxCoord x, wxCoord y, wxCoord w, wxCoord h,
                         double sa, double ea)
        {AddToList(new gpdcDrawEllipticArcOp(x,y,w,h,sa,ea));}
    void DrawEllipticArc(const wxPoint& pt, const wxSize& sz,
                         double sa, double ea)
        { DrawEllipticArc(pt.x, pt.y, sz.x, sz.y, sa, ea); }

    void DrawPoint(wxCoord x, wxCoord y)
        {AddToList(new gpdcDrawPointOp(x,y));}
    void DrawPoint(const wxPoint& pt)
        { DrawPoint(pt.x, pt.y); }

    void DrawPolygon(int n, wxPoint points[],
                     wxCoord xoffset = 0, wxCoord yoffset = 0,
                     int fillStyle = wxODDEVEN_RULE)
        {AddToList(new gpdcDrawPolygonOp(n,points,xoffset,yoffset,fillStyle));}

    void DrawPolyPolygon(int n, int count[], wxPoint points[],
                         wxCoord xoffset = 0, wxCoord yoffset = 0,
                         int fillStyle = wxODDEVEN_RULE)
        {AddToList(new gpdcDrawPolyPolygonOp(n,count,points,xoffset,yoffset,fillStyle));}

    void DrawRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
        {AddToList(new gpdcDrawRectangleOp(x, y, width, height));}
    void DrawRectangle(const wxPoint& pt, const wxSize& sz)
        { DrawRectangle(pt.x, pt.y, sz.x, sz.y); }
    void DrawRectangle(const wxRect& rect)
        { DrawRectangle(rect.x, rect.y, rect.width, rect.height); }

    void DrawRoundedRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height,
                              double radius)
        {AddToList(new gpdcDrawRoundedRectangleOp(x,y,width,height,radius));}
    void DrawRoundedRectangle(const wxPoint& pt, const wxSize& sz,
                             double radius)
        { DrawRoundedRectangle(pt.x, pt.y, sz.x, sz.y, radius); }
    void DrawRoundedRectangle(const wxRect& r, double radius)
        { DrawRoundedRectangle(r.x, r.y, r.width, r.height, radius); }

    void DrawCircle(wxCoord x, wxCoord y, wxCoord radius)
        { DrawEllipse(x - radius, y - radius, 2*radius, 2*radius); }
    void DrawCircle(const wxPoint& pt, wxCoord radius)
        { DrawCircle(pt.x, pt.y, radius); }

    void DrawEllipse(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
        {AddToList(new gpdcDrawEllipseOp(x,y,width,height));}
    void DrawEllipse(const wxPoint& pt, const wxSize& sz)
        { DrawEllipse(pt.x, pt.y, sz.x, sz.y); }
    void DrawEllipse(const wxRect& rect)
        { DrawEllipse(rect.x, rect.y, rect.width, rect.height); }

    void DrawIcon(const wxIcon& icon, wxCoord x, wxCoord y)
        {AddToList(new gpdcDrawIconOp(icon,x,y));}
    void DrawIcon(const wxIcon& icon, const wxPoint& pt)
        { DrawIcon(icon, pt.x, pt.y); }

    void DrawLines(int n, wxPoint points[],
               wxCoord xoffset = 0, wxCoord yoffset = 0)
        {AddToList(new gpdcDrawLinesOp(n,points,xoffset,yoffset));}

    void DrawBitmap(const wxBitmap &bmp, wxCoord x, wxCoord y,
                    bool useMask = false)
        {AddToList(new gpdcDrawBitmapOp(bmp,x,y,useMask));}
    void DrawBitmap(const wxBitmap &bmp, const wxPoint& pt,
                    bool useMask = false)
        { DrawBitmap(bmp, pt.x, pt.y, useMask); }

    void DrawText(const wxString& text, wxCoord x, wxCoord y)
        {AddToList(new gpdcDrawTextOp(text, x, y));}
    void DrawText(const wxString& text, const wxPoint& pt)
        { DrawText(text, pt.x, pt.y); }

    void DrawRotatedText(const wxString& text, wxCoord x, wxCoord y, double angle)
        {AddToList(new gpdcDrawRotatedTextOp(text,x,y,angle));}
    void DrawRotatedText(const wxString& text, const wxPoint& pt, double angle)
        { DrawRotatedText(text, pt.x, pt.y, angle); }

    // this version puts both optional bitmap and the text into the given
    // rectangle and aligns is as specified by alignment parameter; it also
    // will emphasize the character with the given index if it is != -1 
    void DrawLabel(const wxString& text,
                           const wxBitmap& image,
                           const wxRect& rect,
                           int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                           int indexAccel = -1)
        {AddToList(new gpdcDrawLabelOp(text,image,rect,alignment,indexAccel));}

    void DrawLabel(const wxString& text, const wxRect& rect,
                   int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                   int indexAccel = -1)
        { DrawLabel(text, wxNullBitmap, rect, alignment, indexAccel); }

/*?????? I don't think that the source dc would stick around
    void Blit(wxCoord xdest, wxCoord ydest, wxCoord width, wxCoord height,
              wxDC *source, wxCoord xsrc, wxCoord ysrc,
              int rop = wxCOPY, bool useMask = false, wxCoord xsrcMask = wxDefaultCoord, wxCoord ysrcMask = wxDefaultCoord)
                {AddToList(new gpdcBlitOp(xdest,ydest,width,height,source,xsrc,
                                         ysrc,rop,useMask,xsrcMask,ysrcMask));}
    void Blit(const wxPoint& destPt, const wxSize& sz,
              wxDC *source, const wxPoint& srcPt,
              int rop = wxCOPY, bool useMask = false, const wxPoint& srcPtMask = wxDefaultPosition)
    {
        Blit(destPt.x, destPt.y, sz.x, sz.y, source, srcPt.x, srcPt.y, 
             rop, useMask, srcPtMask.x, srcPtMask.y);
    }
??????*/

#if wxUSE_SPLINES
    void DrawSpline(int n, wxPoint points[])
        {AddToList(new gpdcDrawSplineOp(n,points));}
#endif // wxUSE_SPLINES

#if wxUSE_PALETTE
    void SetPalette(const wxPalette& palette)
        {AddToList(new gpdcSetPaletteOp(palette));}
#endif // wxUSE_PALETTE

    void SetLogicalFunction(int function)
        {AddToList(new gpdcSetLogicalFunctionOp(function));}
    void SetFont(const wxFont& font) 
        {AddToList(new gpdcSetFontOp(font));}
    void SetPen(const wxPen& pen)
        {AddToList(new gpdcSetPenOp(pen));}
    void SetBrush(const wxBrush& brush)
        {AddToList(new gpdcSetBrushOp(brush));}
    void SetBackground(const wxBrush& brush)
        {AddToList(new gpdcSetBackgroundOp(brush));}
    void SetBackgroundMode(int mode)
        {AddToList(new gpdcSetBackgroundModeOp(mode));}
    void SetTextBackground(const wxColour& colour)
        {AddToList(new gpdcSetTextBackgroundOp(colour));}
    void SetTextForeground(const wxColour& colour)
        {AddToList(new gpdcSetTextForegroundOp(colour));}

    void Clear()
        {AddToList(new gpdcClearOp());}
    void BeginDrawing()
        {AddToList(new gpdcBeginDrawingOp());}
    void EndDrawing()
        {AddToList(new gpdcEndDrawingOp());}

protected:
    // ------------------------------------------------------------------------
    // protected helper methods
    void AddToList(gpdcOp *newOp);
    gpdcObject *FindObject(int id, bool create=false);
    
    // ------------------------------------------------------------------------
    // Data members
    // 
    int m_currId; // id to use for operations done on the PseudoDC
    gpdcObject *m_lastObject; // used to find last used object quickly
    gpdcObjectList m_objectlist; // list of objects
    gpdcObjectHash m_objectIndex; //id->object lookup index
    
};

#endif

