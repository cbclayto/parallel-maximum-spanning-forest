# makefile editied from 15-418 assignment 3

OUTPUTDIR := bin/

#-std=c++14
CFLAGS := -std=c++11 -fvisibility=hidden -lpthread

ifeq (,$(CONFIGURATION))
	CONFIGURATION := solver
endif

ifeq (debug,$(CONFIGURATION))
CFLAGS += -g
else
CFLAGS += -O2 -fopenmp
endif

SOURCES := src/*.cpp
HEADERS := src/*.h

TARGETBIN := msf-$(CONFIGURATION)

.SUFFIXES:
.PHONY: all clean

all: $(TARGETBIN)

$(TARGETBIN): $(SOURCES) $(HEADERS)
	$(CXX) -o $@ $(CFLAGS) $(SOURCES)

clean:
	rm -rf ./msf-$(CONFIGURATION)

