#include "starmap.h"
#include "starlist.h"
#include "import.h"
#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/rawbmp.h>
#include <wx/textdlg.h>

#define APP_QUIT    100
#define APP_ABOUT   101
#define APP_NAMES   201
#define APP_GRID    202
#define APP_LINES   203
#define APP_COLORS  204
#define APP_FLIP    205
#define APP_SEARCH  300
#define APP_FILTER  301

// some informative stuff

stardesc::stardesc(const Star *st, const Vector& ref)
  : star(st),
    prepped(FALSE)
{
  wxString tmp;
  if (!star->names.empty()) {
    desc << "Names: ";
    for (const auto& it : star->names) {
      desc << '\t' << it.name << '\n';
    }
  }
  if (!star->type.IsEmpty())
    desc << "Type: \t" << star->type << '\n';
  tmp.Printf("Pos: \t(%+.2f,%+.2f,%+.2f)\n",
	     // use units which seem natural for the user
	     star->pos.get_x() * LIGHTYEAR_PER_PARSEC,
	     star->pos.get_y() * LIGHTYEAR_PER_PARSEC,
	     -star->pos.get_z() * LIGHTYEAR_PER_PARSEC);
  desc << tmp;
  Vector stc(star->pos);
  stc -= ref;
  tmp.Printf("Dist: \t%.2f ly\n", stc.norm() * LIGHTYEAR_PER_PARSEC);
  desc << tmp;
  tmp.Printf("Vmag: \t%.2f\n", star->vmag);
  desc << tmp;
  if (!star->remarks.IsEmpty())
    desc << "Remarks: \t" << star->remarks << '\n';
  // anything else?
}

// main routine

StarFrame *frame = (StarFrame *)NULL;
wxMenuBar *menu_bar = (wxMenuBar *)NULL;

IMPLEMENT_APP(StarApp)

StarApp::StarApp(void)
{
}

bool StarApp::OnInit(void)
{
  frame = new StarFrame((wxFrame *)NULL, "Starmap", 0, 0, 550, 500);

  frame->SetStatusText("Loading...");
  frame->Show(TRUE);
  SetTopWindow(frame);

  import_all();

  return TRUE;
}

StarFrame::StarFrame(wxFrame *frame, const char *title, int x, int y, int w, int h)
  : wxFrame(frame, -1, title, wxPoint(x, y), wxSize(w, h))
{
  canvas = new StarCanvas(this);

  CreateStatusBar(2);

  wxMenu *file_menu = new wxMenu;
  file_menu->Append(APP_QUIT, "E&xit", "Quit Starmap");
  wxMenu *option_menu = new wxMenu;
  option_menu->Append(APP_NAMES,  "&Names", "Show star names", TRUE);
  option_menu->Append(APP_GRID,   "&Grid", "Show grid", TRUE);
  option_menu->Append(APP_LINES,  "&Lines", "Show lines to galactic plane", TRUE);
  option_menu->Append(APP_COLORS, "&Colors", "Show colors", TRUE);
  option_menu->Append(APP_FLIP,   "Fli&p", "Rotate 180 degrees around X axis", TRUE);
  wxMenu *view_menu = new wxMenu;
  view_menu->Append(APP_SEARCH,"&Search", "Find star names");
  wxMenu *help_menu = new wxMenu;
  help_menu->Append(APP_ABOUT, "&About", "About Starmap");
  menu_bar = new wxMenuBar;
  menu_bar->Append(file_menu, "&File");
  menu_bar->Append(option_menu, "&Options");
  menu_bar->Append(view_menu, "&View");
  menu_bar->Append(help_menu, "&Help");
  SetMenuBar(menu_bar);
  menu_bar->Check(APP_NAMES,  TRUE);
  menu_bar->Check(APP_GRID,   TRUE);
  menu_bar->Check(APP_LINES,  TRUE);
  menu_bar->Check(APP_COLORS, TRUE);
}

BEGIN_EVENT_TABLE(StarFrame, wxFrame)
  EVT_MENU(APP_QUIT,  StarFrame::Quit)
  EVT_MENU(APP_ABOUT, StarFrame::About)
  EVT_MENU(APP_NAMES, StarFrame::Option)
  EVT_MENU(APP_GRID,  StarFrame::Option)
  EVT_MENU(APP_LINES, StarFrame::Option)
  EVT_MENU(APP_COLORS,StarFrame::Option)
  EVT_MENU(APP_FLIP,  StarFrame::Option)
  EVT_MENU(APP_SEARCH,StarFrame::Search)
  EVT_SIZE(StarFrame::OnSize)
  EVT_CLOSE(StarFrame::OnCloseWindow)
