#include "LoopUnrollPass.h"

#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

bool LoopUnroll::runOnLoop(Loop *L, LPPassManager &LPM)
{
    BasicBlock *H = L->getHeader();
    Function *F = H->getParent();
    StringRef funcName = F->getName();

    errs() << "Found loop in: ";
    errs().write_escaped(F->getName()) << "\n";

    // check if magic function
    if (funcName != MAGIC_FUNC) {
        errs() << "Skipping...\n";
    }

    LoopInfo *LI = NULL; //&getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    unsigned count = 0;
    unsigned tripCount = 0;
    bool force = false;
    bool allowRuntime = false;
    bool allowExpensiveTripCount = false;
    bool preserveCondBr = false;
    bool preserveOnlyFirst = false;
    unsigned tripMultiple = 0;
    unsigned peelCount = 0;
    bool preserveLCSSA = false;


    // unroll
    if (!UnrollLoop(L, count, tripCount, force,
                    allowRuntime, allowExpensiveTripCount,
                    preserveCondBr, preserveOnlyFirst,
                    tripMultiple, peelCount,
                    LI, NULL, NULL, NULL, NULL,
                    preserveLCSSA)) {
        errs() << "Failed to unroll\n";
        return false;
    }

    errs() << "Success!\n";
    return true;
}

char LoopUnroll::ID = 0;
