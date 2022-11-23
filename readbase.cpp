#include "readbase.h"

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <wx/log.h>

const Transform ReadBase::B1950(
    // galactic core: RA 17h 42m 4s, DE -28d 55m
    Vector::dir(Angle((17*3600 + 42*60 + 4) * M_PI / (12*3600)),
                Angle(-(28*60 + 55) * M_PI / (180*60))),
    // galactic north pole: RA 12h 49m, DE 27d 24m
    Vector::dir(Angle((12*60 + 49) * M_PI / (12*60)),
                Angle(-(27*60 + 24) * M_PI / (180*60))));

const Transform ReadBase::J2000(
    // galactic core: RA 17h 45.6m, DE -28d 56.3m
    Vector::dir(Angle((17*60 + 45.6) * M_PI / (12*60)),
                Angle(-(28*60 + 56.3) * M_PI / (180*60))),
    // galactic north pole: RA 12h 51.4m, DE 27d 7.7m
    Vector::dir(Angle((12*60 + 51.4) * M_PI / (12*60)),
                Angle(-(27*60 + 7.7) * M_PI / (180*60))));

bool ReadBase::OpenStream(boost::iostreams::filtering_istream& stream, const wxFileName& name) {
  // Open uncompressed file, if any.
  if (name.FileExists()) {
    stream.push(boost::iostreams::file_descriptor_source(name.GetFullPath().ToStdString()));
    return stream.good();
  }

  // Otherwise, see if there's a gzip-compressed file.
  wxFileName gzname(name.GetFullPath() + wxT(".gz"));
  if (gzname.FileExists()) {
    stream.push(boost::iostreams::gzip_decompressor());
    stream.push(boost::iostreams::file_descriptor_source(gzname.GetFullPath().ToStdString()));
    return stream.good();
  }

  // No file found...
  return false;
}

void ReadBase::ReadRA(WorkData& data, const std::string& h, const std::string& m, const std::string& s) {
  data.ra = (std::stoul(h)*3600 + std::stoul(m)*60 + std::stod(s)) * M_PI / (12*3600);
}

void ReadBase::ReadDE(WorkData& data, char sg, const std::string& d, const std::string& m, const std::string& s) {
  data.de = (std::stoul(d)*3600 + std::stoul(m)*60 + std::stod(s)) * M_PI / (180*3600);
  if (sg == '-') data.de = -data.de;
}

void ReadBase::ReadDE(WorkData& data, char sg, const std::string& d, const std::string& m) {
  data.de = (std::stoul(d)*60 + std::stod(m)) * M_PI / (180*60);
  if (sg == '-') data.de = -data.de;
}

void ReadBase::ReadSpectralType(StarData& data, const std::string& type) {
  data.spectral_type = type;
  data.spectral_type.Trim(true);
  data.spectral_type.Trim(false);
}

bool ReadBase::ReadComponents(StarData& data, const std::string& comps) {
  data.components = comps;
  data.components.Trim(true);
  return !data.components.IsEmpty();
}

bool ReadBase::ReadDurchmusterung(StarData& data, const std::string& cat,
                                  const std::string& dec, const std::string& num) {
  wxString n = num;
  n.Trim(true);
  n.Trim(false);
  if (n.IsEmpty()) return false;

  wxString d = dec;
  // note that d[0] is the sign.
  for (size_t x = 1; x < d.Length(); x++)
  {
    if (d[x] == wxT(' ')) d[x] = wxT('0');
  }
  d += wxT('\u00b0');

  wxString c = cat;
  if (c[0] == wxT(' ')) {
    c = wxT("BD");
  }

  data.AddName(c + d + n, PRI_DM);
  return true;
}

bool ReadBase::ReadDurchmusterung(StarData& data, const std::string& cat,
                                  const std::string& id) {
  return ReadDurchmusterung(data, cat, id.substr(0, 3), id.substr(4));
}

bool ReadBase::ReadDurchmusterung(StarData& data, const std::string& id) {
  return ReadDurchmusterung(data, id.substr(0, 2), id.substr(2, 3), id.substr(6));
}

bool ReadBase::ReadGiclas(StarData& data, const std::string& id) {
  if (id[0] != 'G') return false;
  unsigned id1 = std::stoul(id.substr(1, 3));
  unsigned id2 = std::stoul(id.substr(5, 3));
  data.AddName(wxString::Format("G %u-%u", id1, id2), PRI_Giclas);
  return true;
}

