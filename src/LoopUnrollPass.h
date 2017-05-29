#ifndef LOOP_UNROLL_PASS_H
#define LOOP_UNROLL_PASS_H

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>

class LoopUnroll : public llvm::LoopPass
{
 public:
    static char ID;
 LoopUnroll() : llvm::LoopPass(ID) {};
    /* virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const; */
    virtual bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM);
    /* virtual const char *getPassName() const { return "LoopUnrollPass"; }; */
};

#endif /* LOOP_UNROLL_PASS_H */
