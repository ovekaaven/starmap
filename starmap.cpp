#include "starmap.h"
#include <stdio.h>

#define APP_QUIT    100
#define APP_ABOUT   101
#define APP_FLICKER 200
#define APP_NAMES   201
#define APP_GRID    202
#define APP_LINES   203
#define APP_FLIP    204
#define APP_SEARCH  300
#define APP_FILTER  301

const coords coords::null(0, 0, 0);

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(desclist);

// some informative stuff

stardesc::stardesc(stardata *st, coords& ref)
  : star(st),
    prepped(FALSE)
{
  wxString tmp;
  wxnamelistNode *nnode = star->names.GetFirst();
  if (nnode) {
    desc << "Names: ";
    while (nnode) {
      starname *name = nnode->GetData();
      desc << '\t' << name->name << '\n';
      nnode = nnode->GetNext();
    }
  }
  if (!star->type.IsEmpty())
    desc << "Type: \t" << star->type << '\n';
  tmp.Printf("Pos: \t(%+.2f,%+.2f,%+.2f)\n",
	     // use units which seem natural for the user
	     star->x * LIGHTYEAR_PER_PARSEC,
	     star->y * LIGHTYEAR_PER_PARSEC,
	     -star->z * LIGHTYEAR_PER_PARSEC);
  desc << tmp;
  coords stc(star->x, star->y, star->z);
  stc -= ref;
  tmp.Printf("Dist: \t%.2f ly\n", (double)distance(stc) * LIGHTYEAR_PER_PARSEC);
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
  printf("Loading Gliese star catalog...\n");
  read_gliese3("gliese/gliese3.dat");
  printf("Merging Yale bright star catalog...\n");
  read_bright("bright/catalog.dat", "bright/notes.dat");
  printf("Loaded %u stars. Starting...\n", stars.GetCount());

  frame = new StarFrame((wxFrame *)NULL, "Starmap", 0, 0, 550, 500);

  frame->SetStatusText("Loading...");
  frame->Show(TRUE);
  SetTopWindow(frame);

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
  option_menu->Append(APP_FLICKER,"&Flicker", "Trades speed against flicker", TRUE);
  option_menu->Append(APP_NAMES,  "&Names", "Show star names", TRUE);
  option_menu->Append(APP_GRID,   "&Grid", "Show grid", TRUE);
  option_menu->Append(APP_LINES,  "&Lines", "Show lines to galactic plane", TRUE);
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
  menu_bar->Check(APP_NAMES, TRUE);
  menu_bar->Check(APP_GRID,  TRUE);
  menu_bar->Check(APP_LINES, TRUE);
}

BEGIN_EVENT_TABLE(StarFrame, wxFrame)
  EVT_MENU(APP_QUIT,  StarFrame::Quit)
  EVT_MENU(APP_ABOUT, StarFrame::About)
  EVT_MENU(APP_NAMES, StarFrame::Option)
  EVT_MENU(APP_GRID,  StarFrame::Option)
  EVT_MENU(APP_LINES, StarFrame::Option)
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
  Refresh();
}

void StarFrame::Search(wxCommandEvent& WXUNUSED(event) )
{
  wxString str = wxGetTextFromUser("Star name", "Search", "", this);

  if (str.IsEmpty()) {
    // wxMessageBox("Nothing entered.", "Search", wxOK|wxCENTRE|wxICON_EXCLAMATION, this);
    return;
  }

  SetStatusText("Searching...");
  wxstarlistNode *node = stars.GetFirst();
  while (node) {
    stardata *star = node->GetData();
    wxnamelistNode *nnode = star->names.GetFirst();
    while (nnode) {
      starname *name = nnode->GetData();
      if (name->name.Find(str) >= 0) {
	// found a match, center on it
	canvas->pos = coords(-star->x, -star->y, canvas->pos.depth());
	Refresh();
	return;
      }
      nnode = nnode->GetNext();
    }
    node = node->GetNext();
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
    zoom(0.5),
    need_redraw(FALSE),
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
  Refresh();
}

void StarCanvas::OnChar(wxKeyEvent& event)
{
  bool flip = menu_bar->IsChecked(APP_FLIP);
  switch(event.GetKeyCode()) {
  case WXK_LEFT:
    pos += coords(+zoom/10, 0, 0);
    Redraw();
    break;
  case WXK_RIGHT:
    pos += coords(-zoom/10, 0, 0);
    Redraw();
    break;
  case WXK_UP:
    pos += coords(0, flip ? -zoom/10 : +zoom/10, 0);
    Redraw();
    break;
  case WXK_DOWN:
    pos += coords(0, flip ? +zoom/10 : -zoom/10, 0);
    Redraw();
    break;
  case WXK_PAGEUP:
    pos += coords(0, 0, -zoom/2);
    Redraw();
    break;
  case WXK_PAGEDOWN:
    pos += coords(0, 0, +zoom/2);
    Redraw();
    break;
  case WXK_HOME:
    pitch += 5*M_PI/180;
    Redraw();
    break;
  case WXK_END:
    pitch -= 5*M_PI/180;
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
  was = (descs.GetFirst() != NULL);
  descpt.x = event.GetX();
  descpt.y = event.GetY();

  // find closest star(s) to pointer
  select.Clear();
  wxstarlistNode *node = stars.GetFirst();
  while (node) {
    stardata *star = node->GetData();

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
	  select.Clear();
	  select.Append(star);
	  any = TRUE;
	}
	else if (td == mdist) {
	  // star at same distance, add it to list
	  select.Append(star);
	  any = TRUE;
	}
      }
    }

    node = node->GetNext();
  }

  // create descriptions
  CreateDescs();

  if (any || was) Redraw(FALSE);
}

