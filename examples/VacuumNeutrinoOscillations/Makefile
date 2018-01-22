include ../../settings.mk

#PATH_SQUIDS=../..
#CXXFLAGS+=-I$(INCDIR)
#LDFLAGS+= -Wl,-rpath -Wl,$(LIBDIR) -L$(LIBDIR) -lSQuIDS

CXXFLAGS+=-I$(INCDIR)
LDFLAGS+= -Wl,-rpath -Wl,$(LIBDIR) -L$(LIBDIR) -lSQuIDS

EXE=vacuum.exe

# Compilation rules
all: $(EXE)

vacuum.o: vacuum.cpp vacuum.h
	$(CXX) -c $(CXXFLAGS) $(CFLAGS) vacuum.cpp 
main.o: main.cpp vacuum.h
	$(CXX) -c $(CXXFLAGS) $(CFLAGS) main.cpp 
$(EXE): main.o vacuum.o
	$(CXX) $(CXXFLAGS) $(CFLAGS) main.o vacuum.o $(LDFLAGS)  -o $(EXE)
clean:
	rm -f *.o *.fo *.dat $(EXE)
