#include "starmap.h"
#include <stdio.h>

#ifndef WX_PRECOMP
#include <wx/tokenzr.h>
#endif

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(namelist);

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(starlist);

starlist stars;
starnamemap starnames;

struct {
  const char*abb1,*abb2,*letter;
} greek[]={
  {"ALF","ALP","Alpha"},
  {"BET","B",  "Beta"},
  {"GAM",NULL, "Gamma"},
  {"DEL",NULL, "Delta"},
  {"EPS",NULL, "Epsilon"},
  {"ZET","Z",  "Zeta"},
  {"ETA",NULL, "Eta"},
  {"THE",NULL, "Theta"},
  {"IOT","I",  "Iota"},
  {"KAP",NULL, "Kappa"},
  {"LAM",NULL, "Lambda"},
  {"MU", NULL, "Mu"},
  {"NU", NULL, "Nu"},
  {"XI", "X",  "Xi"},
  {"OMI",NULL, "Omicron"},
  {"PI", "P",  "Pi"},
  {"RHO","R",  "Rho"},
  {"SIG","S",  "Sigma"},
  {"TAU","T",  "Tau"},
  {"UPS",NULL, "Upsilon"},
  {"PHI",NULL, "Phi"},
  {"CHI",NULL, "Chi"},
  {"PSI",NULL, "Psi"},
  {"OME",NULL, "Omega"},
  {NULL}
};

struct {
  const char*abb,*name,*genitive;
} constellation[]={
  {"And","Andromeda",          "Andromedae"},
  {"Ant","Antlia",             "Antliae"},
  {"Aps","Apus",               "Apodis"},
  {"Aqr","Aquarius",           "Aquarii"},
  {"Aql","Aquila",             "Aquilae"},
  {"Ara","Ara",                "Arae"},
  {"Ari","Aries",              "Arietis"},
  {"Aur","Auriga",             "Aurigae"},
  {"Boo","Bootes",             "Bootis"},
  {"Cae","Caelum",             "Caeli"},
  {"Cam","Camelopardalis",     "Camelopardalis"},
  {"Cnc","Cancer",             "Cancri"},
  {"CVn","Canes Venatici",     "Canum Venaticorum"},
  {"CMa","Canis Major",        "Canis Majoris"},
  {"CMi","Canis Minor",        "Canis Minoris"},
  {"Cap","Capricornus",        "Capricorni"},
  {"Car","Carina",             "Carinae"},
  {"Cas","Cassiopeia",         "Cassiopeiae"},
  {"Cen","Centaurus",          "Centauri"},
  {"Cep","Cepheus",            "Cephei"},
  {"Cet","Cetus",              "Ceti"},
  {"Cha","Chamaeleon",         "Chamaeleontis"},
  {"Cir","Circinus",           "Circini"},
  {"Col","Columba",            "Columbae"},
  {"Com","Coma Berenices",     "Comae Berenices"},
  {"CrA","Corona Australis",   "Coronae Australis"},
  {"CrB","Corona Borealis",    "Coronae Borealis"},
  {"Crv","Corvus",             "Corvi"},
  {"Crt","Crater",             "Crateris"},
  {"Cru","Crux",               "Crucis"},
  {"Cyg","Cygnus",             "Cygni"},
  {"Del","Delphinus",          "Delphini"},
  {"Dor","Dorado",             "Doradus"},
  {"Dra","Draco",              "Draconis"},
  {"Equ","Equuleus",           "Equulei"},
  {"Eri","Eridanus",           "Eridani"},
  {"For","Fornax",             "Fornacis"},
  {"Gem","Gemini",             "Geminorum"},
  {"Gru","Grus",               "Gruis"},
  {"Her","Hercules",           "Herculis"},
  {"Hor","Horologium",         "Horologii"},
  {"Hya","Hydra",              "Hydrae"},
  {"Hyi","Hydrus",             "Hydri"},
  {"Ind","Indus",              "Indi"},
  {"Lac","Lacerta",            "Lacertae"},
  {"Leo","Leo",                "Leonis"},
  {"LMi","Leo Minor",          "Leonis Minoris"},
  {"Lep","Lepus",              "Leporis"},
  {"Lib","Libra",              "Librae"},
  {"Lup","Lupus",              "Lupi"},
  {"Lyn","Lynx",               "Lyncis"},
  {"Lyr","Lyra",               "Lyrae"},
  {"Men","Mensa",              "Mensae"},
  {"Mic","Microscopium",       "Microscopii"},
  {"Mon","Monoceros",          "Monocerotis"},
  {"Mus","Musca",              "Muscae"},
  {"Nor","Norma",              "Normae"},
  {"Oct","Octans",             "Octantis"},
  {"Oph","Ophiuchus",          "Ophiuchi"},
  {"Ori","Orion",              "Orionis"},
  {"Pav","Pavo",               "Pavonis"},
  {"Peg","Pegasus",            "Pegasi"},
  {"Per","Perseus",            "Persei"},
  {"Phe","Phoenix",            "Phoenicis"},
  {"Pic","Pictor",             "Pictoris"},
  {"Psc","Pisces",             "Piscium"},
  {"PsA","Piscis Austrinius",  "Piscis Austrini"},
  {"Pup","Puppis",             "Puppis"},
  {"Pyx","Pyxis",              "Pyxidis"},
  {"Ret","Reticulum",          "Reticuli"},
  {"Sge","Sagitta",            "Sagittae"},
  {"Sgr","Sagittarius",        "Sagittarii"},
  {"Sco","Scorpius",           "Scorpii"},
  {"Scl","Sculptor",           "Sculptoris"},
  {"Sct","Scutum",             "Scuti"},
  {"Ser","Serpens",            "Serpentis"},
  {"Sex","Sextans",            "Sextantis"},
  {"Tau","Taurus",             "Tauri"},
  {"Tel","Telescopium",        "Telescopii"},
  {"Tri","Triangulum",         "Trianguli"},
  {"TrA","Triangulum Australe","Trianguli Australis"},
  {"Tuc","Tucana",             "Tucanae"},
  {"UMa","Ursa Major",         "Ursae Majoris"},
  {"UMi","Ursa Minor",         "Ursae Minoris"},
  {"Vel","Vela",               "Velorum"},
  {"Vir","Virgo",              "Virginis"},
  {"Vol","Volans",             "Volantis"},
  {"Vul","Vulpecula",          "Vulpeculae"},
  {NULL}
};

