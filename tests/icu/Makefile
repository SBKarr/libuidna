
AR ?= ar
AR_FLAGS ?= rcs
MKDIR ?= mkdir -p
RM ?= rm -f
CP ?= cp -f

PREFIX ?= /usr/local

LIBNAME ?= libuidna.a

UTS46TEST := $(OUTDIR)/uts46test

CFLAGS += -fPIC -std=c++17 -g
CFLAGS_OPTIMIZE ?= -O2

OBJS := $(OUTDIR)/uts46test.o $(OUTDIR)/uts46test_unistr.o

$(UTS46TEST): $(OBJS)
	$(CXX) $(CFLAGS) $(CFLAGS_OPTIMIZE) $(OBJS) -I../../include -I../../src  -L$(OUTDIR) -l:$(LIBNAME).a $(WITH_ICU) -o $@

$(OUTDIR)/uts46test.o: uts46test.cpp
	$(CXX) $(CFLAGS) $(CFLAGS_OPTIMIZE) -I../../include -I../../src -c -o $@ $^

$(OUTDIR)/uts46test_unistr.o: uts46test_unistr.cpp
	$(CXX) $(CFLAGS) $(CFLAGS_OPTIMIZE) -I../../include -I../../src -c -o $@ $^
