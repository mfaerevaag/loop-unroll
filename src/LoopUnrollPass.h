#ifndef LOOP_UNROLL_PASS_H
#define LOOP_UNROLL_PASS_H

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

class LoopUnroll : public LoopPass
{
 public:
    static char ID;
 LoopUnroll() : LoopPass(ID) {}

    bool runOnLoop(Loop *L, LPPassManager &LPM);

    void getAnalysisUsage(AnalysisUsage &AU) const override
    {
        getLoopAnalysisUsage(AU);
    }

 private:
    bool unrollLoop(Loop *L, unsigned Count, unsigned Threshold,
                    LoopInfo *LI, DominatorTree &DT, ScalarEvolution *SE);
    BasicBlock *FoldBlockIntoPredecessor(BasicBlock *BB, LoopInfo *LI);

    /// A magic value for use with the Threshold parameter to indicate
    /// that the loop unroll should be performed regardless of how much
    /// code expansion would result.
    static const unsigned NoThreshold = UINT_MAX;
};

#endif /* LOOP_UNROLL_PASS_H */