bool ReadBase::ReadOtherName(StarData& data, const wxString& pfx, const std::string& name, int priority) {
  wxString n = name;
  n.Trim(true);
  n.Trim(false);
  if (n.IsEmpty()) return false;
  data.AddName(pfx + n, priority);
  return true;
}

bool ReadBase::LookupConstellation(wxString& name, const std::string& tok) {
  static const struct {
    const char* abb;
    const char* name;
    const char* genitive;
  } constellation[]={
      {"And", "Andromeda",           "Andromedae"},
      {"Ant", "Antlia",              "Antliae"},
      {"Aps", "Apus",                "Apodis"},
      {"Aqr", "Aquarius",            "Aquarii"},
      {"Aql", "Aquila",              "Aquilae"},
      {"Ara", "Ara",                 "Arae"},
      {"Ari", "Aries",               "Arietis"},
      {"Aur", "Auriga",              "Aurigae"},
      {"Boo", "Bootes",              "Bootis"},
      {"Cae", "Caelum",              "Caeli"},
      {"Cam", "Camelopardalis",      "Camelopardalis"},
      {"Cnc", "Cancer",              "Cancri"},
      {"CVn", "Canes Venatici",      "Canum Venaticorum"},
      {"CMa", "Canis Major",         "Canis Majoris"},
      {"CMi", "Canis Minor",         "Canis Minoris"},
      {"Cap", "Capricornus",         "Capricorni"},
      {"Car", "Carina",              "Carinae"},
      {"Cas", "Cassiopeia",          "Cassiopeiae"},
      {"Cen", "Centaurus",           "Centauri"},
      {"Cep", "Cepheus",             "Cephei"},
      {"Cet", "Cetus",               "Ceti"},
      {"Cha", "Chamaeleon",          "Chamaeleontis"},
      {"Cir", "Circinus",            "Circini"},
      {"Col", "Columba",             "Columbae"},
      {"Com", "Coma Berenices",      "Comae Berenices"},
      {"CrA", "Corona Australis",    "Coronae Australis"},
      {"CrB", "Corona Borealis",     "Coronae Borealis"},
      {"Crv", "Corvus",              "Corvi"},
      {"Crt", "Crater",              "Crateris"},
      {"Cru", "Crux",                "Crucis"},
      {"Cyg", "Cygnus",              "Cygni"},
      {"Del", "Delphinus",           "Delphini"},
      {"Dor", "Dorado",              "Doradus"},
      {"Dra", "Draco",               "Draconis"},
      {"Equ", "Equuleus",            "Equulei"},
      {"Eri", "Eridanus",            "Eridani"},
      {"For", "Fornax",              "Fornacis"},
      {"Gem", "Gemini",              "Geminorum"},
      {"Gru", "Grus",                "Gruis"},
      {"Her", "Hercules",            "Herculis"},
      {"Hor", "Horologium",          "Horologii"},
      {"Hya", "Hydra",               "Hydrae"},
      {"Hyi", "Hydrus",              "Hydri"},
      {"Ind", "Indus",               "Indi"},
      {"Lac", "Lacerta",             "Lacertae"},
      {"Leo", "Leo",                 "Leonis"},
      {"LMi", "Leo Minor",           "Leonis Minoris"},
      {"Lep", "Lepus",               "Leporis"},
      {"Lib", "Libra",               "Librae"},
      {"Lup", "Lupus",               "Lupi"},
      {"Lyn", "Lynx",                "Lyncis"},
      {"Lyr", "Lyra",                "Lyrae"},
      {"Men", "Mensa",               "Mensae"},
      {"Mic", "Microscopium",        "Microscopii"},
      {"Mon", "Monoceros",           "Monocerotis"},
      {"Mus", "Musca",               "Muscae"},
      {"Nor", "Norma",               "Normae"},
      {"Oct", "Octans",              "Octantis"},
      {"Oph", "Ophiuchus",           "Ophiuchi"},
      {"Ori", "Orion",               "Orionis"},
      {"Pav", "Pavo",                "Pavonis"},
      {"Peg", "Pegasus",             "Pegasi"},
      {"Per", "Perseus",             "Persei"},
      {"Phe", "Phoenix",             "Phoenicis"},
      {"Pic", "Pictor",              "Pictoris"},
      {"Psc", "Pisces",              "Piscium"},
      {"PsA", "Piscis Austrinius",   "Piscis Austrini"},
      {"Pup", "Puppis",              "Puppis"},
      {"Pyx", "Pyxis",               "Pyxidis"},
      {"Ret", "Reticulum",           "Reticuli"},
      {"Sge", "Sagitta",             "Sagittae"},
      {"Sgr", "Sagittarius",         "Sagittarii"},
      {"Sco", "Scorpius",            "Scorpii"},
      {"Scl", "Sculptor",            "Sculptoris"},
      {"Sct", "Scutum",              "Scuti"},
      {"Ser", "Serpens",             "Serpentis"},
      {"Sex", "Sextans",             "Sextantis"},
      {"Tau", "Taurus",              "Tauri"},
      {"Tel", "Telescopium",         "Telescopii"},
      {"Tri", "Triangulum",          "Trianguli"},
      {"TrA", "Triangulum Australe", "Trianguli Australis"},
      {"Tuc", "Tucana",              "Tucanae"},
      {"UMa", "Ursa Major",          "Ursae Majoris"},
      {"UMi", "Ursa Minor",          "Ursae Minoris"},
      {"Vel", "Vela",                "Velorum"},
      {"Vir", "Virgo",               "Virginis"},
      {"Vol", "Volans",              "Volantis"},
      {"Vul", "Vulpecula",           "Vulpeculae"},
      {nullptr}
  };

  if (tok.length() != 3 || !isupper(tok[0])) return false;
  char lower1 = tolower(tok[1]);
  char upper1 = toupper(tok[1]);
  char lower2 = tolower(tok[2]);
  char upper2 = toupper(tok[2]);

  for (size_t n = 0; constellation[n].abb; n++) {
    const char* abb = constellation[n].abb;
    if (tok[0] == abb[0] &&
        (lower1 == abb[1] || upper1 == abb[1]) &&
        (lower2 == abb[2] || upper2 == abb[2])) {
      name = constellation[n].genitive;
      return true;
    }
  }
  return false;
}