void StarCanvas::OnLeaveWindow(wxMouseEvent& WXUNUSED(event) )
{
  // if cursor left window, remove descriptions
  if (descs.GetFirst())
    Redraw();
}

void StarCanvas::OnLeftDown(wxMouseEvent& WXUNUSED(event) )
{
  // left button click sets the reference point to selected star
  wxstarlistNode *node = select.GetFirst();
  if (node) {
    stardata *star = node->GetData();
    refpos = coords(star->x, star->y, star->z);

    // recreate descriptions
    CreateDescs();
    Redraw(FALSE);
  }
}

void StarCanvas::OnIdle(wxIdleEvent& WXUNUSED(event) )
{
  if (need_redraw) DoRedraw();
}

void StarCanvas::OnPaint(wxPaintEvent& WXUNUSED(event) )
{
  wxPaintDC dc(this);
  dc.SetBackground(*wxBLACK_BRUSH); // just in case
  DoPaint(dc);
}

void StarCanvas::DoPaint(wxDC& dc)
{
  wxSize siz(GetClientSize());
  wxRect rect(wxPoint(0, 0), siz);
  wxRegion area(rect);
  double factor = sqrt(siz.GetX()*siz.GetX() + siz.GetY()*siz.GetY())*2 / zoom;
  int mx = siz.GetX()/2, my = siz.GetY()/2;
  bool names = menu_bar->IsChecked(APP_NAMES);
  bool grid = menu_bar->IsChecked(APP_GRID);
  bool lines = menu_bar->IsChecked(APP_LINES);
  bool flip = menu_bar->IsChecked(APP_FLIP);

  tmatrix cam(pitch, 0, 0, pos, flip);

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
		 0.5 / zoom);
    frame->SetStatusText(ptext, 1);
  }

  // draw grid
  if (grid && !pos.behind()) {
    coords grid_pos = flip ? pos.multiply(coords(1,-1,1)) : pos;
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
    dc.SetPen(*wxGREY_PEN);

    while (left < siz.GetX()) {
      long x = (long)left;
      dc.DrawLine(x, 0, x, siz.GetY());
      left += fac;
    }

    while (top < siz.GetY()) {
      long y = (long)top;
      dc.DrawLine(0, y, siz.GetX(), y);
      top += fac;
    }
  }

  // first pass, calculate positions
  wxstarlistNode *node = stars.GetFirst();
  while (node) {
    stardata *star = node->GetData();
    coords np = star->get_pos() * cam;
    if (np.behind()) star->show = FALSE; else {
      star->proj = np.pproject(factor, mx, my);
      star->show = area.Contains(star->proj) != wxOutRegion;
    }
    node = node->GetNext();
  }

  // draw names (before the stars themselves, so the stars come on top)
  if (names) {
    dc.SetFont(*wxSMALL_FONT);
    dc.SetBackgroundMode(wxTRANSPARENT);
    dc.SetTextForeground(*wxGREEN);
    node = stars.GetFirst();
    while (node) {
      stardata *star = node->GetData();
      if (star->show) {
	wxnamelistNode *nnode = star->names.GetFirst();
	if (nnode) {
	  starname *name = nnode->GetData();
	  if (!star->te) {
	    dc.GetTextExtent(name->name, &star->tw, &star->th);
	    star->te = TRUE;
	  }
	  if (star->comp) // binary/trinary star systems or something?
	    dc.DrawText(name->name, star->proj.x - star->tw/2, star->proj.y + star->th * (star->comp - 2));
	  else
	    dc.DrawText(name->name, star->proj.x - star->tw/2, star->proj.y - star->th);
	}
      }
      node = node->GetNext();
    }
  }

  // draw stars
  dc.SetBrush(*wxWHITE_BRUSH);
  dc.SetPen(*wxTRANSPARENT_PEN);
  node = stars.GetFirst();
  while (node) {
    stardata *star = node->GetData();
    if (star->show) {
      if (lines) {
	coords p(star->get_pos());
	p.flatten();
	coords np = p * cam;
	if (!np.behind()) {
	  wxPoint bp = np.pproject(factor, mx, my);
	  // if the endpoint is outside screen, don't plot it
	  // even if the star is inside, to avoid clutter and slowdown
	  if (area.Contains(bp) != wxOutRegion) {
	    dc.SetPen(*wxCYAN_PEN);
	    dc.DrawLine(bp.x, bp.y, star->proj.x, star->proj.y);
	    dc.SetPen(*wxTRANSPARENT_PEN);
	  }
	}
      }
      dc.DrawEllipse(star->proj.x-1, star->proj.y-1, 3, 3);
    }
    node = node->GetNext();
  }

  // update description boxes
  dc.SetFont(*wxSMALL_FONT);
  wxdesclistNode *dnode = descs.GetFirst();
  if (dnode) {
    int bw = 0, bh = 0;
    // first, calculate their sizes
    while (dnode) {
      stardesc *desc = dnode->GetData();
      if (!desc->prepped) {
	desc->siz = CalcBox(dc, desc->desc);
	desc->prepped = TRUE;
      }
      bw += desc->siz.x;
      if (desc->siz.y > bh) bh = desc->siz.y;
      dnode = dnode->GetNext();
    }

    // calculate appropriate position
    wxPoint doff;
    doff.x = descpt.x - (bw / 2);
    if (doff.x + bw > siz.x) doff.x = siz.x - bw;
    if (doff.x < 0) doff.x = 0;
    if (descpt.y > my) doff.y = descpt.y - bh;
    else doff.y = descpt.y;

    // place and draw boxes
    dnode = descs.GetFirst();
    while (dnode) {
      stardesc *desc = dnode->GetData();
      desc->pos = doff;
      doff.x += desc->siz.x;
      ShowBox(dc, desc->desc, desc->pos);
      dnode = dnode->GetNext();
    }
  }

  frame->SetStatusText("Ready.", 0);
  ready = TRUE;
}