struct {
  const char*name;
  int priority;
} nsystem[]={
  // since I don't know much about these systems,
  // my priority ordering may appear more or less arbitrary
  // so just reprioritize as you see fit

  // don't use same priority for different systems,
  // as it might make the sorting nondeterministic.

  {"Common",         0}, // Sirius, Procyon
  {"Bayer",          1}, // Alpha Centauri
  {"Flamsteed",      2}, // 44 Iota Bootis

  {"AC",             6}, // AC+82:1111
  {"ADS",           20}, // ?
  {"BPM",           21}, // ?
  {"Durchmusterung", 5}, // DM-36°2147
  {"Giclas",         7}, // G158-027
  {"Gliese",        50}, // ?
  {"Harvard",       51}, // ?
  {"Henry Draper",  40}, // ?
  {"L",             30}, // ??
  {"LFT",           10}, // ?
  {"LHS",           11}, // ?
  {"LP",            12}, // ?
  {"LTT",           10}, // ?
  {"SAO",           45}, // ?
  {"TR",             8}, // ?
  {"UGPMF",         31}, // ?
  {"Vyssotsky",     32}, // ?
  {"Other",          4}, // ?
  {"Simple",         3}, // Ross 128, Wolf 359
  {NULL}
};
enum esystem {
  Common = 0,
  Bayer,
  Flamsteed,

  AC,
  ADS,
  BPM,
  DM,
  Giclas,
  Gliese,
  Harvard,
  HD,
  L,
  LFT,
  LHS,
  LPM,
  LTT,
  SAO,
  TR,
  UGPMF,
  Vyssotsky,
  Other,
  Simple
};

// named systems (name + number, separated by space)
struct {
  const char*abb,*name;
  esystem sys;
} pfx_naming[]={
  {"ADS",        "ADS",        ADS},
  {"BPM",        "BPM",        BPM},
  {"Feige",      "Feige",      Simple},
  {"Gl",         "Gl",         Gliese},
  {"GJ",         "GJ",         Gliese}, // Gliese/Jahreiss
  {"Hei",        "Hei",        Simple},
  {"Kui",        "Kui",        Simple},
  {"L",          "L",          L},
  {"LFT",        "LFT",        LFT},
  {"LHS",        "LHS",        LHS},    // ?
  {"LOWNE",      "LOWNE",      Simple},
  {"LP",         "LP",         LPM},
  {"LTT",        "LTT",        LTT},
  {"Rob",        "Rob",        Simple},
  {"Ross",       "Ross",       Simple},
  {"San",        "San",        Simple},
  {"SAO",        "SAO",        SAO},    // ?
  {"Stein",      "Stein",      Simple},
  {"Steph",      "Steph",      Simple},
  {"van Maanen", "van Maanen", Simple},
  {"Wo",         "GJ",         Gliese}, // Wo is deprecated, need to use GJ
  {"Wolf",       "Wolf",       Simple},
  {NULL}
};

