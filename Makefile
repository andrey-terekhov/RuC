CC=gcc
CFLAGS=-c -Wall -DROBOTS
LDFLAGS=
LIBS=-lm -lpthread
INC=-Iutil

CXX=g++
CXXFLAGS=-c -std=c++11 
CXXLDFLAGS=

ifeq ($(DEBUG), y)
    DBFLAGS=-g -O0
else
    DBFLAGS=
endif

BINDIR=bin
OBJDIR=obj

UTIL_SRCDIR=util
UTIL_OBJDIR=$(OBJDIR)/utilobj
UTIL_SOURCES=$(wildcard $(UTIL_SRCDIR)/*.c)
UTIL_OBJECTS=$(patsubst $(UTIL_SRCDIR)/%.c,$(UTIL_OBJDIR)/%.o,$(UTIL_SOURCES))

COMPILER_SRCDIR=RuC
COMPILER_OBJDIR=$(OBJDIR)/rucobj
COMPILER_SOURCES=$(wildcard $(COMPILER_SRCDIR)/*.c)
COMPILER_OBJECTS=$(patsubst $(COMPILER_SRCDIR)/%.c,$(COMPILER_OBJDIR)/%.o,$(COMPILER_SOURCES))
RUC_COMPILER=ruc

VM_SRCDIR=RuCVM
VM_OBJDIR=$(OBJDIR)/vmobj
VM_SOURCES=$(wildcard $(VM_SRCDIR)/*.c)
VM_OBJECTS=$(patsubst $(VM_SRCDIR)/%.c,$(VM_OBJDIR)/%.o,$(VM_SOURCES))
RUC_VM=rucvm

FLTK_DIR=thirdparty/fltk
FLTK_CFLAGS=$(shell $(FLTK_DIR)/fltk-config --cxxflags)
FLTK_LDFLAGS=$(shell $(FLTK_DIR)/fltk-config --ldflags)
CXXFLAGS+=$(FLTK_CFLAGS)
CXXLDFLAGS+=$(FLTK_LDFLAGS)

UI_SRCDIR=UI
UI_OBJDIR=$(OBJDIR)/uiobj
UI_SOURCES=$(wildcard $(UI_SRCDIR)/*.cpp)
UI_OBJECTS=$(patsubst $(UI_SRCDIR)/%.cpp,$(UI_OBJDIR)/%.o,$(UI_SOURCES))
UI=face

all: $(RUC_COMPILER) $(RUC_VM) $(UI)

$(RUC_COMPILER): $(COMPILER_OBJECTS) $(UTIL_OBJECTS)
	mkdir -p $(BINDIR)
	$(CC) $(DBFLAGS) $(LDFLAGS) $(COMPILER_OBJECTS) $(UTIL_OBJECTS) -o $(BINDIR)/$@ $(LIBS)

$(RUC_VM): $(VM_OBJECTS) $(UTIL_OBJECTS)
	mkdir -p $(BINDIR)
	$(CC) $(DBFLAGS) $(LDFLAGS) $(VM_OBJECTS) $(UTIL_OBJECTS) -o $(BINDIR)/$@ $(LIBS)

$(UI): fltk $(UI_OBJECTS)
	mkdir -p $(BINDIR)
	$(CXX) $(DBFLAGS) $(UI_OBJECTS) $(CXXLDFLAGS) -o $(BINDIR)/$@

$(COMPILER_OBJECTS): $(COMPILER_OBJDIR)/%.o: $(COMPILER_SRCDIR)/%.c
	mkdir -p $(COMPILER_OBJDIR)
	$(CC) $(DBFLAGS) $(INC) $(CFLAGS) -o $@ $^

$(UTIL_OBJECTS): $(UTIL_OBJDIR)/%.o: $(UTIL_SRCDIR)/%.c
	mkdir -p $(UTIL_OBJDIR)
	$(CC) $(DBFLAGS) $(INC) $(CFLAGS) -o $@ $^

$(VM_OBJECTS): $(VM_OBJDIR)/%.o: $(VM_SRCDIR)/%.c
	mkdir -p $(VM_OBJDIR)
	$(CC) $(DBFLAGS) $(INC) $(CFLAGS) -o $@ $^

$(UI_OBJECTS): $(UI_OBJDIR)/%.o: $(UI_SRCDIR)/%.cpp
	mkdir -p $(UI_OBJDIR)
	$(CXX) $(DBFLAGS) $(CXXFLAGS) -o $@ $^

fltk:
	cd $(FLTK_DIR) && $(MAKE)

clear:
	rm -r $(BINDIR) $(OBJDIR)
