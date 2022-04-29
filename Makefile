
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

install-static: $(STATIC_LIB)
	@$(MKDIR) $(PREFIX)/lib
	$(CP) $(STATIC_LIB) $(PREFIX)/lib

install-shared: $(SHARED_LIB)
	@$(MKDIR) $(PREFIX)/lib
	$(CP) $(SHARED_LIB) $(PREFIX)/lib

install-include:
	@$(MKDIR) $(PREFIX)/include
	$(CP) -r include/* $(PREFIX)/include

install-pc: $(PREFIX)/lib/pkgconfig/$(LIBNAME).pc

install: install-static install-shared install-include install-pc

test: $(STATIC_LIB)
	$(MAKE) -C tests/icu OUTDIR=$(abspath $(OUTDIR))
	$(MAKE) -C tests/idn2 OUTDIR=$(abspath $(OUTDIR))

run-test: test
	chmod +x $(OUTDIR)/uts46test
	$(OUTDIR)/uts46test
	chmod +x $(OUTDIR)/idn2-test-lookup
	$(OUTDIR)/idn2-test-lookup

$(PREFIX)/lib/pkgconfig/$(LIBNAME).pc:
	@$(MKDIR) $(PREFIX)/lib/pkgconfig
	@echo 'prefix=$(PREFIX)' > $@
	@echo 'exec_prefix=$${prefix}' >> $@
	@echo 'includedir=$(PREFIX)/include' >> $@
	@echo 'libdir=$(PREFIX)/lib' >> $@
	@echo '' >> $@
	@echo 'Name: $(LIBNAME)' >> $@
	@echo 'Description: IDN Library, extracted from ICU ' >> $@
	@echo 'Version: 0.1.0' >> $@
	@echo 'Cflags: -I$${includedir}' >> $@
	@echo 'Libs: -L$${libdir} -l$(LIBNAME:lib%=%) -lstdc++' >> $@
	@echo 'Libs.private:  -L$(PREFIX)/lib' >> $@

clean:
	$(RM) $(STATIC_LIB) $(SHARED_LIB)
	$(RM) -r $(OUTDIR)

.PHONY: all clean install install-static install-shared install-include test static shared run-test