END_EVENT_TABLE()

void StarFrame::Quit(wxCommandEvent& WXUNUSED(event) )
{
  Close(TRUE);
}

void StarFrame::About(wxCommandEvent& WXUNUSED(event) )
{
  (void)wxMessageBox(wxT("Starmap\nby Ove K\u00e5ven"),
                     wxT("About Starmap"), wxOK|wxCENTRE);
}

void StarFrame::Option(wxCommandEvent& WXUNUSED(event) )
{
  canvas->Redraw();
}

void StarFrame::Search(wxCommandEvent& WXUNUSED(event) )
{
  wxString str = wxGetTextFromUser("Star name", "Search", "", this);

  if (str.IsEmpty()) {
    // wxMessageBox("Nothing entered.", "Search", wxOK|wxCENTRE|wxICON_EXCLAMATION, this);
    return;
  }

  SetStatusText("Searching...");
  for (const auto star : stars) {
    for (const auto &nit : star->names) {
      if (nit.name.Find(str) >= 0) {
	// found a match, center on it
	canvas->pos = Vector(-star->pos.get_x(), -star->pos.get_y(), canvas->pos.depth());
	Refresh();
	return;
      }
    }
  }
  SetStatusText("Ready.");
  wxMessageBox("No match found.", "Search", wxOK|wxCENTRE|wxICON_EXCLAMATION, this);
}

void StarFrame::OnSize(wxSizeEvent& WXUNUSED(event) )
{
  canvas->SetSize(GetClientSize());
}

void StarFrame::OnCloseWindow(wxCloseEvent& WXUNUSED(event) )
{
  Destroy();
}

StarCanvas::StarCanvas(wxFrame *parent)
  : wxWindow(parent, -1),
    pos(0, 0, 35), // 35 parsec away from standard galactic plane
    refpos(0, 0, SOL_Z_OFFSET), // use Sol's position as initial ref
    pitch(0),
    zoom(1.0),
    need_realloc(FALSE),
    need_render(FALSE),
    need_paint(FALSE),
    ready(FALSE)
{
  SetBackgroundColour(*wxBLACK);
  SetCursor(*wxCROSS_CURSOR);
}

BEGIN_EVENT_TABLE(StarCanvas, wxWindow)
  EVT_SIZE(StarCanvas::OnSize)
  EVT_CHAR(StarCanvas::OnChar)
  EVT_KEY_DOWN(StarCanvas::OnKeyDown)
  EVT_MOTION(StarCanvas::OnMotion)
  EVT_LEAVE_WINDOW(StarCanvas::OnLeaveWindow)
  EVT_LEFT_DOWN(StarCanvas::OnLeftDown)
  EVT_IDLE(StarCanvas::OnIdle)
  EVT_PAINT(StarCanvas::OnPaint)
END_EVENT_TABLE()

void StarCanvas::OnSize(wxSizeEvent& WXUNUSED(event) )
{
  need_realloc = TRUE;
  need_render = TRUE;
}

void StarCanvas::OnChar(wxKeyEvent& event)
{
  bool flip = menu_bar->IsChecked(APP_FLIP);
  double factor = event.ControlDown() ? 10.0 : 1.0;
  switch(event.GetKeyCode()) {
  case WXK_LEFT:
    pos += Vector(+zoom/20.0, 0.0, 0.0) * factor;
    Redraw();
    break;
  case WXK_RIGHT:
    pos += Vector(-zoom/20.0, 0.0, 0.0) * factor;
    Redraw();
    break;
  case WXK_UP:
    pos += Vector(0.0, flip ? -zoom/20.0 : +zoom/20.0, 0.0) * factor;
    Redraw();
    break;
  case WXK_DOWN:
    pos += Vector(0.0, flip ? +zoom/20.0 : -zoom/20.0, 0.0) * factor;
    Redraw();
    break;
  case WXK_PAGEUP:
    pos += Vector(0.0, 0.0, -zoom/2.0) * factor;
    Redraw();
    break;
  case WXK_PAGEDOWN:
    pos += Vector(0.0, 0.0, +zoom/2.0) * factor;
    Redraw();
    break;
  case WXK_HOME:
    pitch += 5.0*M_PI/180.0;
    Redraw();
    break;
  case WXK_END:
    pitch -= 5.0*M_PI/180.0;
    Redraw();
    break;
  case '+':
  case WXK_ADD:
    zoom /= 1.1;
    Redraw();
    break;
  case '-':
  case WXK_SUBTRACT:
    zoom *= 1.1;
    Redraw();
    break;
  default:
    event.Skip();
  }
}

