#include "llvm/PassSupport.h"

#include "loop_unroll.cpp"

using namespace llvm;

static RegisterPass<RegAlloc> X("RegAlloc", "Register Allocation", false, false);
