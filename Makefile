### Make CSPICE_TOP point to the top directory of the CSPICE install
CSPICE_TOP=$(HOME)/cspice

CPPFLAGS=-I$(CSPICE_TOP)/include
LDLIBS=$(CSPICE_TOP)/lib/cspice.a -lm

rex_intercept:: rex_intercept.c
