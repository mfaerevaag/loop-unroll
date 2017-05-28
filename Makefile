CC = clang
CFLAGS =

SDIR = src
ODIR = build
PDIR = program

TARGET = CompArch

PROG ?= add
PROG-OPT ?= ${PROG}-opt
PROG-BEST ?= ${PROG}-best
PROG-FUNC = magic

export


.PHONY: all prog clean cleanprog

.SECONDARY: ${PROG}.s ${PROG-OPT}.s ${PROG-BEST}.s

all: ${ODIR}/${TARGET}


# src

${ODIR}:
	mkdir -p ${ODIR}

${ODIR}/Makefile:
	cd ${ODIR} && cmake -DMAGIC_FUNC=${PROG-FUNC} ../${SDIR}

${ODIR}/${TARGET}: ${ODIR} ${ODIR}/Makefile
	${MAKE} --no-print-directory -C ${ODIR}

# program

# bytecode
%.ll: ${PDIR}/%.c
	${CC} -DMAGIC_FUNC=${PROG-FUNC} -S -emit-llvm -o $@ $<

# optimize
${PROG-OPT}.ll: ${PROG}.ll ${ODIR}/${TARGET}
	opt -S -load ${ODIR}/lib${TARGET}.so -${TARGET} -o $@ $< > /dev/null

# optimize
${PROG-BEST}.ll: ${PROG}.ll ${ODIR}/${TARGET}
	opt -S -O3 -o $@ $< > /dev/null

# assemble
%.s: %.ll
	llc -o $@ $<

# binary
%.out: %.s
	${CC} -o $@ $<

prog: ${PROG}.out ${PROG-OPT}.out ${PROG-BEST}.out


# misc

clean:
	rm -rf ${ODIR}/* *.ll *.s *.out

cleanprog:
	rm -rf *.ll *.s *.out
