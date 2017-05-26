#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
    struct RegAlloc : public FunctionPass
    {
        static char ID;
        RegAlloc() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override
        {
            if (F.getName() != MAGIC_FUNC) return false;

            // F.dump();

            for (auto& B : F) {
                // B.dump();

                // for (auto& I : B) {
                //     errs() << "Instruction: ";
                //     I.dump();
                // }
            }

            for (auto& B : F) {
                for (auto& I : B) {
                    if (auto* op = dyn_cast<BinaryOperator>(&I)) {
                        // Insert at the point where the instruction `op` appears.
                        IRBuilder<> builder(op);

                        // Make a multiply with the same operands as `op`.
                        Value* lhs = op->getOperand(0);
                        Value* rhs = op->getOperand(1);
                        Value* mul = builder.CreateMul(lhs, rhs);

                        // Everywhere the old instruction was used as an operand, use our
                        // new multiply instruction instead.
                        for (auto& U : op->uses()) {
                            User* user = U.getUser();  // A User is anything with operands.
                            user->setOperand(U.getOperandNo(), mul);
                        }

                        // We modified the code.
                        return true;
                    }
                }
            }

            return false;
        }
    };
}

// register pass
char RegAlloc::ID = 0;
static RegisterPass<RegAlloc> X("RegAlloc", "Register Allocation", false, false);
