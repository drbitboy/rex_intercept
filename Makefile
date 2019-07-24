### Make CSPICE_TOP point to the top directory of the CSPICE install
CSPICE_TOP=$(HOME)/cspice

CPPFLAGS=-I$(CSPICE_TOP)/include
LDLIBS=$(CSPICE_TOP)/lib/cspice.a -lm

EXE=rex_intercept

default: $(EXE)

run: $(EXE).csv

$(EXE).csv: $(EXE)
	echo "./$(EXE) 2015-07-14T11:58:{30..00} 2015-07-14T11:55:{59..00} nh_pcnh_002_pck.tm mk.tm" | bash | tee $@
	@###
	@### E.g. assuming the PDS NH SPICE archive resides under
	@###      ../nhsp_1000/, the following command:
	@###
	@###        ln -s ../nhsp_1000/extras/mk/nh_v04.tm mk.tm
	@###
	@###      executed in the Present Working Directory (PWD) makes
	@###      the symbolic link (symlink) mk.tm effectively the same
	@###      as the Meta-Kernel nh_v04.tm in that archive.

clean:
	$(RM) $(EXE)
