#include "readgliese.h"

#include <wx/log.h>

ReadGliese::ReadGliese(const wxString& directory) {
  wxFileName catalog_name(directory, wxT("gliese3.dat"));

  OpenStream(_catalog, catalog_name);

  nn_count = 3001;
}

bool ReadGliese::IsOk() {
  return _catalog.good();
}

wxString ReadGliese::GetCatalogName() {
  return wxT("Gliese star catalog");
}

bool ReadGliese::ReadNext(StarData& data) {
  while (true)
  {
    std::string line;
    if (!std::getline(_catalog, line)) {
      return false;
    }

    data.ClearLists();

    ReadComponents(data, line.substr(8, 2));

    std::string npfx = line.substr(0, 2);
    if (npfx == "  ") {
      // This case is for the Sun.
      wxString n = line.substr(2, 6);
      n.Trim(true);
      data.SetName(n, PRI_Common);
    } else if (npfx == "NN") {
      // Gliese apparently never got around to numbering these.
      // The commonly used unofficial numbering starts with 3001.
      unsigned num = nn_count++;
      data.SetName(wxString::Format(wxT("GJ %u"), num), PRI_Gliese);
    } else {
      wxString num = line.substr(2, 6);
      num.Trim(true);
      num.Trim(false);
      if (!data.components.IsEmpty()) {
        num += wxT(' ');
        num += data.components;
      }
      // Note that Wo is deprecated, we just use GJ for it too.
      wxString pfx = npfx == "Gl" ? wxT("Gl ") : wxT("GJ ");
      data.SetName(pfx + num, PRI_Gliese);
    }

    try {
      ReadOtherName(data, wxT("HD "), line.substr(146, 6), PRI_HD);
      ReadDurchmusterung(data, line.substr(153, 12));
      ReadGiclas(data, line.substr(166, 9));

      // There seems to sometimes be a spurious left-justified "6" in
      // the LHS field. Make sure to only use right-justified numbers.
      if (line.length() > 180 && line[180] != ' ') {
        ReadOtherName(data, wxT("LHS "), line.substr(176, 5), PRI_LHS);
      }
      ReadExtraName(data, line.substr(182, 5));
      RemarkReader reader(data, line.substr(188));
      reader.Read();
    } catch (std::out_of_range&) {
      // May get out_of_range if line is truncated.
    }

    WorkData work(data);

    try {
      ReadRA(work, line.substr(12, 2), line.substr(15, 2), line.substr(18, 2));
      ReadDE(work, line[21], line.substr(22, 2), line.substr(25, 4));
    } catch (std::invalid_argument&) {
      // In the Gliese catalog, the only star without coordinates is the Sun.
      work.ra = NAN;
      work.de = NAN;
    }
    try {
      double mu = std::stod(line.substr(30, 6));
      Angle theta = Angle::from_deg(std::stod(line.substr(37, 5)));
      work.pmra = mu * theta.sin();
      work.pmde = mu * theta.cos();
    } catch (std::invalid_argument&) {
      // May get invalid_argument if field is empty.
      work.pmra = NAN;
      work.pmde = NAN;
    }
    try {
      work.rvel = std::stod(line.substr(43, 6));
    } catch (std::invalid_argument&) {
      // May get invalid_argument if field is empty.
      work.rvel = NAN;
    }
    ReadSpectralType(data, line.substr(54, 12));
    work.vmag = std::stod(line.substr(67, 6));
    try {
      work.bvmag = std::stod(line.substr(75, 5));
    } catch (std::invalid_argument&) {
      // May get invalid_argument if field is empty.
      work.bvmag = NAN;
    }
    try {
      work.plx = std::stod(line.substr(108, 6));
    } catch (std::invalid_argument&) {
      // May get invalid_argument if field is empty.
      work.plx = NAN;
    }

    Calculate(work, epoch1950);

    // Special overrides for the Sun.
    if (std::isnan(work.ra)) {
      data.is3d = true;
      data.position = Vector::null;
      data.motion = Vector::null;
      data.vmag = std::stod(line.substr(121, 5));
    }

    return true;
  }
}

