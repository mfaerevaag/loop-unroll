#include "llvm/PassSupport.h"

#include "LoopUnrollPass.h"

using namespace llvm;

static RegisterPass<LoopUnroll> X("LoopUnroll", "My loop unrolling", false, false);
