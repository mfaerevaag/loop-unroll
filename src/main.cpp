#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

struct RegAlloc : public FunctionPass
{
    static char ID;
    RegAlloc() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override
    {
        errs() << "Func: ";
        errs().write_escaped(F.getName()) << '\n';

        for (BasicBlock &blk : F) {
            for (BasicBlock::iterator i = blk.begin(), e = blk.end(); i != e; ++i) {
                errs() << *i << "\n";
            }
        }

        return false;
    }
};

char RegAlloc::ID = 0;

// register pass
static void registerPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM)
{
    PM.add(new RegAlloc());
}
static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                                             registerPass);
