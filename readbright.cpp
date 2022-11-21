#include "readbright.h"

#include <sstream>
#include <string>
#include <wx/log.h>

ReadBright::ReadBright(const wxString& directory) {
  wxFileName catalog_name(directory, wxT("catalog.dat"));
  wxFileName notes_name(directory, wxT("notes.dat"));

  if (!OpenStream(_catalog, catalog_name)) {
    return;
  }

  if (OpenStream(_notes, notes_name)) {
    NextNote();
  }
}

bool ReadBright::IsOk() {
  return _catalog.good();
}

wxString ReadBright::GetCatalogName() {
  return wxT("Yale bright star catalog");
}

bool ReadBright::ReadNext(StarData& data) {
  while (true)
  {
    std::string line;
    if (!std::getline(_catalog, line)) {
      return false;
    }

    data.ClearLists();

    unsigned hr = std::stoul(line.substr(0, 4));
    data.SetName(wxString::Format(wxT("HR %u"), hr), PRI_Harvard);

    bool has_bayer = false;
    ReadDurchmusterung(data, line.substr(14, 11));
    ReadOtherName(data, wxT("HD "), line.substr(25, 6), PRI_HD);
    ReadOtherName(data, wxT("SAO "), line.substr(31, 6), PRI_SAO);
    ReadOtherName(data, wxT("FK "), line.substr(37, 4), PRI_FK5);
    ReadOtherName(data, wxT("ADS "), line.substr(44, 5), PRI_ADS);
    ReadComponents(data, line.substr(49, 2));
    ReadVarStarName(data, line.substr(51, 9), has_bayer);
    ReadGeneralName(data, line.substr(4, 10), has_bayer);
    if (hr == _note_hr) {
      ReadNotes(data);
    }

    WorkData work(data);

    try {
      ReadRA(work, line.substr(75, 2), line.substr(77, 2), line.substr(79, 4));
      ReadDE(work, line[83], line.substr(84, 2), line.substr(86, 2), line.substr(88, 2));
    } catch (std::invalid_argument&) {
      // Stars without a position at all are of pretty limited use...
      // wxLogVerbose(wxT("Discarding star: %s"), data.name.name);
      continue;
    }
    work.vmag = std::stod(line.substr(102, 5));
    try {
      work.bvmag = std::stod(line.substr(109, 5));
    } catch (std::exception&) {
      // May get invalid_argument if field is empty.
      work.bvmag = NAN;
    }
    ReadSpectralType(data, line.substr(127, 20));
    work.pmra = std::stod(line.substr(148, 6)) * 1000.0 * cos(work.de);
    work.pmde = std::stod(line.substr(154, 6)) * 1000.0;
    try {
      work.plx = std::stod(line.substr(161, 5)) * 1000.0;
    } catch (std::exception&) {
      // May get invalid_argument if field is empty.
      // May get out_of_range if line is truncated.
      work.plx = NAN;
    }
    try {
      work.rvel = std::stod(line.substr(166, 4));
    } catch (std::exception&) {
      // same as above.
      work.rvel = NAN;
    }

    Calculate(work, epoch2000);
    return true;
  }
}

void ReadBright::NextNote() {
  if (!_notes.good()) return;
  std::getline(_notes, _note);
  try {
    _note_hr = std::stoul(_note.substr(1,4));
  } catch (std::exception&) {
    _note_hr = 0;
  }
}