void StarCanvas::OnKeyDown(wxKeyEvent& event)
{
  event.Skip();
}

void StarCanvas::OnMotion(wxMouseEvent& event)
{
  int mdist = 19, xd, yd, td;
  bool any, was;

  if (!ready) return;

  any = FALSE;
  was = !descs.empty();
  descpt.x = event.GetX();
  descpt.y = event.GetY();

  // find closest star(s) to pointer
  select.clear();
  for (const auto star : stars) {
    if (star->show) {
      xd = star->proj.x - descpt.x;
      if (xd < 0) xd = -xd;
      yd = star->proj.y - descpt.y;
      if (yd < 0) yd = -yd;

      if ((xd < 4) && (yd < 4)) {
	td = xd*xd + yd*yd;

	if (td < mdist) {
	  // closer star, clear list and select this
	  mdist = td;
	  select.clear();
	  select.push_back(star);
	  any = TRUE;
	}
	else if (td == mdist) {
	  // star at same distance, add it to list
	  select.push_back(star);
	  any = TRUE;
	}
      }
    }
  }

  // create descriptions
  CreateDescs();

  if (any || was) Repaint(FALSE);
}

void StarCanvas::OnLeaveWindow(wxMouseEvent& WXUNUSED(event) )
{
  // if cursor left window, remove descriptions
  if (!descs.empty())
    Redraw();
}

void StarCanvas::OnLeftDown(wxMouseEvent& WXUNUSED(event) )
{
  // left button click sets the reference point to selected star
  if (!select.empty()) {
    const auto star = select.front();
    refpos = star->pos;

    // recreate descriptions
    CreateDescs();
    Repaint(FALSE);
  }
}

void StarCanvas::OnIdle(wxIdleEvent& WXUNUSED(event) )
{
  if (need_render) RenderView();
  if (need_paint) DoRepaint();
}

void StarCanvas::OnPaint(wxPaintEvent& WXUNUSED(event) )
{
  wxPaintDC pdc(this);
  DoPaint(pdc);
}

static wxColour::ChannelType BlendComponent(wxColour::ChannelType a,
                                            wxColour::ChannelType b)
{
#if 1
  // Screen
  return 255 - (255 - a) * (255 - b) / 255;
#else
  // Saturated add
  unsigned sum = a + b;
  return sum <= 255 ? sum : 255;
#endif
}

static void BlendPixel(wxNativePixelData::Iterator& pixel, const wxColour& color, bool colors)
{
  if (colors) {
    pixel.Red()   = BlendComponent(pixel.Red(),   color.Red());
    pixel.Green() = BlendComponent(pixel.Green(), color.Green());
    pixel.Blue()  = BlendComponent(pixel.Blue(),  color.Blue());
  } else {
    pixel.Red()   = color.Red();
    pixel.Green() = color.Green();
    pixel.Blue()  = color.Blue();
  }
}

void StarCanvas::RenderStars()
{
  bool colors = menu_bar->IsChecked(APP_COLORS);
  dc->SelectObject(wxNullBitmap);
  {
    wxNativePixelData data(*bmp);
    auto pixels = data.GetPixels();
    for (const auto star : stars) {
      if (star->show &&
          star->proj.y > 1 && star->proj.y < data.GetHeight() - 1 &&
          star->proj.x > 1 && star->proj.x < data.GetWidth() - 1) {
        wxColour color = colors ? star->color : *wxWHITE;
        pixels.MoveTo(data, star->proj.x, star->proj.y - 1);
        BlendPixel(pixels, color, colors);
        pixels.MoveTo(data, star->proj.x - 1, star->proj.y);
        BlendPixel(pixels, color, colors);
        pixels++;
        BlendPixel(pixels, color, colors);
        pixels++;
        BlendPixel(pixels, color, colors);
        pixels.MoveTo(data, star->proj.x, star->proj.y + 1);
        BlendPixel(pixels, color, colors);
      }
    }
  }
  dc->SelectObject(*bmp);
}