// patterned systems
struct {
  const char*msk;
  esystem sys;
} dsg_naming[]={
  // match characters:
  //   = optional whitespace (will be removed)
  // ? = a letter
  // + = plus or minus
  // # = any number of digits except none
  // * = any character sequence (use at end of string only)
  // @ = any letter sequence (use at end of string only)
  // 0 = a digit
  // 9 = a digit or a space (spaces will be replaced by zeros)
  {"?? +#: #*",  AC},
  {"G000-000",   Giclas},
  {"GD #",       Other},
  {"L999-#",     Other},
  {"LB99999",    Other},
  {"PG*",        Other},
  {"TR999",      TR},
  {"U999",       UGPMF},
  {"V999@",      Vyssotsky},
  {"VB #",       Other},
  {"W999",       Other}, // White Dwarf (EG or Gr) number
  {"WD0000-000", Other},
  {NULL}
};

// galactic transformation:
// transformation matrices that convert from equatorial coordinates
// to galactic coordinates; these are computed from the vectors to
// the galactic core and the galactic north pole
tmatrix epoch1950(coords(angle_ra(17, 42, 4), // galactic core
			 angle_de(28, 55, '-'),
			 distance(1)),
		  coords(angle_ra(12, 49), // galactic north pole
			 angle_de(27, 24, '+'),
			 distance(1)));
tmatrix epoch2000(coords(angle_ra(17, 45.6), // galactic core
			 angle_de(28, 56.3, '-'),
			 distance(1)),
		  coords(angle_ra(12, 51.4), // galactic north pole
			 angle_de(27, 07.7, '+'),
			 distance(1)));
coords sol_pos(0, 0, SOL_Z_OFFSET);

// helper routines

bool is_greek(wxString &tok)
{
  wxString ttok(tok);
  unsigned cnt;
  int len = ttok.Find('(');
  if (len>=0) ttok.Truncate(len);
  ttok.MakeUpper();

  for (cnt=0; greek[cnt].abb1 && (ttok != greek[cnt].abb1); cnt++);
  if (greek[cnt].abb1) {
    tok = greek[cnt].letter + tok.Mid(strlen(greek[cnt].abb1));
    return TRUE;
  }
  for (cnt=0; greek[cnt].abb1 && ((!greek[cnt].abb2) || (ttok != greek[cnt].abb2)); cnt++);
  if (greek[cnt].abb2) {
    tok = greek[cnt].letter + tok.Mid(strlen(greek[cnt].abb2));
    return TRUE;
  }
  return FALSE;
}

bool is_constellation(wxString &tok)
{
  unsigned cnt;
  for (cnt=0; constellation[cnt].abb && (tok.CmpNoCase(constellation[cnt].abb) != 0); cnt++);
  if (constellation[cnt].abb) {
    tok = constellation[cnt].genitive;
    return TRUE;
  }
  return FALSE;
}

void add_name(stardata *star, esystem sys, wxString name)
{
  int priority = nsystem[sys].priority;

  star->names.Append(new starname(name, priority));
}

void add_name(stardata *star, wxString pfx, wxString name, esystem sys = Other)
{
  if (name.CmpNoCase(wxT("star")) == 0) {
    // handles cases like Barnard's star
    sys = Common;
  } else {
    unsigned cnt;
    for (cnt=0; pfx_naming[cnt].abb && (pfx != pfx_naming[cnt].abb); cnt++);
    if (pfx_naming[cnt].abb) {
      pfx = pfx_naming[cnt].name;
      sys = pfx_naming[cnt].sys;
    }
  }
  add_name(star, sys, pfx + ' ' + name);
}

bool add_desig(stardata *star, char*&input)
{
  char*isrc;
  const char*imsk;
  wxString dat;
  unsigned cnt;
  for (cnt=0; dsg_naming[cnt].msk; cnt++) {
    dat.Empty(); isrc = input; imsk = dsg_naming[cnt].msk;
    while (isrc && *isrc && *imsk) {
      switch (*imsk) {
      case ' ':
	while (*isrc == ' ') isrc++;
	break;
      case '?':
	if (isalpha(*isrc)) dat += *isrc++;
	else isrc = NULL;
	break;
      case '+':
	if (strchr("+-", *isrc)) dat += *isrc++;
	else isrc = NULL;
	break;
      case '*':
	while (*isrc && (*isrc != ' '))
	  dat += *isrc++;
	break;
      case '@':
	while (*isrc && isalpha(*isrc))
	  dat += *isrc++;
	break;
      case '#':
	if (isdigit(*isrc)) {
	  while (isdigit(*isrc))
	    dat += *isrc++;
	} else isrc = NULL;
	break;
      case '9':
	if (*isrc == ' ') {
	  dat += '0';
	  isrc++;
	  break;
	}
      case '0':
	if (isdigit(*isrc)) dat += *isrc++;
	else isrc = NULL;
	break;
      default:
	if (*isrc == *imsk) dat += *isrc++;
	else isrc = NULL;
      }

      imsk++;
    }
    if (isrc && ((!*isrc) || (*isrc == ' '))) {
      // got a match
      if (*isrc) input = isrc + 1;
      else input = NULL;
      add_name(star, dsg_naming[cnt].sys, dat);
      return TRUE;
    }
  }
  return FALSE;
}

