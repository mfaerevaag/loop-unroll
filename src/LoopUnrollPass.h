#ifndef LOOP_UNROLL_PASS_H
#define LOOP_UNROLL_PASS_H

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>

using namespace llvm;

class LoopUnroll : public LoopPass
{
 public:
    static char ID;
 LoopUnroll() : LoopPass(ID) {};
    virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
};

#endif /* LOOP_UNROLL_PASS_H */
