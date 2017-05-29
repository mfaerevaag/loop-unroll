#include "LoopUnrollPass.h"

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

bool LoopUnroll::runOnLoop(Loop *L, LPPassManager &LPM){
    errs() << "Helloooooooooo\n";
}

char LoopUnroll::ID = 0;
