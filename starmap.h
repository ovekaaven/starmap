#include "maths.h"
#include <list>
#include <memory>
#include <wx/app.h>
#include <wx/dcmemory.h>
#include <wx/frame.h>

// some definitions

#define LIGHTYEAR_PER_PARSEC 3.26

// Sol is at least 14 light years above galactic plane,
// but I'm not sure which way. For now, pretend we're
// on the galactic plane.
#define SOL_Z_OFFSET 0

// other stuff we might need someday
#define ECLIPTIC 84381.412 // arcsec
#define LIGHTSEC 299792.458 // km/s
#define PRECESSION 5028.83  // arcsec/century

class Star;

// the user interface

class stardesc {
 public:
  const Star *star;
  wxString desc;
  bool prepped;
  wxPoint pos;
  wxSize siz;
  stardesc(const Star *st, const Vector& ref);
};

class StarApp : public wxApp
{
 public:
  StarApp(void);
  bool OnInit(void);
};

class StarCanvas;
class StarFrame : public wxFrame
{
 public:
  StarCanvas *canvas;

  StarFrame(wxFrame *parent, const char *title, int x, int y, int w, int h);

  void OnSize(wxSizeEvent& event);
  void OnCloseWindow(wxCloseEvent& event);

  void Quit(wxCommandEvent& event);
  void About(wxCommandEvent& event);
  void Option(wxCommandEvent& event);
  void Search(wxCommandEvent& event);

  DECLARE_EVENT_TABLE()
};

class StarCanvas : public wxWindow
{
 public:
  Vector pos, refpos;
  Angle pitch;
  double zoom;
  bool need_realloc, need_render, need_paint, ready;
  std::unique_ptr<wxBitmap> bmp;
  std::unique_ptr<wxMemoryDC> dc;

  std::list<const Star*> select;
  std::list<stardesc> descs;
  wxPoint descpt;

  StarCanvas(wxFrame *parent);

  void OnSize(wxSizeEvent& event);
  void OnChar(wxKeyEvent& event);
  void OnKeyDown(wxKeyEvent& event);
  void OnMotion(wxMouseEvent& event);
  void OnLeaveWindow(wxMouseEvent& event);
  void OnLeftDown(wxMouseEvent& event);
  void OnIdle(wxIdleEvent& event);
  void OnPaint(wxPaintEvent& event);
  void RenderStars();
  void RenderView();
  void DoPaint(wxDC& pdc);
  void DoRepaint(void);
  void Redraw();
  void Repaint(bool clr_desc = TRUE);
  void CreateDescs(void);
  void ClearDescs(void);
  wxSize CalcBox(wxDC& pdc, wxString txt, int *tabpos = (int *)NULL);
  void ShowBox(wxDC& pdc, wxString txt, wxPoint pos);

  DECLARE_EVENT_TABLE()
};
