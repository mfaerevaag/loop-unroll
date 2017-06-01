#include "llvm/PassSupport.h"

#include "LoopUnroll.h"

using namespace llvm;

static RegisterPass<LoopUnroll> X("my-loop-unroll", "My loop unroll pass", false, false);