void StarCanvas::RenderView()
{
  wxSize siz(GetClientSize());
  wxRect rect(wxPoint(0, 0), siz);
  wxRegion area(rect);
  double factor = sqrt(siz.GetX()*siz.GetX() + siz.GetY()*siz.GetY())*4.0 / zoom;
  int mx = siz.GetX()/2, my = siz.GetY()/2;
  bool names = menu_bar->IsChecked(APP_NAMES);
  bool grid = menu_bar->IsChecked(APP_GRID);
  bool lines = menu_bar->IsChecked(APP_LINES);
  bool flip = menu_bar->IsChecked(APP_FLIP);

  if (!bmp || need_realloc) {
    bmp = std::make_unique<wxBitmap>(siz.GetX(), siz.GetY(), 24);
    dc = std::make_unique<wxMemoryDC>();
    dc->SelectObject(*bmp);
    need_realloc = FALSE;
  }

  dc->SetBackground(*wxBLACK_BRUSH);
  dc->Clear();

  Transform cam(pitch, Angle(), Angle(), pos, flip);

  // update frame's status bar
  wxFrame *frame = (wxFrame *)GetParent();
  frame->SetStatusText("Drawing...", 0);
  {
    wxString ptext;
    double px, py, pz;
    pos.get(px, py, pz);
    ptext.Printf("(%+.2f,%+.2f,%+.2f) x%.2f",
		 // use units which seem natural for the user
		 -px * LIGHTYEAR_PER_PARSEC,
		 -py * LIGHTYEAR_PER_PARSEC,
		 pz * LIGHTYEAR_PER_PARSEC,
		 1.0 / zoom);
    frame->SetStatusText(ptext, 1);
  }

  // draw grid
  if (grid && !pos.behind()) {
    Vector grid_pos = flip ? pos.multiply(Vector(1.0, -1.0, 1.0)) : pos;
    // not sure of the best way to draw it
    wxPoint origin = grid_pos.pproject(factor, mx, my);

    double fac = factor / LIGHTYEAR_PER_PARSEC / pos.depth();
    // select a somewhat decent grid factor
    if (fac < 10) fac *= 2;
    if (fac < 10) fac *= 5;

    double left = origin.x, top = origin.y;
    while (left > 0) left -= fac;
    while (top > 0) top -= fac;
    while (left < 0) left += fac;
    while (top < 0) top += fac;

    // draw grid with gray pen
    dc->SetPen(*wxGREY_PEN);

    while (left < siz.GetX()) {
      long x = (long)left;
      dc->DrawLine(x, 0, x, siz.GetY());
      left += fac;
    }

    while (top < siz.GetY()) {
      long y = (long)top;
      dc->DrawLine(0, y, siz.GetX(), y);
      top += fac;
    }
  }

  // first pass, calculate positions
  for (const auto star : stars) {
    Vector np = star->get_pos() * cam;
    if (np.behind()) star->show = FALSE; else {
      star->proj = np.pproject(factor, mx, my);
      star->show = area.Contains(star->proj) != wxOutRegion;
    }
  }

  // draw names (before the stars themselves, so the stars come on top)
  if (names) {
    dc->SetFont(*wxSMALL_FONT);
    dc->SetBackgroundMode(wxTRANSPARENT);
    dc->SetTextForeground(*wxGREEN);
    for (const auto star : stars) {
      if (star->show && !star->names.empty()) {
        const auto &nit = star->names.front();
        if (!star->te) {
          dc->GetTextExtent(nit.name, &star->tw, &star->th);
          star->te = TRUE;
        }
        if (star->comp) // binary/trinary star systems or something?
          dc->DrawText(nit.name, star->proj.x - star->tw/2, star->proj.y + star->th * (star->comp - 2));
        else
          dc->DrawText(nit.name, star->proj.x - star->tw/2, star->proj.y - star->th);
      }
    }
  }

  // draw stars
  dc->SetBrush(*wxWHITE_BRUSH);
  dc->SetPen(*wxTRANSPARENT_PEN);
  for (const auto star : stars) {
    if (star->show) {
      if (lines) {
	Vector p(star->get_pos());
	p.flatten();
	Vector np = p * cam;
	if (!np.behind()) {
	  wxPoint bp = np.pproject(factor, mx, my);
	  // if the endpoint is outside screen, don't plot it
	  // even if the star is inside, to avoid clutter and slowdown
	  if (area.Contains(bp) != wxOutRegion) {
	    dc->SetPen(*wxCYAN_PEN);
	    dc->DrawLine(bp.x, bp.y, star->proj.x, star->proj.y);
	    dc->SetPen(*wxTRANSPARENT_PEN);
	  }
	}
      }
    }
  }

  RenderStars();
  need_render = FALSE;
  need_paint = TRUE;

  frame->SetStatusText("Ready.", 0);
  ready = TRUE;
}

