#ifndef LOOP_UNROLL_H
#define LOOP_UNROLL_H

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

using namespace llvm;

class LoopUnroll : public LoopPass
{
 public:
    static char ID;
 LoopUnroll() : LoopPass(ID) {}

    bool runOnLoop(Loop *L, LPPassManager &LPM);

    void getAnalysisUsage(AnalysisUsage &AU) const override
    {
        AU.addRequired<AssumptionCacheTracker>();
        AU.addRequired<TargetTransformInfoWrapperPass>();
        getLoopAnalysisUsage(AU);
    }
};

#endif /* LOOP_UNROLL_H */
