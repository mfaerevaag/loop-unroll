#include "LoopUnrollPass.h"

#if defined(HAVE_LLVM_IR_CONSTANTS_H)
#include <llvm/IR/Constants.h>
#elif defined(HAVE_LLVM_CONSTANTS_H)
#include <llvm/Constants.h>
#endif
#if defined(HAVE_LLVM_IR_INSTRUCTIONS_H)
#include <llvm/IR/Instructions.h>
#elif defined(HAVE_LLVM_INSTRUCTIONS_H)
#include <llvm/Instructions.h>
#endif
#include <llvm/Transforms/Utils/Cloning.h>

void LoopUnrollPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
  llvm::LoopPass::getAnalysisUsage(AU);
  AU.addRequired<DeclareAssumePass>();
  AU.addPreserved<DeclareAssumePass>();
}

bool LoopUnrollPass::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
    errs << "Helloooooooooo\n";
}

char LoopUnrollPass::ID = 0;
