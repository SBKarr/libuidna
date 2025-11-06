
AR ?= ar
AR_FLAGS ?= rcs
MKDIR ?= mkdir -p
RM ?= rm -f
CP ?= cp -f

PREFIX ?= /usr/local

IDN2TEST := $(OUTDIR)/idn2-test-lookup

LIBNAME ?= libuidna.a

CFLAGS += -fPIC -std=c11 -g
CFLAGS_OPTIMIZE ?= -O2

UIDNA_LIBCXX=stdc++

ifeq ($(WITH_ICU),)
EXTRA_LIBS=-lunistring
else
EXTRA_LIBS=
endif

$(IDN2TEST): test-lookup.c
	$(CC) $(CFLAGS) $(CFLAGS_OPTIMIZE) test-lookup.c -I../../include $(EXTRA_LIBS) -L$(OUTDIR) -l:$(LIBNAME).a -l$(UIDNA_LIBCXX) $(WITH_ICU) -o $@
