#include "llvm/PassSupport.h"

#include "LoopUnrollPass.h"

using namespace llvm;

static RegisterPass<LoopUnroll> X("LoopUnroll", "Loop unrolling", false, false);