bool ReadBright::LookupGreek(wxString& name, const std::string& tok) {
  static const struct {
    const char* abb;
    const char* name;
  } greek[] = {
      {"Alp", "Alpha"},
      {"Bet", "Beta"},
      {"Gam", "Gamma"},
      {"Del", "Delta"},
      {"Eps", "Epsilon"},
      {"Zet", "Zeta"},
      {"Eta", "Eta"},
      {"The", "Theta"},
      {"Iot", "Iota"},
      {"Kap", "Kappa"},
      {"Lam", "Lambda"},
      {"Mu",  "Mu"},
      {"Nu",  "Nu"},
      {"Xi",  "Xi"},
      {"Omi", "Omicron"},
      {"Pi",  "Pi"},
      {"Rho", "Rho"},
      {"Sig", "Sigma"},
      {"Tau", "Tau"},
      {"Ups", "Upsilon"},
      {"Phi", "Phi"},
      {"Chi", "Chi"},
      {"Psi", "Psi"},
      {"Ome", "Omega"},
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

bool ReadBright::ReadVarStarName(StarData& data, const std::string& name, bool& has_bayer) {
  std::istringstream stream(name);

  stream >> std::ws;
  if (stream.eof() || isdigit(stream.peek()))
  {
    // No name, or a "Catalogue of Suspected Variable Stars" number.
    // The latter is unlikely to still be useful.
    return false;
  }

  std::string tok;
  // Grab initial alphabetic letters.
  // Numbers may follow in at least two cases:
  // - we have a numeric label (V335)
  // - we have a superscripted label (Tau8).
  while (isalpha(stream.peek())) {
    tok.push_back((char)stream.get());
  }

  bool is_bayer = false;
  wxString pfx;

  if (tok == "Var") {
    // Apparently not an actual name
    return false;
  }
  else if (tok == "V" && isdigit(stream.peek())) {
    // A numeric label (V335)
    while (isdigit(stream.peek())) {
      tok.push_back((char)stream.get());
    }
    pfx = tok;
  }
  else  {
    if (LookupGreek(pfx, tok)) {
      // Greek letter, this is a Bayer name
      is_bayer = true;
    } else {
      // Latin letters
      pfx = tok;
    }

    // Check for superscripted digits
    tok.clear();
    stream >> std::ws;
    while (isdigit(stream.peek())) {
      tok.push_back((char)stream.get());
    }
    pfx += MakeSuperscript(tok);
  }

  pfx += wxT(' ');

  // Finally, look up constellation
  stream >> tok;

  wxString constellation;
  if (LookupConstellation(constellation, tok)) {
    data.AddName(pfx + constellation, is_bayer ? PRI_Bayer : PRI_Variable);
    if (is_bayer) has_bayer = true;
    return true;
  }

  wxLogMessage(wxT("Unrecognized variable star name: %s"), name);

  return false;
}

bool ReadBright::ReadGeneralName(StarData& data, const std::string& name, bool& has_bayer) {
  std::istringstream stream(name);

  stream >> std::ws;
  if (stream.eof()) {
    return false;
  }

  // Look for Flamsteed label
  std::string flamsteed_num;
  if (isdigit(stream.peek())) {
    while (isdigit(stream.peek())) {
      flamsteed_num.push_back((char)stream.get());
    }
    stream >> std::ws;
  }

  // The next token could be either a Bayer label, or a constellation.
  // The string "Del" could be either (can mean Delta or Delphini),
  // we won't know before parsing the final token. So let's just assume
  // we have a Bayer label until proven otherwise.
  std::string bayer_tok;
  while (isalpha(stream.peek())) {
    bayer_tok.push_back((char)stream.get());
  }
  stream >> std::ws;

  // Check for superscripted digits
  std::string superscript_tok;
  while (isdigit(stream.peek())) {
    superscript_tok.push_back((char)stream.get());
  }

  if (bayer_tok == "M" && !superscript_tok.empty()) {
    // The catalog contains a couple of Messier objects for some reason.
    // If we have M and a number, assume this is one of those,
    // rather than a Bayer superscript.
    bayer_tok += superscript_tok;
    data.AddName(bayer_tok, PRI_Simple);
    // The M31 entry also has the constellation (Andromeda) for some reason,
    // but Messier designations don't use that, so ignore it.
    return true;
  }

  // Look for constellation
  std::string constellation_tok;
  stream >> constellation_tok;
  if (constellation_tok.empty()) {
    // Seems we do not actually have a Bayer label.
    constellation_tok = bayer_tok;
    bayer_tok.clear();
  }

  wxString constellation;
  if (!LookupConstellation(constellation, constellation_tok)) {
    // If we can't find a constellation, this is not a Bayer/Flamsteed name.
    // The catalog does contain some nova and galaxy names for some reason,
    // so this entry must be one of those.
    wxString n = name;
    n.Trim(true);
    n.Trim(false);
    data.AddName(n, PRI_Simple);
    return true;
  }

  if (!bayer_tok.empty() && !has_bayer) {
    wxString bayer_pfx;
    if (!LookupGreek(bayer_pfx, bayer_tok)) {
      bayer_pfx = bayer_tok;
    }
    bayer_pfx += MakeSuperscript(superscript_tok);
    bayer_pfx += wxT(' ');
    data.AddName(bayer_pfx + constellation, PRI_Bayer);
    has_bayer = true;
  }

  if (!flamsteed_num.empty()) {
    wxString flamsteed_pfx = flamsteed_num;
    flamsteed_pfx += wxT(' ');
    data.AddName(flamsteed_pfx + constellation, PRI_Flamsteed);
  }

  return true;
}

void ReadBright::ReadNotes(StarData& data) {
  unsigned hr = _note_hr;
  while (_note_hr == hr) {
    unsigned count = std::stoul(_note.substr(5, 2));
    std::string cat = _note.substr(7, 4);
    std::string remark = _note.substr(12);
    if (count == 1 && cat[0] == 'N') {
      // Currently we limit ourselves to using the first listed name,
      // and only if it's all-caps. Perhaps we could do better
      // in some cases, but it'll do for now.
      size_t sep = remark.find(';');
      if (sep == std::string::npos) {
        sep = remark.find('.');
      }
      if (sep != std::string::npos) {
        std::string name = remark.substr(0, sep);
        // Check that the name is uppercase, and convert it to normal case.
        bool is_upper = true, is_first = true;
        for (size_t n = 0; n < name.length(); n++) {
          if (name[n] == ' ') {
            is_first = true;
          } else if (islower(name[n])) {
            is_upper = false;
          } else if (is_first) {
            is_first = false;
          } else {
            name[n] = (char)tolower(name[n]);
          }
        }
        if (is_upper) {
          // Found an uppercase name.
          data.AddName(name, PRI_Common);
        } else {
          sep = std::string::npos;
        }
      }
      // Store the remaining names as remarks.
      if (sep == std::string::npos) {
        data.AddRemark(remark);
      } else if (sep + 2 < remark.length()) {
        // Assume that the semicolon is followed by a space.
        data.AddRemark(remark.substr(sep + 2));
      }
    }
    else if (cat[0] == 'N') {
      data.AddRemark(remark);
    }
    NextNote();
  }
}
