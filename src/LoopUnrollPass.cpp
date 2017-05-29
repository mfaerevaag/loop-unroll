#include "LoopUnrollPass.h"

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

bool LoopUnroll::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
    llvm::errs() << "Helloooooooooo\n";
}

char LoopUnroll::ID = 0;
