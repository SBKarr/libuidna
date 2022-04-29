
AR ?= ar
AR_FLAGS ?= rcs
MKDIR ?= mkdir -p
RM ?= rm -f
CP ?= cp -f

PREFIX ?= /usr/local

LIBNAME ?= libuidna
OUTDIR ?= bin

STATIC_EXT ?= a
SHARED_EXT ?= so

STATIC_LIB = $(OUTDIR)/$(LIBNAME).$(STATIC_EXT)
SHARED_LIB = $(OUTDIR)/$(LIBNAME).$(SHARED_EXT)

CFLAGS += -fPIC -std=c++17
CFLAGS_OPTIMIZE ?= -O2

all: $(STATIC_LIB) $(SHARED_LIB) test

static: $(STATIC_LIB)

shared: $(SHARED_LIB)

$(OUTDIR)/$(LIBNAME).o: src/uidna.cpp
	@$(MKDIR) $(OUTDIR)
	$(CXX) $(CFLAGS) $(CFLAGS_OPTIMIZE) -Iinclude -c -o $@ $^

$(STATIC_LIB): $(OUTDIR)/$(LIBNAME).o
	@$(MKDIR) $(OUTDIR)
	$(AR) $(AR_FLAGS) $@ $^

$(SHARED_LIB): $(OUTDIR)/$(LIBNAME).o
	@$(MKDIR) $(OUTDIR)
	$(CXX) -shared $(LDFLAGS) $^ -o $@

install-static:
	@$(MKDIR) $(PREFIX)/lib
	$(CP) $(STATIC_LIB) $(PREFIX)/lib

install-shared:
	$(CP) $(SHARED_LIB) $(PREFIX)/lib

install-include:
	@$(MKDIR) $(PREFIX)/include/libuidna
	$(CP) -r include/* $(PREFIX)/include/libuidna

install: install-static install-shared install-include

test: $(STATIC_LIB)
	$(MAKE) -C tests/icu OUTDIR=$(abspath $(OUTDIR))

run-test: test
	chmod +x $(OUTDIR)/uts46test
	$(OUTDIR)/uts46test

clean:
	$(RM) $(STATIC_LIB) $(SHARED_LIB)
	$(RM) -r $(OUTDIR)

.PHONY: all clean install install-static install-shared install-include test static shared run-test
