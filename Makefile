CXX = clang
CXXFLAGS =

SDIR = src
ODIR = build
PDIR = program

TARGET = RegAlloc

PROGRAM ?= add.c

export


.PHONY: all run clean opt

all: ${ODIR}/${TARGET}


# src

${ODIR}:
	mkdir -p ${ODIR}

${ODIR}/Makefile:
	cd ${ODIR} && cmake ../${SDIR}

${ODIR}/${TARGET}: ${ODIR} ${ODIR}/Makefile
	${MAKE} --no-print-directory -C ${ODIR}

# misc

# opt: ${PDIR}/${PROGRAM} ${ODIR}/${TARGET}
# 	opt -load ${ODIR}/lib${TARGET}.so -${TARGET} < ${PDIR}/${PROGRAM} > /dev/null

run: ${PDIR}/${PROGRAM} ${ODIR}/${TARGET}
	${CXX} ${CXXFLAGS} -Xclang -load -Xclang ${ODIR}/lib${TARGET}.so -o ${ODIR}/a.out ${PDIR}/${PROGRAM}

clean:
	rm -rf ${ODIR}/*
