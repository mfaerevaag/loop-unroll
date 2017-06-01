#ifndef LOOP_UNROLL_PASS_H
#define LOOP_UNROLL_PASS_H

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>

#include "llvm/Transforms/Scalar.h"
/* #include "llvm/Constants.h" */
/* #include "llvm/Function.h" */
/* #include "llvm/Instructions.h" */
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
/* #include "llvm/Support/CFG.h" */
#include "llvm/Support/Compiler.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
/* #include "llvm/IntrinsicInst.h" */
#include <algorithm>
#include <climits>
#include <cstdio>

using namespace llvm;

class LoopUnroll : public LoopPass
{
 public:
    static char ID;
 LoopUnroll() : LoopPass(ID) {}

    /// A magic value for use with the Threshold parameter to indicate
    /// that the loop unroll should be performed regardless of how much
    /// code expansion would result.
    static const unsigned NoThreshold = UINT_MAX;

    bool runOnLoop(Loop *L, LPPassManager &LPM);
    bool unrollLoop(Loop *L, unsigned Count, unsigned Threshold, LoopInfo *LI, DominatorTree &DT, ScalarEvolution *SE);
    BasicBlock *FoldBlockIntoPredecessor(BasicBlock *BB, LoopInfo *LI);

    void getAnalysisUsage(AnalysisUsage &AU) const override {
        /* AU.addRequired<AssumptionCacheTracker>(); */
        /* AU.addRequired<TargetTransformInfoWrapperPass>(); */
        // FIXME: Loop passes are required to preserve domtree, and for now we just
        // recreate dom info if anything gets unrolled.
        getLoopAnalysisUsage(AU);
    }
};

#endif /* LOOP_UNROLL_PASS_H */
