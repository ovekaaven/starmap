#ifndef STARMAP_READBRIGHT_H
#define STARMAP_READBRIGHT_H

#include "readbase.h"

// Importer for the Yale Bright Star Catalogue.

class ReadBright: public ReadBase {
public:
  explicit ReadBright(const wxString& directory);

  bool IsOk() override;
  wxString GetCatalogName() override;
  bool ReadNext(StarData& data) override;

protected:
  boost::iostreams::filtering_istream _catalog;
  boost::iostreams::filtering_istream _notes;
  std::string _note;
  unsigned _note_hr;

  void NextNote();

  static bool LookupGreek(wxString& name, const std::string& tok);
  static bool ReadVarStarName(StarData& data, const std::string& name, bool& has_bayer);
  static bool ReadGeneralName(StarData& data, const std::string& name, bool& has_bayer);

  void ReadNotes(StarData& data);
};

#endif //STARMAP_READBRIGHT_H