bool ReadGliese::ReadExtraName(ReadBase::StarData& data, const std::string& name) {
  if (name[0] == ' ') return false;

  unsigned num;
  try {
    num = std::stoul(name.substr(1, 3));
  } catch (std::invalid_argument&) {
    return false;
  }

  wxString suffix = name.substr(4);
  suffix.Trim(true);

  switch (name[0]) {
  case 'V':
    data.AddName(wxString::Format(wxT("Vys %03u%s"), num, suffix), PRI_Vyssotsky);
    return true;
  case 'U':
    data.AddName(wxString::Format(wxT("UGP %u%s"), num, suffix), PRI_UGPMF);
    return true;
  case 'W':
    data.AddName(wxString::Format(wxT("EGGR %u%s"), num, suffix), PRI_EGGR);
    return true;
  default:
    return false;
  }
}

bool ReadGliese::LookupGreek(wxString& name, const std::string& tok) {
  static const struct {
    const char* abb;
    const char* name;
  } greek[] = {
      {"ALF", "Alpha"},
      {"BET", "Beta"},
      {"GAM", "Gamma"},
      {"DEL", "Delta"},
      {"EPS", "Epsilon"},
      {"ZET", "Zeta"},
      {"ETA", "Eta"},
      {"THE", "Theta"},
      {"IOT", "Iota"},
      {"KAP", "Kappa"},
      {"LAM", "Lambda"},
      {"MU",  "Mu"},
      {"NU",  "Nu"},
      {"XI",  "Xi"},
      {"OMI", "Omicron"},
      {"PI",  "Pi"},
      {"RHO", "Rho"},
      {"SIG", "Sigma"},
      {"TAU", "Tau"},
      {"UPS", "Upsilon"},
      {"PHI", "Phi"},
      {"CHI", "Chi"},
      {"PSI", "Psi"},
      {"OME", "Omega"},
      {nullptr}
  };

  for (size_t x = 0; greek[x].abb; x++) {
    if (tok == greek[x].abb) {
      name = greek[x].name;
      return true;
    }
  }
  return false;
}

ReadGliese::RemarkReader::RemarkReader(ReadGliese::StarData& data, const std::string& remarks)
    : _data(data), _stream(remarks) {
  _end = false;
  NextToken();
}

void ReadGliese::RemarkReader::NextToken(bool force) {
  if (_end) return;
  _token.clear(); // explicit clear since it might not be overwritten if we hit eof
  if (force || isalnum(_stream.peek())) {
    _stream >> _token;
    _stream.ignore(); // eat a space
    _end = _token.empty();
  } else {
    _end = true;
  }
}