wxString ReadBase::MakeSuperscript(const std::string& num) {
  static const wxChar digits[10] = {
      wxT('\u2070'),
      wxT('\u00b9'),
      wxT('\u00b2'),
      wxT('\u00b3'),
      wxT('\u2074'),
      wxT('\u2075'),
      wxT('\u2076'),
      wxT('\u2077'),
      wxT('\u2078'),
      wxT('\u2079')
  };

  wxString n = num;
  for (size_t x = 0; x < n.Length(); x++) {
    if (n[x] >= '0' && n[x] <= '9') {
      n[x] = digits[n[x] - '0'];
    }
  }
  return n;
}

void ReadBase::Calculate(WorkData& data, const Transform& frame, double epoch) {
  Vector dir = Vector::dir(Angle(data.ra), Angle(data.de)) * frame;

  // Position calculation
  double dist;
  if (!std::isnan(data.plx) && data.plx > 0.0) {
    dist = 1000.0 / data.plx;
    data.star->is3d = true;
    data.star->position = dir * dist;
    data.star->vmag = data.vmag - 5 * (log10(dist) - 1.0);
  } else {
    dist = 1.0;
    data.star->is3d = false;
    data.star->position = dir;
    data.star->vmag = data.vmag;
  }

  // Proper motion calculation
  data.star->epoch = epoch;
  data.star->motion = Vector::null;
  if (!std::isnan(data.pmra)) {
    data.star->motion += Vector::d_phi_s(Angle(data.ra)) * frame
                         * (data.pmra * dist);
  }
  if (!std::isnan(data.pmde)) {
    data.star->motion += Vector::d_theta(Angle(data.ra), Angle(data.de)) * frame
                         * (data.pmde * dist);
  }
  if (data.star->is3d && !std::isnan(data.rvel)) {
    // rvel is given in km/s. To convert to parsec/year, we have
    // 1 parsec = 30856775814913.673 km
    // 1 year = 31556952 s
    // so the scaling factor becomes 977812.2999621...
    data.star->motion += dir * (data.rvel / 977812.2999621);
  }

  // Color temperature calculation
  data.star->temperature = EstimateTemperature(data.star->spectral_type, data.bvmag);
  data.star->color = Color::FromTemperature(data.star->temperature);
}

