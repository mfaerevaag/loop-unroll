CC = clang
CFLAGS =

SDIR = src
ODIR = build
PDIR = program

TARGET = RegAlloc

PROG ?= add
PROG-OPT ?= ${PROG}-opt

export


.PHONY: all prog clean cleanprog

.SECONDARY: ${PROG}.s ${PROG-OPT}.s

all: ${ODIR}/${TARGET}


# src

${ODIR}:
	mkdir -p ${ODIR}

${ODIR}/Makefile:
	cd ${ODIR} && cmake ../${SDIR}

${ODIR}/${TARGET}: ${ODIR} ${ODIR}/Makefile
	${MAKE} --no-print-directory -C ${ODIR}


# program

# bytecode
%.ll: ${PDIR}/%.c
	${CC} -S -emit-llvm -o $@ $<

# optimize
${PROG-OPT}.ll: ${PROG}.ll ${ODIR}/${TARGET}
	opt -load ${ODIR}/lib${TARGET}.so -${TARGET} -o $@ $< > /dev/null

# assemble
%.s: %.ll
	llc -o $@ $<

# binary
%.out: %.s
	${CC} -o $@ $<

prog: ${PROG}.out ${PROG}-opt.out


# misc

clean:
	rm -rf ${ODIR}/* *.ll *.s *.out

cleanprog:
	rm -rf *.ll *.s *.out
