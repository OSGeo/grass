#ifndef WXNVIZ_H
#define WXNVIZ_H

#include <string>
#include <vector>

extern "C" {
#include <grass/gis.h>
#include <grass/gsurf.h>
#include <grass/gstypes.h>
#include <grass/nviz.h>
}

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include <wx/glcanvas.h>

class Nviz
{
private:
    nv_data *data;
    wxGLCanvas *glCanvas;

    /* surface.cpp */
    int SetSurfaceAttr(int, int, bool, const char *);
    int UnsetSurfaceAttr(int, int);

public:
    /* constructor */
    Nviz();

    /* destructor */
    ~Nviz();

    std::string GetErrorMsg();

    /* change_view.cpp */
    int ResizeWindow(int, int);
    std::vector<double> SetViewDefault();
    int SetView(float, float,
		float, float, float);
    int SetZExag(float);

    /* init.cpp */
    int SetDisplay(void *);
    void InitView();
    void Reset();
    void SetBgColor(const char *);

    /* lights.cpp */
    void SetLightsDefault();

    /* load.cpp */
    int LoadSurface(const char*, const char *, const char *);
    int UnloadSurface(int);
    int LoadVector(const char *, bool);
    int UnloadVector(int, bool);

    /* draw.cpp */
    void Draw(bool, bool, bool);
    void EraseMap();

    /* surface.cpp */
    int SetSurfaceTopo(int, bool, const char *);
    int SetSurfaceColor(int, bool, const char *);
    int SetSurfaceMask(int, bool, const char *);
    int SetSurfaceTransp(int, bool, const char *);
    int SetSurfaceShine(int, bool, const char *);
    int SetSurfaceEmit(int, bool, const char *);
    int UnsetSurfaceMask(int);
    int UnsetSurfaceTransp(int);
    int UnsetSurfaceEmit(int);
    int SetSurfaceRes(int, int, int);
    int SetSurfaceStyle(int, int);
    int SetWireColor(int, const char *);
    std::vector<double> GetSurfacePosition(int);
    int SetSurfacePosition(int, float, float, float);

    /* vector */
    int SetVectorLineMode(int, const char *, int, int);
    int SetVectorLineHeight(int, float);
    int SetVectorLineSurface(int, int);
    int SetVectorPointMode(int, const char*, int, float, int);
    int SetVectorPointHeight(int, float);
    int SetVectorPointSurface(int, int);
};

#endif /* WXNVIZ_H */