double ReadBase::EstimateTemperature(const wxString& type, double bvmag) {
  typedef struct {
    wxChar cls;
    unsigned subdiv;
    double temp;
  } seq_temp;

  static seq_temp main_seq[] = {
      // https://en.wikipedia.org/wiki/O-type_main-sequence_star
      {wxT('O'), 3, 44900},
      {wxT('O'), 4, 42900},
      {wxT('O'), 5, 41400},
      {wxT('O'), 6, 39500},
      {wxT('O'), 7, 37100},
      {wxT('O'), 8, 35100},
      {wxT('O'), 9, 33300},
      // https://en.wikipedia.org/wiki/B-type_main-sequence_star
      {wxT('B'), 0, 31400},
      {wxT('B'), 1, 26000},
      {wxT('B'), 2, 20600},
      {wxT('B'), 3, 17000},
      {wxT('B'), 4, 16400},
      {wxT('B'), 5, 15700},
      {wxT('B'), 6, 14500},
      {wxT('B'), 7, 14000},
      {wxT('B'), 8, 12300},
      {wxT('B'), 9, 10700},
      // https://en.wikipedia.org/wiki/A-type_main-sequence_star
      {wxT('A'), 0, 9700},
      {wxT('A'), 1, 9300},
      {wxT('A'), 2, 8800},
      {wxT('A'), 3, 8600},
      {wxT('A'), 4, 8250},
      {wxT('A'), 5, 8100},
      {wxT('A'), 6, 7910},
      {wxT('A'), 7, 7760},
      {wxT('A'), 8, 7590},
      {wxT('A'), 9, 7400},
      // https://en.wikipedia.org/wiki/F-type_main-sequence_star
      {wxT('F'), 0, 7220},
      {wxT('F'), 1, 7020},
      {wxT('F'), 2, 6820},
      {wxT('F'), 3, 6750},
      {wxT('F'), 4, 6670},
      {wxT('F'), 5, 6550},
      {wxT('F'), 6, 6350},
      {wxT('F'), 7, 6280},
      {wxT('F'), 8, 6180},
      {wxT('F'), 9, 6050},
      // https://en.wikipedia.org/wiki/G-type_main-sequence_star
      {wxT('G'), 0, 5930},
      {wxT('G'), 1, 5860},
      {wxT('G'), 2, 5770},
      {wxT('G'), 3, 5720},
      {wxT('G'), 4, 5680},
      {wxT('G'), 5, 5660},
      {wxT('G'), 6, 5600},
      {wxT('G'), 7, 5550},
      {wxT('G'), 8, 5480},
      {wxT('G'), 9, 5380},
      // https://en.wikipedia.org/wiki/K-type_main-sequence_star
      {wxT('K'), 0, 5270},
      {wxT('K'), 1, 5170},
      {wxT('K'), 2, 5100},
      {wxT('K'), 3, 4830},
      {wxT('K'), 4, 4800},
      {wxT('K'), 5, 4440},
      {wxT('K'), 6, 4300},
      {wxT('K'), 7, 4100},
      {wxT('K'), 8, 3990},
      {wxT('K'), 9, 3930},
      // https://en.wikipedia.org/wiki/Red_dwarf
      {wxT('M'), 0, 3850},
      {wxT('M'), 1, 3660},
      {wxT('M'), 2, 3560},
      {wxT('M'), 3, 3430},
      {wxT('M'), 4, 3210},
      {wxT('M'), 5, 3060},
      {wxT('M'), 6, 2810},
      {wxT('M'), 7, 2680},
      {wxT('M'), 8, 2570},
      {wxT('M'), 9, 2380},
      {0}
  };
  // https://sites.uni.edu/morgans/astro/course/Notes/section2/spectraltemps.html
  static seq_temp giant[] = {
      {wxT('G'), 5, 5010},
      {wxT('G'), 8, 4870},
      {wxT('K'), 0, 4720},
      {wxT('K'), 1, 4580},
      {wxT('K'), 2, 4460},
      {wxT('K'), 3, 4210},
      {wxT('K'), 4, 4010},
      {wxT('K'), 5, 3780},
      {wxT('M'), 0, 3660},
      {wxT('M'), 1, 3600},
      {wxT('M'), 2, 3500},
      {wxT('M'), 3, 3300},
      {wxT('M'), 4, 3100},
      {wxT('M'), 5, 2950},
      {wxT('M'), 6, 2800},
      {0}
  };
  static seq_temp supergiant[] = {
      {wxT('B'), 0, 21000},
      {wxT('B'), 1, 16000},
      {wxT('B'), 2, 14000},
      {wxT('B'), 3, 12800},
      {wxT('B'), 5, 11500},
      {wxT('B'), 6, 11000},
      {wxT('B'), 7, 10500},
      {wxT('B'), 8, 10000},
      {wxT('B'), 9, 9700},
      {wxT('A'), 0, 9400},
      {wxT('A'), 1, 9100},
      {wxT('A'), 2, 8900},
      {wxT('A'), 5, 8300},
      {wxT('F'), 0, 7500},
      {wxT('F'), 2, 7200},
      {wxT('F'), 5, 6800},
      {wxT('F'), 8, 6150},
      {wxT('G'), 0, 5800},
      {wxT('G'), 2, 5500},
      {wxT('G'), 5, 5100},
      {wxT('G'), 8, 5050},
      {wxT('K'), 0, 4900},
      {wxT('K'), 1, 4700},
      {wxT('K'), 2, 4500},
      {wxT('K'), 3, 4300},
      {wxT('K'), 4, 4100},
      {wxT('K'), 5, 3750},
      {wxT('M'), 0, 3660},
      {wxT('M'), 1, 3600},
      {wxT('M'), 2, 3500},
      {wxT('M'), 3, 3300},
      {wxT('M'), 4, 3100},
      {wxT('M'), 5, 2950},
      {0}
  };
  // https://en.wikipedia.org/wiki/Carbon_star
  static seq_temp carbon_c[] = {
      {wxT('C'), 0, 4500},
      {wxT('C'), 1, 4300},
      {wxT('C'), 2, 4100},
      {wxT('C'), 3, 3900},
      {wxT('C'), 4, 3650},
      {wxT('C'), 5, 3450},
      {0}
  };
  static seq_temp carbon_r[] = {
      {wxT('R'), 0, 4300},
      {wxT('R'), 3, 3900},
      {wxT('R'), 5, 3700},
      {wxT('R'), 8, 3450},
      {0}
  };
  // https://en.wikipedia.org/wiki/Wolf-Rayet_star
  static seq_temp wr_n[] = {
      {wxT('N'), 2, 141000},
      {wxT('N'), 3, 85000},
      {wxT('N'), 4, 70000},
      {wxT('N'), 5, 60000},
      {wxT('N'), 6, 56000},
      {wxT('N'), 7, 50000},
      {wxT('N'), 8, 45000},
      {0}
  };
  static seq_temp wr_nh[] = {
      {wxT('N'), 5, 50000},
      {wxT('N'), 6, 45000},
      {wxT('N'), 7, 45000},
      {wxT('N'), 8, 40000},
      {wxT('N'), 9, 35000},
      {0}
  };
  static seq_temp wr_c[] = {
      {wxT('O'), 2, 200000},
      {wxT('C'), 4, 117000},
      {wxT('C'), 5, 83000},
      {wxT('C'), 6, 78000},
      {wxT('C'), 7, 71000},
      {wxT('C'), 8, 60000},
      {wxT('C'), 9, 44000},
      {0}
  };

  SpectralType spec(type);

  wxString clss = spec.GetClass();
  if (clss.IsEmpty()) {
    // No spectral class (probably a variable star), fall back
    return EstimateTemperatureFromBV(bvmag);
  }

  // Currently only the first letter of the class matters.
  wxChar cls = clss[0];

  bool bad_subdiv = false;
  float subdiv = spec.GetSubdivision();
  if (std::isnan(subdiv)) {
    bad_subdiv = true;
    subdiv = 5; // a mid-range number
  }

  if (cls == wxT('D')) {
    // White dwarf types conveniently encode the temperature directly,
    // no table lookup needed.
    return CombineWithTemperatureFromBV(50400.0 / subdiv, bvmag, bad_subdiv);
  }
  if (cls == wxT('S')) {
    // S-type stars have roughly same temperature as M-type.
    cls = wxT('M');
  }
  if (cls == wxT('N')) {
    // N0 maps roughly to C6.
    cls = wxT('C');
    subdiv += 6;
  }

  seq_temp* table;
  seq_temp* fallback = nullptr;
  if (cls == wxT('C')) {
    table = carbon_c;
  }
  else if (cls == wxT('R')) {
    table = carbon_r;
  }
  else if (cls == wxT('W')) {
    cls = clss[1];
    if (cls == wxT('N')) {
      const wxString& pec = spec.GetPeculiarity();
      if (!pec.IsEmpty() && pec[0] == wxT('h')) {
        table = wr_nh;
      } else {
        table = wr_n;
      }
    } else {
      table = wr_c;
    }
  }
  else if (spec.IsSupergiant()) {
    table = supergiant;
    fallback = main_seq;
  }
  else if (spec.IsGiant()) {
    table = giant;
    fallback = main_seq;
  }
  else {
    table = main_seq;
  }

  size_t n;
  while (true) {
    bool cls_found = false;
    for (n = 0; table[n].cls; n++) {
      if (table[n].cls == cls) {
        // Specific tables may start with a too high subdiv number,
        // don't set cls_found if so because we want to fall back
        // to the main sequence table in this case.
        if (table[n].subdiv <= subdiv) cls_found = true;
        if (table[n].subdiv >= subdiv) break;
      }
      else if (cls_found) break;
    }

    if (cls_found) break;

    if (!fallback) {
      if (n == 0 && table[n].cls == cls) break; // extrapolated O-class
      wxLogMessage(wxT("Unrecognized spectral class %s"), type);
      return EstimateTemperatureFromBV(bvmag);
    }

    // If class wasn't found in specific table,
    // try searching main sequence table
    table = fallback;
    fallback = nullptr;
  }

  double temp;
  if (!table[n].cls && n > 1) {
    // at end of table, extrapolate
    unsigned dist = (table[n-2].cls == cls ? 0 : 10) + table[n-1].subdiv - table[n-2].subdiv;
    temp = table[n-1].temp - (table[n-2].temp - table[n-1].temp) * (subdiv - table[n-1].subdiv) / dist;
  } else if (!table[n].cls) {
    // at end of table, but table is too short to extrapolate...
    temp = table[0].temp;
  } else if (table[n].subdiv > subdiv && n > 0) {
    // in middle of table, interpolate
    unsigned dist = (table[n].cls == cls ? 0 : 10) + table[n].subdiv - table[n-1].subdiv;
    temp = table[n-1].temp - (table[n-1].temp - table[n].temp) * (subdiv - table[n-1].subdiv) / dist;
  } else if (table[n].subdiv < subdiv && n == 0 && table[n+1].cls) {
    // at beginning of table, extrapolate
    unsigned dist = (table[n+1].cls == cls ? 0 : 10) + table[n+1].subdiv - table[n].subdiv;
    temp = table[n].temp + (table[n].temp - table[n+1].temp) * (table[n].subdiv - subdiv) / dist;
  } else {
    // found "exact" value in table
    temp = table[n].temp;
  }

  return CombineWithTemperatureFromBV(temp, bvmag, bad_subdiv);
}

double ReadBase::EstimateTemperatureFromBV(double bvmag) {
  // The Ballesteros formula. Good enough for what we need...
  return 4600.0 * (1.0/(0.92*bvmag + 1.7) + 1.0/(0.92*bvmag + 0.62));
}

double ReadBase::CombineWithTemperatureFromBV(double temp, double bvmag, bool bad_subdiv) {
  // For temperatures below 10000 (i.e. classes other than O and B),
  // using the B-V magnitude may provide an improved estimate.
  // It seems a bit unreliable above 10000, though.
  if (temp < 10000.0 && !std::isnan(bvmag)) {
    double bv_temp = EstimateTemperatureFromBV(bvmag);
    if (temp < 7500.0 || bad_subdiv) {
      // Use the BV-based estimate as-is for temperatures below 7500
      // (i.e. classes other than O, B, and A).
      temp = bv_temp;
    } else {
      // Over 7500 (i.e. in class A), transition to the BV-based estimate
      // a bit more smoothly, just in case.
      double factor = (temp - 7500.0) / 2500.0;
      temp = temp * factor + bv_temp * (1.0 - factor);
    }
  }
  return temp;
}
