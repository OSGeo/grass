%{
#include <wx/wxPython/wxPython.h>
#include <wx/wxPython/pyclasses.h>
#include <wx/dcbuffer.h>
%}

%{
#include "pseudodc.h"
%}

%rename(PseudoDC) gwxPseudoDC;

%typemap(in) wxString& (bool temp=false) {
    $1 = wxString_in_helper($input);
    if ($1 == NULL) SWIG_fail;
    temp = true;
}
%typemap(freearg) wxString& {
    if (temp$argnum)
        delete $1;
}

%apply wxString& { wxString* };


%typemap(in) wxRect& (wxRect temp) {
    $1 = &temp;
    if ( ! wxRect_helper($input, &$1)) SWIG_fail;
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_POINTER) wxRect& {
    $1 = wxPySimple_typecheck($input, wxT("wxRect"), 4);
}

%apply wxRect& { wxRect* };

%typemap(out) wxRect {
    $result = Py_BuildValue("[iiii]", $1.x, $1.y, $1.width, $1.height);
}

%typemap(out) wxArrayInt& {
    $result = wxArrayInt2PyList_helper(*$1);
}

%typemap(out) wxArrayInt {
    $result = wxArrayInt2PyList_helper($1);
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
        void SetBrush(const wxBrush&);
        void SetPen(const wxPen&);
	void SetIdBounds(int, wxRect&);
	void DrawLine(const wxPoint&, const wxPoint&);
	void SetFont(const wxFont&);
	void SetTextForeground(const wxColour&);
	%extend {
		void DrawToDC(void *dc) {
			self->DrawToDC((wxDC *) dc);
		}
		void DrawToDCClipped(void *dc, const wxRect& rect) {
			self->DrawToDCClipped((wxDC *) dc, rect);
		}
		wxRect GetIdBounds(int id) {
			wxRect rect;
			self->GetIdBounds(id, rect);
			return rect;
		}
		void TranslateId(int id, float dx, float dy) {
		        self->TranslateId(id, (wxCoord) dx, (wxCoord) dy);
		}
		PyObject *FindObjects(float x, float y, int radius) {
		        return self->FindObjects((wxCoord) x, (wxCoord) y,
			                         (wxCoord) radius, *wxWHITE);
                }
		void DrawRectangleRect(const wxRect& rect) {
		        return self->DrawRectangle(rect);
                }
		void DrawText(const wxString& text, float x, float y) {
		        return self->DrawText(text, (wxCoord) x, (wxCoord) y);
		}
		void DrawRotatedText(const wxString& text, float x, float y, double angle) {
		        return self->DrawRotatedText(text, (wxCoord) x, (wxCoord) y, angle);
		}
                void DrawBitmap(const wxBitmap& bitmap, float x, float y, bool useMask) {
                        return self->DrawBitmap(bitmap, (wxCoord) x, (wxCoord) y, useMask);
                }
		void DrawLinePoint(const wxPoint& p1, const wxPoint& p2) {
		        return self->DrawLine(p1, p2);
                }
	}
};
