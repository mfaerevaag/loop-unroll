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
};

#endif /* LOOP_UNROLL_PASS_H */