void StarCanvas::DoPaint(wxDC& pdc)
{
  if (!ready) return;

  wxSize siz(GetClientSize());
  int mx = siz.GetX()/2, my = siz.GetY()/2;

  pdc.Blit(0, 0, siz.GetX(), siz.GetY(), dc.get(), 0, 0, wxCOPY, FALSE);

  // update description boxes
  pdc.SetFont(*wxSMALL_FONT);
  if (!descs.empty()) {
    int bw = 0, bh = 0;
    // first, calculate their sizes
    for (auto &desc : descs) {
      if (!desc.prepped) {
        desc.siz = CalcBox(pdc, desc.desc);
        desc.prepped = TRUE;
      }
      bw += desc.siz.x;
      if (desc.siz.y > bh) bh = desc.siz.y;
    }

    // calculate appropriate position
    wxPoint doff;
    doff.x = descpt.x - (bw / 2);
    if (doff.x + bw > siz.x) doff.x = siz.x - bw;
    if (doff.x < 0) doff.x = 0;
    if (descpt.y > my) doff.y = descpt.y - bh;
    else doff.y = descpt.y;

    // place and draw boxes
    for (auto &desc : descs) {
      desc.pos = doff;
      doff.x += desc.siz.x;
      ShowBox(pdc, desc.desc, desc.pos);
    }
  }
}

void StarCanvas::DoRepaint(void)
{
  wxClientDC pdc(this);
  DoPaint(pdc);
  need_paint = FALSE;
}

void StarCanvas::Redraw()
{
  ClearDescs();
  need_render = TRUE;
}

void StarCanvas::Repaint(bool clr_desc)
{
  if (clr_desc) ClearDescs();
  need_paint = TRUE;
}

void StarCanvas::CreateDescs(void)
{
  ClearDescs();
  for (const auto star : select) {
    descs.emplace_back(star, refpos);
  }
}

void StarCanvas::ClearDescs(void)
{
  descs.clear();
}

wxSize StarCanvas::CalcBox(wxDC& pdc, wxString txt, int *tabpos)
{
  const wxChar *dat = txt.wx_str();
  const wxChar *next;
  long cw[2] = {0, 0};
  wxCoord ch = 0, w, h;
  int cp = 0;
  size_t len;
  wxSize ret(0, 0);

  do {
    next = wxStrpbrk(dat, wxT("\t\n"));
    len = next ? (next - dat) : wxStrlen(dat);
    pdc.GetTextExtent(wxString(dat, len), &w, &h);
    if (h > ch) ch = h;
    if (w > cw[cp]) cw[cp] = w;
    if ((!next) || (*next == '\n')) {
      ret.y += ch;
      ch = 0;
      cp = 0;
    } else cp++;
    dat = next ? (next+1) : (const wxChar *)NULL;
  } while (dat && *dat);

  for (cp=0; cp<2; cp++) {
    if (tabpos)
      tabpos[cp] = ret.x;
    ret.x += cw[cp];
  }

  // add room for containing rectangle
  ret.x += 3;
  ret.y += 3;

  return ret;
}

void StarCanvas::ShowBox(wxDC& pdc, wxString txt, wxPoint pos)
{
  int tabpos[2], cp = 0;
  wxSize siz = CalcBox(pdc, txt, tabpos);
  const wxChar *dat = txt.wx_str();
  const wxChar *next;
  wxCoord ch = 0, h;
  size_t len;

  pdc.SetBrush(*wxBLACK_BRUSH);
  pdc.SetPen(*wxWHITE_PEN);
  pdc.DrawRectangle(pos.x, pos.y, siz.x, siz.y);
  pos.x += 2;
  pos.y += 2;

  pdc.SetBackgroundMode(wxTRANSPARENT);
  pdc.SetTextForeground(*wxGREEN);

  do {
    next = wxStrpbrk(dat, wxT("\t\n"));
    len = next ? (next - dat) : wxStrlen(dat);
    pdc.DrawText(wxString(dat, len), pos.x + tabpos[cp], pos.y);
    pdc.GetTextExtent(wxString(dat, len), NULL, &h);
    if (h > ch) ch = h;
    if ((!next) || (*next == '\n')) {
      pos.y += ch;
      ch = 0;
      cp = 0;
    } else cp++;
    dat = next ? (next+1) : (const wxChar *)NULL;
  } while (dat && *dat);
}