bool add_des(stardata *star, const char*input)
{
  char*isrc = (char*)input;
  return add_desig(star, isrc);
}

void add_dm(stardata *star, const char*input)
{
  wxString name;
  unsigned cnt = 0;
  if (input[cnt] == ' ') {
    name << wxT("BD");
    cnt += 2;
  }
  while (cnt < 3) name << input[cnt++];
  while (input[cnt] == ' ') {
    name << wxT('0');
    cnt++;
  }
  while (cnt < 5) name << input[cnt++];
  name << wxT('\u00b0');
  while (input[cnt] == ' ') cnt++;
  while (input[cnt] && (input[cnt] != ' ')) name << input[cnt++];
  add_name(star, DM, name);
}

bool eat_name(wxString pfx)
{
  // van Maanen
  // move this to a table later
  if (pfx == wxT("van")) return TRUE;

  return FALSE;
}

void add_star(stardata *star)
{
  stars.Append(star);
  wxnamelistNode *nd = star->names.GetFirst();
  while (nd) {
    starname *nm = nd->GetData();
    starnames[nm->name] = star;
    nd = nd->GetNext();
  }
}

void merge_star(stardata *star)
{
  wxnamelistNode *nd = star->names.GetFirst();
  while (nd) {
    starname *nm = nd->GetData();
    starnamemap::iterator it = starnames.find(nm->name);
    if (it != starnames.end()) {
      stardata *cstar = it->second;
      // in several common naming systems, the component stars of a binary star system
      // don't necessarily have distinct names, so check the component before merging
      if (star->comp == cstar->comp) {
        // found match, merge
        cstar->merge_names(star->names);
        // delete duplicate
        cstar->merged = TRUE;
        delete star;
        return;
      }
    }
    nd = nd->GetNext();
  }
  // not found, consider it a new star
  star->merged = TRUE;
  add_star(star);
}

// importer routines

void chomp(char*buf)
{
  size_t len = strlen(buf);
  if (len && (buf[len-1] == '\n')) len--;
  if (len && (buf[len-1] == '\r')) len--;
  buf[len] = 0;
}

void pad(char*buf, unsigned size)
{
  size_t len = strlen(buf);
  while (len < size) buf[len++] = ' ';
  buf[len] = 0;
}

void trim(char*buf)
{
  size_t len = strlen(buf);
  while (len && (buf[len-1] == ' ')) len--;
  buf[len] = 0;
}

char*sskip(char*buf)
{
  while (*buf == ' ') buf++;
  return buf;
}

void read_dummy(char*&ptr, unsigned len)
{
  ptr += len;
}

void read_str(char*&ptr, unsigned len, char*buf)
{
  memcpy(buf, ptr, len);
  ptr += len;
}

void read_strz(char*&ptr, unsigned len, char*buf)
{
  read_str(ptr, len, buf);
  buf[len] = 0;
  trim(buf);
}

void read_char(char*&ptr, unsigned len, char&buf)
{
  if (len) buf = ptr[len-1];
  ptr += len;
}

void read_num(char*&ptr, unsigned len, unsigned&buf)
{
  char sbuf[16];
  read_strz(ptr, len, sbuf);
  buf = atoi(sbuf);
}

void read_num(char*&ptr, unsigned len, double&buf)
{
  char sbuf[16];
  read_strz(ptr, len, sbuf);
  if (sbuf[0])
    buf = atof(sbuf);
  else
    buf = NAN;
}

wxString read_tok(char*&ptr)
{
  char *tok = ptr;
  char *sep = ptr ? strchr(ptr, ' ') : (char *)NULL;
  if (sep) {
    ptr = sep + 1;
    return wxString(tok, sep - tok);
  } else {
    ptr = NULL;
    return wxString(tok);
  }
}

int name_order(const starname*nam1, const starname*nam2)
{
  // sooner or later wxWindows will fix this typemess, I guess,
  // but for now this is necessary
  const starname *nm1 = *(const starname **)nam1, *nm2 = *(const starname **)nam2;
  return nm1->priority - nm2->priority;
}

// overload the right version for when they do fix it
int name_order(const starname**nm1, const starname**nm2)
{
  return (*nm1)->priority - (*nm2)->priority;
}

stardata::~stardata(void)
{
  wxnamelistNode *node = names.GetFirst();
  while (node) {
    starname *name = node->GetData();
    delete name;
    delete node;
    node = names.GetFirst();
  }
}

void stardata::sort_names(void)
{
  names.Sort(name_order);
}

void stardata::merge_names(const namelist& dat)
{
  wxnamelistNode *nd2 = dat.GetFirst();
  while (nd2) {
    starname *nm2 = nd2->GetData();
    // check whether we already have the name
    wxnamelistNode *nd1 = names.GetFirst();
    while (nd1) {
      starname *nm1 = nd1->GetData();
      if (nm1->name == nm2->name) break;
      nd1 = nd1->GetNext();
    }
    if (!nd1) {
      // nope, so add it
      names.Append(new starname(nm2->name, nm2->priority));
    }
    nd2 = nd2->GetNext();
  }
  // re-sort names
  sort_names();
}

