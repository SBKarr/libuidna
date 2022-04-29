
AR ?= ar
AR_FLAGS ?= rcs
MKDIR ?= mkdir -p
RM ?= rm -f
CP ?= cp -f

PREFIX ?= /usr/local

IDN2TEST := $(OUTDIR)/idn2-test-lookup

CFLAGS += -fPIC -std=c11 -g
CFLAGS_OPTIMIZE ?= -O2

$(IDN2TEST): test-lookup.c
	$(CC) $(CFLAGS) $(CFLAGS_OPTIMIZE) test-lookup.c -I../../include -lunistring -L$(OUTDIR) -l:libuidna.a  -lstdc++ -o $@
