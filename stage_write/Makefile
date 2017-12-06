CC=cc
FC=ftn
CFLAGS=-g -O3
FFLAGS=-g -Wall -fcheck=bounds #-fcheck=array-temps

## ADIOS_DIR/bin should in PATH env
ADIOS_INC=$(shell adios_config -c)
ADIOS_CLIB=$(shell adios_config -l)

default: clean stage_write 
all: default

OBJS = utils.o decompose_block.o 

%.o: %.c
	${CC} ${CFLAGS} -c ${ADIOS_INC} $<

stage_write: ${OBJS} stage_write.o
	${CC} ${LDFLAGS} -o stage_write ${OBJS} stage_write.o ${ADIOS_CLIB}
	@echo "Done"

test_decompose: ${OBJS} test_decompose.o 
	${CC} ${LDFLAGS} -o test_decompose ${OBJS} test_decompose.o ${ADIOS_CLIB}

clean:
	rm -f *.o *.mod *.fh core.*
	rm -f stage_write


distclean: clean
	rm -f log.* dataspaces.conf conf srv.lck *.bp