void ReadGliese::RemarkReader::Read() {
  static const struct {
    const char* desig;
    int priority;
    const char* replace;
  } simple_desig[] = {
      {"ADS", PRI_ADS},
      {"BDS", PRI_Simple}, // not in SIMBAD?
      {"BPM", PRI_BPM},
      {"BS", PRI_Harvard, "HR"},
      {"CAZ", PRI_Simple}, // not in SIMBAD?
      {"CBS", PRI_Simple},
      {"CF", PRI_Simple},
      {"CFS", PRI_Simple, "CF"},
      {"COU", PRI_Simple},
      {"CVS", PRI_Simple, "CSV"},
      {"DON", PRI_Simple},
      {"ER", PRI_Simple},
      {"FK", PRI_FK5},
      {"Feige", PRI_Simple},
      {"Fin", PRI_Simple},
      {"GD", PRI_Simple},
      {"GJ", PRI_Gliese},
      {"GH", PRI_Simple, "HG"},
      {"GR", PRI_Simple, "GR*"},
      {"Hei", PRI_Simple},
      {"HZ", PRI_Simple}, // "C1 Melotte 22" in SIMBAD
      {"Hy", PRI_Simple}, // "C1 Melotte 25" in SIMBAD
      {"Kpr", PRI_Simple, "Kui"},
      {"Kr", PRI_Simple},
      {"Kui", PRI_Simple},
      {"L", PRI_Luyten},
      {"LB", PRI_Simple},
      {"LDS", PRI_Simple},
      {"LE", PRI_Simple}, // not in SIMBAD?
      {"LFT", PRI_LFT},
      {"LOWNE", PRI_Simple},
      {"LP", PRI_LPM},
      {"LTT", PRI_LTT},
      {"MW", PRI_Simple}, // not in SIMBAD
      {"MWC", PRI_Simple}, // not in SIMBAD
      {"NSV", PRI_Simple},
      {"Oo", PRI_Simple}, // not in SIMBAD?
      {"PS", PRI_Simple},
      {"Pulk", PRI_Simple},
      {"RGO", PRI_Simple},
      {"RST", PRI_Simple},
      {"Rob", PRI_Simple},
      {"Ross", PRI_Simple},
      {"S", PRI_Simple}, // "[SLO58]" in SIMBAD
      {"SAO", PRI_SAO},
      {"SM", PRI_Simple, "Smethells"},
      {"San", PRI_Simple},
      {"Sm", PRI_Simple, "Smethells"},
      {"Stein", PRI_Simple},
      {"Steph", PRI_Simple},
      {"TR", PRI_Simple}, // not in SIMBAD?
      {"TS", PRI_Simple},
      {"U", PRI_UGPMF, "UGP"},
      {"USN", PRI_Simple, "USNO"},
      {"USNO", PRI_Simple},
      {"VA", PRI_Simple}, // "C1* Melotte 25 VA" in SIMBAD
      {"VB", PRI_Simple},
      {"VV", PRI_Simple, "VVO"},
      {"VVO", PRI_Simple},
      {"WOR", PRI_Simple, "Wor"},
      {"Wo", PRI_Gliese, "GJ"},
      {"Wor", PRI_Simple},
      {"Wolf", PRI_Simple},
      {"van Maanen", PRI_Simple},
      {nullptr}
  };

  // wxLogVerbose(wxT("Incoming remarks [%s]: %s"), _data.name.name, _stream.str());

  while (!_token.empty()) {
    size_t n;
    std::string tok = _token;
    std::string flamsteed_num;
    if (isdigit(tok[0])) {
      flamsteed_num = tok;
      NextToken();
      tok = _token;
      NextToken();
    } else {
      if (tok == "no") {
        // The catalog has a comment starting with a "no",
        // then a colon, then a name. Try to skip
        // the comment so we can parse the name.
        _stream.ignore(std::numeric_limits<std::streamsize>::max(), ':');
        NextToken(true);
        continue;
      }

      // Look for Astrographic Catalogue designations.
      if (tok.length() >= 4 && tok[0] == 'A' && tok[1] == 'C' &&
          (tok[2] == '+' || tok[2] == '-')) {
        size_t colon = tok.find(':', 3);
        if (colon == std::string::npos) {
          NextToken(true);
          tok += ':';
          tok += _token;
        }
        _data.AddName(tok, PRI_AC);
        NextToken();
        continue;
      }

      // Look for Durchmusterung designations.
      // (Also Astronomische Gesellschaft designations.)
      // Note that there's already a dedicated Durchmusterung field,
      // and any BD designations would go there. Thus, we only
      // need to check for non-BD designations here.
      if (tok.length() >= 2 &&
          ((tok[0] == 'C' && (tok[1] == 'D' || tok[1] == 'd' ||
                              tok[1] == 'P' || tok[1] == 'p')) ||
           (tok[0] == 'A' && tok[1] == 'G'))) {
        int sign = tok.length() > 2 ? tok[2] : _stream.peek();
        if (sign == '+' || sign == '-') {
          if (tok.length() == 2) {
            NextToken(true);
            tok += _token;
          }
          std::string cat(1, tok[0]);
          cat += toupper(tok[1]);
          size_t colon = tok.find(':', 3);
          if (colon != std::string::npos) {
            std::string num = tok.substr(colon + 1);
            if (num.empty()) {
              NextToken(true);
              num = _token;
            }
            if (ReadDurchmusterung(_data, cat,
                                   tok.substr(2, colon - 2),
                                   num)) {
              NextToken();
              continue;
            }
          }
        }
      }

      // Look for Giclas designations.
      if (tok.length() == 8 && tok[0] == 'G' && tok[4] == '-') {
        if (ReadGiclas(_data, tok)) {
          NextToken();
          continue;
        }
      }

      // Look for White Dwarf designations.
      if (tok.length() >= 4 && tok[0] == 'W' && tok[1] == 'D') {
        wxString name = wxString::Format(wxT("WD %s"), tok.substr(2));
        _data.AddName(name, PRI_Simple);
        NextToken();
        continue;
      }

      // Look for Furuhjelm designations.
      if (tok.length() >= 4 && tok[0] == 'F' && tok[1] == 'I' &&
          tok.find('-', 2) != std::string::npos) {
        wxString name = wxString::Format(wxT("Furuhjelm %s"), tok.substr(1));
        _data.AddName(name, PRI_Simple);
        NextToken();
        continue;
      }

      if (tok.length() >= 2 && tok[0] == 'P' && tok[1] == 'G') {
        // This token does not seem to do anything particularly useful.
        // When it's just "PG", an identifier never seems to be directly attached
        // to it in the catalog. If a WD designation is present (which may be before
        // or after), then the PG designation may get a related identifier (judging
        // by SIMBAD data), but WD is not always present, and there's no other apparent
        // way to determine the identifier. Probably best to just ignore it in this case.
        // There is one other case, when there are numbers in the same token ("PG197-4"),
        // but this doesn't match with anything in SIMBAD (which instead has "PG 0939+0762").
        // So it seems best to ignore it in this case, too.
        NextToken();
        continue;
      }

      if (tok == "LHS.") {
        // This seems to be a broken token, as it's not followed by an identifier.
        // Even if it did, it would be redundant because a dedicated LHS field exists.
        // When this token occurs, we also seem to have the broken left-justified "6"
        // in the actual LHS field. In any case, all we can do is to skip this.
        // This seems to always be the last token in the name list.
        NextToken();
        continue;
      }

      if (tok == "LP.") {
        // Another broken token. It only seems to occur once, but it's at
        // the beginning of the name list, and is followed by two spaces
        // and another name ("Rob 233"). While the only thing we can do
        // with this token is skip it, we should make sure the other name
        // can be parsed.
        NextToken(true);
        continue;
      }

      // Check for simple (system + numeric identifier) designations
      // without spaces (the catalog sometimes omits them).
      for (n = 0; simple_desig[n].desig; n++) {
        size_t len = strlen(simple_desig[n].desig);
        if (tok.length() > len && isdigit(tok[len]) &&
            simple_desig[n].desig == tok.substr(0, len)) break;
      }
      if (simple_desig[n].desig) {
        const char *desig = simple_desig[n].replace ?
                            simple_desig[n].replace : simple_desig[n].desig;
        std::string tok2 = tok.substr(strlen(simple_desig[n].desig));
        wxString name = wxString::Format(wxT("%s %s"), desig, tok2);
        _data.AddName(name, simple_desig[n].priority);
        NextToken();
        continue;
      }

      // Although the catalog generally has extra spaces to mark
      // the end of the names, it sometimes does have extra spaces
      // before a designation's numeric identifier. Since at this
      // point, we've covered all cases where we may only need one
      // token, we can now safely assume we'll need a second token
      // even if extra spaces exist.
      NextToken(true);

      if (tok == "V" && isdigit(_token[0])) {
        // Probably a variable star designation with a spurious space.
        tok += _token;
        NextToken();
      }

      if (tok == "vB") {
        // The catalog has an entry "vB 170 Hyades". We can probably
        // consider it equivalent to "Hy 170".
        std::string num = _token;
        NextToken();
        if (_token == "Hyades") {
          tok = "Hy";
        }
        _token = num;
      }

      if (tok == "van") {
        // van Maanen
        tok += ' ';
        tok += _token;
        NextToken();
      }

      // Check for simple (system + numeric identifier) designations
      // with spaces.
      if (!_token.empty() && isdigit(_token[0])) {
        for (n = 0; simple_desig[n].desig; n++) {
          if (simple_desig[n].desig == tok) break;
        }
        if (simple_desig[n].desig) {
          const char *desig = simple_desig[n].replace ?
                              simple_desig[n].replace : simple_desig[n].desig;
          wxString name = wxString::Format(wxT("%s %s"), desig, _token);
          _data.AddName(name, simple_desig[n].priority);
          NextToken();
          continue;
        }

        // Check for Durchmusterung Selected Area designations.
        if (tok == "SA") {
          // One of the entries has a colon in it instead of a hyphen.
          std::string id = _token;
          size_t colon = id.find(':');
          if (colon != std::string::npos) {
            id.replace(colon, 1, "-");
          }
          // It also has a space after the colon.
          size_t hyphen = id.find('-');
          if (hyphen + 1 == id.length()) {
            NextToken(true);
            id += _token;
          }
          wxString name = wxString::Format(wxT("SA %s"), id);
          _data.AddName(name, PRI_Other);
          NextToken();
          continue;
        }

        // Check for Guide Star Catalog designations.
        if (tok == "GSC") {
          // The catalog has only one GSC reference, and it does
          // not appear to match the SIMBAD data. Not sure what
          // to make of it, but maybe it's of some use.
          wxString name = wxString::Format(wxT("GSC %s"), _token);
          _data.AddName(name, PRI_Other);
          NextToken();
          continue;
        }

        // Check for Einstein designations.
        if (tok == "IE") {
          // In one of the catalog entries, the period is missing.
          std::string num = _token;
          if (num.length() > 4 && isdigit(num[4])) {
            num.insert(4, ".");
          }
          // Also, the 1st Einstein catalog should be called 1E, not IE.
          wxString name = wxString::Format(wxT("1E %s"), num);
          _data.AddName(name, PRI_Other);
          NextToken();
          continue;
        }

        // Check for Toulouse designations.
        if (tok == "Tou") {
          // Apparently the "23." prefix is redundant, remove it.
          std::string num = _token;
          if (num.substr(0, 3) == "23.") {
            num = num.substr(3);
          }
          wxString name = wxString::Format(wxT("Tou %s"), num);
          _data.AddName(name, PRI_Simple);
          NextToken();
          continue;
        }
      }

      if (tok.substr(tok.length() - 2) == "'s" &&
          !_token.empty() && isalpha(_token[0])) {
        // "Barnard's star", "Riepe's double"
        wxString name = wxString::Format(wxT("%s %s"), tok, _token);
        _data.AddName(name, PRI_Common);
        NextToken();
        continue;
      }
    }

    // Check for Bayer designations.
    wxString constellation, bayer_pfx;
    if (flamsteed_num.empty() || !LookupConstellation(constellation, tok)) {
      if (LookupConstellation(constellation, _token)) {
        // Found a Bayer designation
        std::string superscript;
        size_t spos = tok.find('(');
        if (spos != std::string::npos &&
            tok[tok.length()-1] == ')') {
          superscript = tok.substr(spos + 1, tok.length() - spos - 2);
          tok = tok.substr(0, spos);
        }
        if (!LookupGreek(bayer_pfx, tok)) {
          bayer_pfx = tok;
        }
        bayer_pfx += MakeSuperscript(superscript);
        bayer_pfx += wxT(' ');
        NextToken();
      } else {
        // wxLogVerbose(wxT("Unrecognized token: %s from: [%s] %s"), tok, _data.name.name, _stream.str());
        break;
      }
    }

    if (_token == "A" || _token == "B") {
      constellation += wxT(' ');
      constellation += _token;
      NextToken();
    }

    if (!bayer_pfx.IsEmpty()) {
      _data.AddName(bayer_pfx + constellation, PRI_Bayer);
    }

    // Check for Flamsteed designations.
    if (!flamsteed_num.empty()) {
      wxString flamsteed_pfx = flamsteed_num;
      flamsteed_pfx += wxT(' ');
      _data.AddName(flamsteed_pfx + constellation, PRI_Flamsteed);
    }
  }
}
