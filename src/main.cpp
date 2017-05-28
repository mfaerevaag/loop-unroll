#include "llvm/PassSupport.h"

#include "LoopUnrollPass.h"

using namespace llvm;

static RegisterPass<LoopUnrollPass> X("LoopUnrollPass", "Loop unrolling", false, false);
