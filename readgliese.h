#ifndef STARMAP_READGLIESE_H
#define STARMAP_READGLIESE_H

#include "readbase.h"

#include <sstream>

// Importer for the Gliese Catalogue of Nearby Stars.

class ReadGliese: public ReadBase {
public:
  explicit ReadGliese(const wxString& directory);

  bool IsOk() override;
  wxString GetCatalogName() override;
  bool ReadNext(StarData& data) override;

protected:
  boost::iostreams::filtering_istream _catalog;
  unsigned nn_count;

  static bool ReadExtraName(StarData& data, const std::string& name);
  static bool LookupGreek(wxString& name, const std::string& tok);


  class RemarkReader {
  private:
    ReadGliese::StarData& _data;
    std::istringstream _stream;
    std::string _token;
    bool _end;
  public:
    RemarkReader(ReadGliese::StarData& data, const std::string& remarks);
    void NextToken(bool force = false);
    void Read();
  };
};


#endif //STARMAP_READGLIESE_H
