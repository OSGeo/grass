%{
#include <wx/wxPython/wxPython.h>
#include <wx/wxPython/pyclasses.h>
#include <wx/dcbuffer.h>
%}

%{
#include "pseudodc.h"
%}

%rename(PseudoDC) gwxPseudoDC;

%typemap(out) wxRect {
	$result = Py_BuildValue("iiii", $1.x, $1.y, $1.width, $1.height);
}

class gwxPseudoDC
{
public:
	gwxPseudoDC();
	~gwxPseudoDC();
	void Clear();
    	void ClearId(int);
	void RemoveAll();
	void RemoveId(int);
	void BeginDrawing();
	void EndDrawing();
        void SetBackground(const wxBrush&);
	void SetId(int);
        void DrawBitmap(const wxBitmap&, const wxPoint&,
	                bool);
        void SetBrush(const wxBrush&);
        void SetPen(const wxPen&);
	void SetIdBounds(int, wxRect&);
	void DrawLine(const wxPoint&, const wxPoint&);
	%extend {
		void DrawToDC(wxBufferedPaintDC *dc) {
			self->DrawToDC((wxDC *) dc);
		}
		void DrawToDC(wxGCDC *dc) {
			self->DrawToDC((wxDC *) dc);
		}
		void DrawToDCClipped(wxBufferedPaintDC *dc, const wxRect& rect) {
			self->DrawToDCClipped((wxDC *) dc, rect);
		}
		void DrawToDCClipped(wxGCDC *dc, const wxRect& rect) {
			self->DrawToDCClipped((wxDC *) dc, rect);
		}
		wxRect GetIdBounds(int id) {
			wxRect rect;
			self->GetIdBounds(id, rect);
			return rect;
		}
		void TranslateId(int id, int dx, int dy) {
		        self->TranslateId(id, (wxCoord) dx, (wxCoord) dy);
		}
		PyObject *FindObjects(int x, int y, int radius) {
		        return self->FindObjects((wxCoord) x, (wxCoord) y,
			                         (wxCoord) radius, *wxWHITE);
                }
		void DrawRectangleRect(const wxRect& rect) {
		        return self->DrawRectangle(rect);
                }
	}
};
