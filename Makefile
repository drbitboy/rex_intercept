### Make CSPICE_TOP point to the top directory of the CSPICE install
CSPICE_TOP=$(HOME)/cspice

CPPFLAGS=-I$(CSPICE_TOP)/include
LDLIBS=$(CSPICE_TOP)/lib/cspice.a -lm

EXE=rex_intercept

default: $(EXE)

run: $(EXE).csv

$(EXE).csv: $(EXE)
	### E.g. ln -s ../nhsp_1000/extras/mk/nh_v04.tm mk.tm
	echo "./$(EXE) 2015-07-14T11:55:{5,4,3,2,1,0}{9,8,7,6,5,4,3,2,1,0} nh_pcnh_002_pck.tm mk.tm" | bash | tee $@

clean:
	$(RM) $(EXE)