void StarCanvas::DoRedraw(void)
{
  if (menu_bar->IsChecked(APP_FLICKER)) {
    // the fast way, with flicker
    wxClientDC dc(this);

    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    DoPaint(dc);
  } else {
    // the flicker-free slow way: paint to memory bitmap first
    wxSize siz(GetClientSize());
    wxBitmap surf(siz.GetX(), siz.GetY());
    wxMemoryDC dc;
    dc.SelectObject(surf);

    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    DoPaint(dc);

    // then copy to window
    // (someone please implement MIT-SHM in wxGTK?)
    wxClientDC pdc(this);
    pdc.Blit(0, 0, siz.GetX(), siz.GetY(), &dc, 0, 0, wxCOPY, FALSE);
  }
  need_redraw = FALSE;
}

void StarCanvas::Redraw(bool clr_desc)
{
  if (clr_desc) ClearDescs();
  need_redraw = TRUE;
}

void StarCanvas::CreateDescs(void)
{
  ClearDescs();
  wxstarlistNode *node = select.GetFirst();
  while (node) {
    stardata *star = node->GetData();
    stardesc *desc = new stardesc(star, refpos);
    descs.Append(desc);
    node = node->GetNext();
  }
}

void StarCanvas::ClearDescs(void)
{
  wxdesclistNode *node = descs.GetFirst();
  while (node) {
    stardesc *desc = node->GetData();
    delete desc;
    delete node;
    node = descs.GetFirst();
  }
}

wxSize StarCanvas::CalcBox(wxDC& dc, wxString txt, int *tabpos)
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
    dc.GetTextExtent(wxString(dat, len), &w, &h);
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

void StarCanvas::ShowBox(wxDC& dc, wxString txt, wxPoint pos)
{
  int tabpos[2], cp = 0;
  wxSize siz = CalcBox(dc, txt, tabpos);
  const wxChar *dat = txt.wx_str();
  const wxChar *next;
  wxCoord ch = 0, h;
  size_t len;

  dc.SetBrush(*wxBLACK_BRUSH);
  dc.SetPen(*wxWHITE_PEN);
  dc.DrawRectangle(pos.x, pos.y, siz.x, siz.y);
  pos.x += 2;
  pos.y += 2;

  dc.SetBackgroundMode(wxTRANSPARENT);
  dc.SetTextForeground(*wxGREEN);

  do {
    next = wxStrpbrk(dat, wxT("\t\n"));
    len = next ? (next - dat) : wxStrlen(dat);
    dc.DrawText(wxString(dat, len), pos.x + tabpos[cp], pos.y);
    dc.GetTextExtent(wxString(dat, len), NULL, &h);
    if (h > ch) ch = h;
    if ((!next) || (*next == '\n')) {
      pos.y += ch;
      ch = 0;
      cp = 0;
    } else cp++;
    dat = next ? (next+1) : (const wxChar *)NULL;
  } while (dat && *dat);
}