void stardata::calc_temp(void)
{
  // calculate the star's surface temperature
  // can apparently be done from bvmag if available
  // or else from spectral type

  // then it could calculate the star's color from the temperature...
}

// Gliese 3.0 loader

void read_gliese3(const char*fname)
{
  FILE*fil = fopen(fname,"r");
  char ident[9], comp[3], distrel;
  unsigned ra_h, ra_m, ra_s;
  char de_sg;
  unsigned de_d;
  double de_m, pm;
  double pma, rv;
  char rv_rem[4], sptype[13], sps;
  double ap_mag;
  double col_bv, col_ub, col_ri;
  double trgp, trgp_err;
  double plx, plx_err;
  char plx_c;
  double vis_mag;
  unsigned v_u, v_v, v_w;
  unsigned hd;
  char dnum[13], giclas[10], lhs[6], desig[6];
  char buffer[512], *ptr, *remark;

  if (!fil) {
    printf("Gliese data not available.\n");
    return;
  }

  while (!feof(fil)) {
    fgets(buffer, sizeof(buffer), fil);
    chomp(buffer); pad(buffer, sizeof(buffer)-1);
    ptr = buffer;

    read_strz(ptr, 8, ident);   // identifier (Gl, GJ, Wo, NN)
    read_strz(ptr, 2, comp);    // components (ABC...)
    read_char(ptr, 1, distrel); // distance reliability (pqsx)
    read_num(ptr, 3, ra_h);     // 1950-epoch right ascension (hours, minutes, seconds)
    read_num(ptr, 3, ra_m);
    read_num(ptr, 3, ra_s);
    read_char(ptr, 2, de_sg);   // 1950-epoch declination (sign, degrees, minutes)
    read_num(ptr, 2, de_d);
    read_num(ptr, 5, de_m);
    read_num(ptr, 7, pm);       // total proper motion (arcsec/yr)
    read_dummy(ptr, 1);         // proper motion uncertainty (:)
    read_num(ptr, 5, pma);      // proper motion angle (degrees)
    read_num(ptr, 7, rv);       // radial velocity (km/s)
    read_dummy(ptr, 1);
    read_strz(ptr, 3, rv_rem);  // RV remarks (VAR, SB?, SB)
    read_dummy(ptr, 1);
    read_strz(ptr, 12, sptype); // spectral type
    read_char(ptr, 1, sps);     // selected source for spectral type
    read_num(ptr, 6, ap_mag);   // apparent magnitude
    read_dummy(ptr, 1);         // origin of apparent magnitude
    read_dummy(ptr, 1);         // joint magnitude (J)
    read_num(ptr, 5, col_bv);   // colored magnitude
    read_dummy(ptr, 1);         // origin
    read_dummy(ptr, 1);         // joint magnitude (4)
    read_num(ptr, 5, col_ub);   // colored magnitude
    read_dummy(ptr, 1);         // origin
    read_dummy(ptr, 1);         // joint magnitude (4)
    read_num(ptr, 5, col_ri);   // colored magnitude
    read_dummy(ptr, 1);         // origin
    read_dummy(ptr, 1);         // joint magnitude (4)
    read_num(ptr, 6, trgp);     // trigonometric parallax
    read_num(ptr, 5, trgp_err); // standard error of trigonometric parallax
    read_num(ptr, 7, plx);      // resulting parallax
    read_num(ptr, 5, plx_err);  // standard error of resulting parallax
    read_char(ptr, 1, plx_c);   // parallax code (rwsop)
    read_num(ptr, 6, vis_mag);  // absolute visual magnitude
    read_dummy(ptr, 2);         // duplicated origin/joint from apparent magnitude
    read_dummy(ptr, 1);         // quality of absolute magnitude (abcdef)
    read_dummy(ptr, 1);
    read_num(ptr, 5, v_u);      // velocity (u = towards galactic core)
    read_num(ptr, 5, v_v);      // velocity (v = with galactic rotation)
    read_num(ptr, 5, v_w);      // velocity (w = to galactic north pole)
    read_dummy(ptr, 1);
    read_num(ptr, 6, hd);       // HD number
    read_dummy(ptr, 1);
    read_strz(ptr, 12, dnum);   // Durchmusterung number
    read_dummy(ptr, 1);
    read_strz(ptr, 9, giclas);  // Giclas number
    read_dummy(ptr, 1);
    read_strz(ptr, 5, lhs);     // LHS number
    read_dummy(ptr, 1);
    read_strz(ptr, 5, desig);   // Other designation (V, U, W)
    read_dummy(ptr, 1);
    trim(remark = ptr);         // Additional (LTT, LFT, Wolf, Ross), remarks

    // calculate data
    coords pos(angle_ra(ra_h, ra_m, ra_s),
	       angle_de(de_d, isnan(de_m)?0:de_m, de_sg),
	       isnan(plx)?dist_ps(0):dist_ps(1000, plx));
    // convert to galactic coordinates
    pos *= epoch1950;
    pos += sol_pos;

#if 0
    double x, y, z;
    pos.get(x, y, z);
    printf("Identity: %s, Comp: %2.2s, DistRel: %c\n", ident, comp, distrel);
    printf("RA: %dh %dm %ds Dec: %c %dd %.1fm, Plx: %f\n", ra_h, ra_m, ra_s, de_sg, de_d, de_m, plx);
    printf("Spectral type: %s (source %c)\n", sptype, sps);
    printf("Apparent magnitude: %f, Absolute magnitude: %f\n", ap_mag, vis_mag);
    printf("HD: %d, Durchmusterung: %s, Giclas: %s, LHS: %s\n", hd, dnum, giclas, lhs);
    printf("Other designation: %s, Remarks: %s\n", desig, remark);
    printf("Distance: %f parsec, Coordinates: %f,%f,%f\n", (double)distance(pos), x, y, z);
#endif
    // add star
    stardata *star = new stardata;
    star->vmag = vis_mag;
    star->bvmag = col_bv;
    star->ubmag = col_ub;
    star->rimag = col_ri;
    star->type = sptype;
    star->calc_temp();
    star->set_pos(pos);
    star->comp = (comp[0] >= 'A') ? (comp[0]-'A'+1) : 0;

    // add Gliese identifier
    {
      wxString pfx;
      pfx << ident[0] << ident[1];
      if ((pfx != "  ") && (pfx != "NN")) {
	wxString num;
	unsigned cnt = 2;
	while (ident[cnt] == ' ') cnt++;
	while (ident[cnt] && (ident[cnt] != ' ')) num << ident[cnt++];
	if (comp[0]) num << ' ' << comp;
	add_name(star, pfx, num, Gliese);
      }
    }
    // add HD number
    if (hd) {
      wxString name;
      name.Printf("HD %d", hd); // ?
      add_name(star, HD, name);
    }
    // add Durchmusterung number
    if (dnum[0]) {
      add_dm(star, dnum);
    }
    // add Giclas number
    if (giclas[0]) {
      add_name(star, Giclas, giclas);
    }
    // add LHS number
    if (lhs[0]) {
      add_name(star, "LHS", sskip(lhs), LHS); // ?
    }
    // add other designation
    if (desig[0]) {
      if (desig[1]) // only bother if there's a number
	if (!add_des(star, desig))
	  add_name(star, Other, desig);
    }
    // add any identification found in Remark field
    while (remark && *remark && (*remark != ' ')) {
      if (!add_desig(star, remark)) {
	wxString stok = read_tok(remark);
	if (eat_name(stok)) {
	  // eat another token (van Maanen)
	  stok << ' ' << read_tok(remark);
	}

	if (stok == "=") {
	  // ?
	}
	else if (remark && *remark && (*remark != ' ')) {
	  // must be two-token
	  wxString tok = read_tok(remark);
	  if (!stok.IsNumber()) {
	    // maybe Bayer name; is first token in our Greek alphabet?
	    is_greek(stok);
	    // is it in a constellation?
	    if (is_constellation(tok)) {
	      // yup, add it
	      add_name(star, Bayer, stok + ' ' + tok);
	    } else {
	      // nope, let addname figure it out
	      add_name(star, stok, tok);
	    }
	  } else {
	    // maybe Flamsteed name; is second token in our Greek alphabet?
	    if (is_greek(tok)) {
	      // get another token
	      stok << ' ' + tok;
	      tok = read_tok(remark);
	    }
	    // is it in a constellation?
	    if (is_constellation(tok)) {
	      // yup, add it
	      add_name(star, Flamsteed, stok + ' ' + tok);
	    } else {
	      // nope, let addname figure it out
	      add_name(star, stok, tok);
	    }
	  }
	} else {
	  // was just a single token here
	  add_name(star, Other, stok);
	  break;
	}
      }
    }
    // Gliese doesn't seem to contain common names

    // save remaining remarks
    if (remark) star->remarks = remark;
#if 0
    printf("Selected star name: %s...\n", star->name.c_str());
    printf("\n");
#endif
    star->sort_names();
    add_star(star);
  }

  fclose(fil);
}

