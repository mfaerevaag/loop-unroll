CC = clang
CFLAGS =

SDIR = src
ODIR = build
PDIR = program

TARGET = CompArch
PASSNAME = LoopUnroll

PROG ?= loop
PROGOPT ?= ${PROG}-opt
PROGBEST ?= ${PROG}-best
PROGFUNC = magic

export


.PHONY: all prog clean cleanprog

.SECONDARY: *.s

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
	${CC} -DMAGIC_FUNC=${PROGFUNC} -S -emit-llvm -o $@ $<

# optimize
${PROGOPT}.ll: ${PROG}.ll ${ODIR}/${TARGET}
	opt -S -load ${ODIR}/lib${TARGET}.so \
-mem2reg -simplifycfg -loops -loop-simplify -loop-rotate \
-${PASSNAME} -o $@ $< > /dev/null

# optimize
${PROGBEST}.ll: ${PROG}.ll ${ODIR}/${TARGET}
	opt -S -O3 -o $@ $< > /dev/null

# assemble
%.s: %.ll
	llc -o $@ $<

# binary
%.out: %.s
	${CC} -o $@ $<

prog: ${PROG}.out ${PROGOPT}.out ${PROGBEST}.out


# misc

clean:
	rm -rf ${ODIR}/* *.ll *.s *.out

cleanprog:
	rm -rf *.ll *.s *.out
