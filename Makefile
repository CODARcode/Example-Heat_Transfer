CC=cc
FC=ftn
CFLAGS=-g -O3
FFLAGS=-g -O3 -Wall -fcheck=bounds #-fcheck=array-temps


## ADIOS_DIR/bin should in PATH env
ADIOS_DIR=$(shell adios_config -d)
ADIOS_FINC=$(shell adios_config -c -f)
ADIOS_FLIB=$(shell adios_config -l -f)

default: clean adios2
all: clean default

io_hdf5_a.o : io_hdf5_a.F90 
	${FC} ${FFLAGS} -c ${HDF5_FINC} $< 

io_hdf5_b.o : io_hdf5_b.F90 
	${FC} ${FFLAGS} -c ${HDF5_FINC} $< 

io_phdf5.o : io_phdf5.F90 
	${FC} ${FFLAGS} -c ${PHDF5_FINC} $< 

gwrite_heat.fh: heat_transfer.xml
	${ADIOS_DIR}/bin/gpp.py heat_transfer.xml
	rm -f gread_heat.fh

io_adios_gpp.o : gwrite_heat.fh io_adios_gpp.F90 
	${FC} ${FFLAGS} -c ${ADIOS_FINC} io_adios_gpp.F90 

io_adios.o : io_adios.F90 
	${FC} ${FFLAGS} -c ${ADIOS_FINC} $< 

io_adios_noxml.o : io_adios_noxml.F90 
	${FC} ${FFLAGS} -c ${ADIOS_FINC} $< 

%.o : %.F90 
	${FC} ${FFLAGS} -c $< 

fort: heat_vars.o io_fort.o heat_transfer.o
	${FC} ${FFLAGS} -o heat_transfer_fort $^ 

hdf5_a: heat_vars.o io_hdf5_a.o heat_transfer.o
	libtool --mode=link --tag=FC ${FC} ${FFLAGS} -o heat_transfer_hdf5_a $^ ${HDF5_FLIB} 

hdf5_b: heat_vars.o io_hdf5_b.o heat_transfer.o
	libtool --mode=link --tag=FC ${FC} ${FFLAGS} -o heat_transfer_hdf5_b $^ ${HDF5_FLIB} 

phdf5: heat_vars.o io_phdf5.o heat_transfer.o
	libtool --mode=link --tag=FC ${FC} ${FFLAGS} -o heat_transfer_phdf5 $^ ${PHDF5_FLIB} 

adios1: heat_vars.o io_adios_gpp.o heat_transfer.o
	${FC} ${FFLAGS} -o heat_transfer_adios1 $^ ${ADIOS_FLIB} 

adios2: heat_vars.o io_adios.o heat_transfer.o
	${FC} ${FFLAGS} -o heat_transfer_adios2 $^ ${ADIOS_FLIB} 
	@echo Done building

heat_transfer_strong_scaling: heat_vars.o io_adios.o heat_transfer_strong_scaling.o
	${FC} ${FFLAGS} -o $@ $^ ${ADIOS_FLIB}
	@echo Done building

noxml: heat_vars.o io_adios_noxml.o heat_transfer.o
	${FC} ${FFLAGS} -o heat_transfer_noxml $^ ${ADIOS_FLIB} 

clean:
	rm -f *.o *.mod *.fh core.*

distclean: clean
	rm -f fort.* 
	rm -f *.png minmax 
	rm -rf *.bp *.bp.dir *.idx
	rm -f *.h5
	rm -f conf

