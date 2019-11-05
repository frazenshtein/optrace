SRCDIR:=$(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))/src

GCC_VER_GTE70 := $(shell echo `g++ -dumpversion | cut -f1-2 -d.` \>= 7.0 | bc)
ifeq ($(GCC_VER_GTE70), 0)
  CLANG_VER_GTE35 := $(shell echo `clang++ -dumpversion | cut -f1-2 -d.` \>= 3.5 | bc)
  ifneq ($(CLANG_VER_GTE35), 0)
    CXX=clang++
  else ifneq (, $(shell which g++-7))
    CXX=g++-7
  else ifneq (, $(shell which g++-8))
    CXX=g++-8
  else ifneq (, $(shell which g++-9))
    CXX=g++-9
  else
    $(error Error: bad GCC version - g++-7 or clang++-3.5 at least is required)
  endif
else
  CXX=g++
endif

CFLAGS=-std=c++14 -O3 -Wall

BIN=optrace

CPPS = $(shell bash -c 'ls $(SRCDIR)/*.cpp')
HEADERS = $(shell bash -c 'ls $(SRCDIR)/*.h')
OBJECTS = $(shell bash -c 'ls $(SRCDIR)/*.cpp | tr "\\n" " " | sed s/.cpp/.cpp.o/g')

.PHONY: Makefile

optrace: $(OBJECTS)
	$(CXX) -o $(BIN) $(OBJECTS) $(CFLAGS)

%.o: $(CPPS) $(HEADERS)
	$(CXX) -c $(SRCDIR)/$(shell basename $(shell basename -s .o $@)) -o $@ $(CFLAGS)

clean:
	rm -f $(BIN) $(SRCDIR)/*.o
