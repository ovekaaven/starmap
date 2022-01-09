AUTOMAKE_OPTIONS = foreign dist-tarZ
EXTRA_DIST = gliese/gliese3.dat gliese/gliese3.doc \
             bright/catalog.dat bright/notes.dat bright/ReadMe
CXXFLAGS = -g -O2 -Wall @WX_FLAGS@
bin_PROGRAMS = starmap
starmap_SOURCES = starmap.cc starmap.h import.cc
