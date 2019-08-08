SRC_EXT   += $(wildcard ../utils/*.cc)

include ../Makefile.common

CXXFLAGS += $(shell root-config --cflags)
CXXFLAGS += -std=c++11 -Wall 

LDFLAGS += $(shell root-config --libs) 
LDLIBS += $(shell root-config --libs) 

CXXFLAGS += -I ../utils -I..
CXXFLAGS += -I$(shell root-config --incdir)
