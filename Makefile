CC = clang
CFLAGS =

SDIR = src
ODIR = build
PDIR = program

TARGET = CompArch
PASSNAME ?= my-loop-unroll
PASSARGS ?=

PROG ?= loop-static
PROGBASE ?= ${PROG}-base
PROGOPT ?= ${PROG}-opt
PROGBEST ?= ${PROG}-best
PROGFUNC ?= magic
PROGTRIP ?= 100

export


.PHONY: all prog clean cleanprog

.SECONDARY: ${PROG}.s ${PROGBASE}.s ${PROGOPT}.s ${PROGBEST}.s

all: ${ODIR}/${TARGET}


# src

${ODIR}:
	mkdir -p ${ODIR}

${ODIR}/Makefile:
	cd ${ODIR} && cmake -DMAGIC_FUNC=${PROGFUNC} ../${SDIR}

${ODIR}/${TARGET}: ${ODIR} ${ODIR}/Makefile
	${MAKE} --no-print-directory -C ${ODIR}

# program

# bytecode
%.ll: ${PDIR}/%.c
	${CC} -DMAGIC_FUNC=${PROGFUNC} -DMAGIC_TRIP=${PROGTRIP} -S -emit-llvm -o $@ $<

# base
${PROGBASE}.ll: ${PROG}.ll ${ODIR}/${TARGET}
	opt -S -mem2reg -simplifycfg -loops -loop-simplify -loop-rotate -o $@ $< > /dev/null

# optimize
${PROGOPT}.ll: ${PROGBASE}.ll ${ODIR}/${TARGET}
	opt -S -load ${ODIR}/lib${TARGET}.so -${PASSNAME} ${PASSARGS} -o $@ $< > /dev/null

# best
${PROGBEST}.ll: ${PROGBASE}.ll ${ODIR}/${TARGET}
	opt -S -loop-unroll -unroll-threshold 99999999 -o $@ $< > /dev/null

# assemble
%.s: %.ll
	llc -o $@ $<

# binary
%.out: %.s
	${CC} -o $@ $<

prog: ${PROG}.out ${PROGBASE}.out ${PROGOPT}.out ${PROGBEST}.out


# misc

clean:
	rm -rf ${ODIR}/* *.ll *.s *.out

cleanprog:
	rm -rf *.ll *.s *.out