// Bright Star Catalog loader

void read_bright(const char*cname, const char*nname)
{
  FILE*fil = fopen(cname,"r");
  FILE*nts = fopen(nname,"r");
  unsigned hrn, nhrn;
  char name[11], dnum[12];
  unsigned hd, sao, fk5;
  char ads[6], comp[3], varid[10];
  unsigned ra_h, ra_m;
  double ra_s;
  char de_sg;
  unsigned de_d, de_m, de_s;
  double vis_mag, col_bv, col_ub, col_ri;
  char sptype[21];
  double pm_ra, pm_de, plx;
  unsigned rv;
  char buffer[512], *ptr;
  char nbuffer[512],*nptr;
  bool have_bayer;

  if (!fil) {
    printf("Bright Star data not available.\n");
    if (nts) fclose(nts);
    return;
  }
  if (!nts) {
    printf("Bright Star notes not available.\n");
    fclose(fil);
    return;
  }

  nptr = NULL;
  while (!feof(fil)) {
    fgets(buffer, sizeof(buffer), fil);
    chomp(buffer); pad(buffer, sizeof(buffer)-1);
    ptr = buffer;

    read_num(ptr, 4, hrn);      // Harvard Revised Number
    read_strz(ptr, 10, name);   // Name (generally Bayer and/or Flamsteed)
    read_strz(ptr, 11, dnum);   // Durchmusterung number
    read_num(ptr, 6, hd);       // HD number
    read_num(ptr, 6, sao);      // SAO number
    read_num(ptr, 4, fk5);      // FK5 number
    read_dummy(ptr, 1);         // IR source (I)
    read_dummy(ptr, 1);         // origin of IR source
    read_dummy(ptr, 1);         // double/multiple star code (A, W, D, I, R, S)
    read_strz(ptr, 5, ads);     // Aitken's Double Star designation
    read_strz(ptr, 2, comp);    // ADS number components
    read_strz(ptr, 9, varid);   // Variable star id
    read_dummy(ptr, 2);         // 1900-epoch right ascension (hours, minutes, seconds)
    read_dummy(ptr, 2);
    read_dummy(ptr, 4);
    read_dummy(ptr, 1);         // 1900-epoch declination (sign, degrees, minutes, seconds)
    read_dummy(ptr, 2);
    read_dummy(ptr, 2);
    read_dummy(ptr, 2);
    read_num(ptr, 2, ra_h);     // 2000-epoch right ascension (hours, minutes, seconds)
    read_num(ptr, 2, ra_m);
    read_num(ptr, 4, ra_s);
    read_char(ptr, 1, de_sg);   // 2000-epoch declination (sign, degrees, minutes, seconds)
    read_num(ptr, 2, de_d);
    read_num(ptr, 2, de_m);
    read_num(ptr, 2, de_s);
    read_dummy(ptr, 6);         // galactic longitude
    read_dummy(ptr, 6);         // galactic latitude
    read_num(ptr, 5, vis_mag);  // visual magnitude
    read_dummy(ptr, 1);         // visual magnitude code (H, R)
    read_dummy(ptr, 1);         // visual magnitude uncertainty (:, ?)
    read_num(ptr, 5, col_bv);   // colored magnitude
    read_dummy(ptr, 1);         // uncertainty
    read_num(ptr, 5, col_ub);   // colored magnitude
    read_dummy(ptr, 1);         // uncertainty
    read_num(ptr, 5, col_ri);   // colored magnitude
    read_dummy(ptr, 1);         // R-I code (C, E, :, ?, D)
    read_strz(ptr, 20, sptype); // spectral type
    read_dummy(ptr, 1);         // spectral type code (e, v, t)
    read_num(ptr, 6, pm_ra);    // proper motion in RA (2000-epoch) (arcsec/yr)
    read_num(ptr, 6, pm_de);    // proper motion in DE (2000-epoch) (arcsec/yr)
    read_dummy(ptr, 1);         // parallax code
    read_num(ptr, 5, plx);      // parallax
    read_num(ptr, 4, rv);       // radial velocity (km/s)
    read_dummy(ptr, 4);         // radial velocity comments (V, V?, SB, SB1, SB2, SB3, O)
    read_dummy(ptr, 2);         // rotational velocity limit characters
    read_dummy(ptr, 3);         // rotational velocity, v sin i (km/s)
    read_dummy(ptr, 1);         // uncertainty/variability
    read_dummy(ptr, 4);         // magnitude difference
    read_dummy(ptr, 6);         // separation of components
    read_dummy(ptr, 4);         // identification of components
    read_dummy(ptr, 2);         // number of components assigned
    read_dummy(ptr, 1);         // note flag

    // calculate data
    coords pos(angle_ra(ra_h, ra_m, ra_s),
	       angle_de(de_d, de_m, de_s, de_sg),
	       dist_ps(1, plx));
    // convert to galactic coordinates
    pos *= epoch2000;
    pos += sol_pos;

#if 0
    double x, y, z;
    pos.get(x, y, z);
    printf("Identity: %d, Name: %s\n", hrn, name);
    printf("RA: %dh %dm %fs Dec: %c %dd %dm %ds, Plx: %f\n", ra_h, ra_m, ra_s, de_sg, de_d, de_m, de_s, plx);
    printf("Spectral type: %s\n", sptype);
    printf("Absolute magnitude: %f\n", vis_mag);
    printf("HD: %d, SAO: %d, FK5: %d, Durchmusterung: %s\n", hd, sao, fk5, dnum);
    printf("Distance: %f parsec, Coordinates: %f,%f,%f\n", (double)distance(pos), x, y, z);
#endif
    // add star
    stardata *star = new stardata;
    star->vmag = vis_mag;
    star->bvmag = col_bv;
    star->ubmag = col_ub;
    star->rimag = col_ri;
    star->type = sptype;
    star->calc_temp();
    star->set_pos(pos);
    // TODO: handle two-letter component names
    star->comp = (comp[0] >= 'A') ? (comp[0]-'A'+1) : 0;

    // add Harvard Revised Number
    {
      wxString name;
      name.Printf("HR %d", hrn);
      add_name(star, Harvard, name);
    }
    // add HD number
    if (hd) {
      wxString name;
      name.Printf("HD %d", hd);
      add_name(star, HD, name);
    }
    // add Durchmusterung number
    if (dnum[0]) {
      add_dm(star, dnum);
    }
    // add SAO number
    if (sao) {
      wxString name;
      name.Printf("SAO %d", sao); // ?
      add_name(star, SAO, name);
    }
    // add FK5 number
    if (fk5) {
      wxString name;
      name.Printf("FK5 %d", fk5); // ?
      add_name(star, Other, name);
    }
    // add ADS designation
    if (ads[0]) {
      add_name(star, "ADS", sskip(ads)); // ?
    }

    // check whether variable star id is a Bayer name
    have_bayer = FALSE;
    if (varid[0]) {
      char*vptr = varid;
      wxString stok = read_tok(vptr);
      if (vptr) {
	is_greek(stok);
	while (*vptr == ' ') vptr++;
	wxString tok = read_tok(ptr);
	if (is_constellation(tok)) {
	  // yup, add it
	  add_name(star, Bayer, stok + ' ' + tok);
	  have_bayer = TRUE;
	}
      }
    }

    // add general name
    if (name[0]) {
      if (strlen(name) == 10) {
	wxString num(name, 3);
	wxString stok(name + 3, 3);
	wxString tok(name + 7, 3);
	num.Trim(FALSE);
	stok.Trim();
	is_greek(stok);
	if (name[6] != ' ') {
	  stok << '(' << name[6] << ')';
	}
	if (is_constellation(tok)) {
	  if (num.IsEmpty()) {
	    // got a Bayer name
	    if (!have_bayer)
	      add_name(star, Bayer, stok + ' ' + tok);
	  } else {
	    // got a Flamsteed name
	    if (stok.IsEmpty())
	      add_name(star, Flamsteed, num + ' ' + tok);
	    else
	      add_name(star, Flamsteed, num + ' ' + stok + ' ' + tok);
	  }
	} else {
	  // not Bayer/Flamsteed
	  add_name(star, Other, name);
	}
      } else {
	// not Bayer/Flamsteed
	add_name(star, Other, name);
      }
    }

    // load any associated notes
    if (!(nptr || feof(nts))) {
      fgets(nbuffer, sizeof(nbuffer), nts);
      chomp(nbuffer); nptr = nbuffer;
      read_num(nptr, 5, nhrn);
    }
    while (nptr && (nhrn == hrn)) {
      unsigned count;
      char cat[6];
      read_num(nptr, 2, count);
      read_strz(nptr, 5, cat);
      if ((count == 1) && (strcmp(cat, "N:") == 0)) {
	// load name
	char*nxt = strchr(nptr, ';');
	if (!nxt) nxt = strchr(nptr, '.');
	if (!nxt) nxt = nptr + strlen(nptr);
	wxString name(nptr, nxt - nptr);
	// it's a common name if it's all uppercase
	if ((!name.IsEmpty()) && (name == name.Upper())) {
	  // capitalize it properly
	  name.MakeLower();
	  name.GetWritableChar(0) = toupper(name.GetChar(0));
#if 0
	  printf("Common Name: %s\n", name.c_str());
#endif
	  // then add it
	  add_name(star, Common, name);
	}
      }
      // TODO: store other notes (for the user)

      // read next line
      nptr = NULL;
      if (!feof(nts)) {
	fgets(nbuffer, sizeof(nbuffer), nts);
	chomp(nbuffer); nptr = nbuffer;
	read_num(nptr, 5, nhrn);
      }
    }

    // only add it if we actually have a position
    if (isnan(plx) || !plx) delete star;
    else {
      star->sort_names();
      merge_star(star);
    }
  }

  fclose(nts);
  fclose(fil);
}

// For consideration: Hipparcos/Tycho loader (approx 1 million stars)
// but it sounds like a bit more data than we need...
// then again, we could filter it in the loader,
// but the download would probably still be enormous (110 MB gzipped),
// and program start up time would be significant...

// if someone is crazy enough to want to load the whole thing, the
// graphics engine should be structured to divide space into sectors,
// so that only the stars within a particular sector needs to be
// processed, rather than going through every single one all the time...
