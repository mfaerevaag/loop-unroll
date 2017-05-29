#include "LoopUnrollPass.h"

#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Analysis/LoopInfo.h"
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

    errs() << "Discarding...\n";
    return false;
}

char LoopUnroll::ID = 0;
