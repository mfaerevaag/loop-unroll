#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/OptimizationDiagnosticInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SimplifyIndVar.h"
using namespace llvm;

#include "LoopUnroll.h"

using namespace llvm;


// command line options

static cl::opt<unsigned> UnrollThreshold ("my-unroll-threshold", cl::init(0), cl::Hidden,
                                          cl::desc("The cut-off point for automatic loop unrolling"));

static cl::opt<unsigned> UnrollCount ("my-unroll-count", cl::init(0), cl::Hidden,
                                      cl::desc("Use this unroll count for all loops, for testing purposes"));


// helper functions

static unsigned estimateLoopSize(const Loop *L, AssumptionCache *AC,
                                 const TargetTransformInfo &TTI)
{
    unsigned size = 0;
    CodeMetrics Metrics;
    SmallPtrSet<const Value *, 32> EphValues;

    // get dynamic values allocated at runtime
    // (those used only by an assume or similar intrinsics in the loop)
    CodeMetrics::collectEphemeralValues(L, AC, EphValues);

    // get metrics for each block in the loop
    for (BasicBlock *BB : L->blocks()) {
        Metrics.analyzeBasicBlock(BB, TTI, EphValues);
    }

    // get total size as number of instructions
    size = Metrics.NumInsts;

    // should never be zero, as we assume at least:
    // one comparison, one branch and one iterator increment instruction
    if (size == 0) {
        size = 3;
    }

    return size;
}

// convert the instruction operands from referencing the current values into
// those specified by ValueMap.
static inline void remapInstruction(Instruction *I, ValueToValueMapTy &ValueMap)
{
    for (unsigned op = 0, E = I->getNumOperands(); op != E; ++op) {
        Value *Op = I->getOperand(op);
        ValueToValueMapTy::iterator It = ValueMap.find(Op);
        if (It != ValueMap.end())
            I->setOperand(op, It->second);
    }

    if (PHINode *PN = dyn_cast<PHINode>(I)) {
        for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
            ValueToValueMapTy::iterator It = ValueMap.find(PN->getIncomingBlock(i));
            if (It != ValueMap.end())
                PN->setIncomingBlock(i, cast<BasicBlock>(It->second));
        }
    }
}

// folds a basic block into its predecessor if it only has one predecessor, and
// that predecessor only has one successor.
// returns the new combined block.
static BasicBlock *foldBlockIntoPredecessor(BasicBlock *BB, LoopInfo *LI,
                                            ScalarEvolution *SE,
                                            SmallPtrSetImpl<Loop *> &ForgottenLoops,
                                            DominatorTree *DT)
{
    BasicBlock *Pred = BB->getSinglePredecessor();

    // can only merge if single distinct predecessor and successor
    if (!Pred) return nullptr;
    if (Pred->getTerminator()->getNumSuccessors() != 1) return nullptr;

    errs() << "merging: " << *BB << "into: " << *Pred;

    // Resolve any PHI nodes at the start of the block.  They are all
    // guaranteed to have exactly one entry if they exist, unless there are
    // multiple duplicate (but guaranteed to be equal) entries for the
    // incoming edges.  This occurs when there are multiple edges from
    // Pred to OnlySucc.
    FoldSingleEntryPHINodes(BB);

    // Delete the unconditional branch from the predecessor...
    Pred->getInstList().pop_back();

    // Make all PHI nodes that referred to BB now refer to Pred as their
    // source...
    BB->replaceAllUsesWith(Pred);

    // Move all definitions in the successor to the predecessor...
    Pred->getInstList().splice(Pred->end(), BB->getInstList());

    // OldName will be valid until erased.
    StringRef OldName = BB->getName();

    // Erase the old block and update dominator info.
    if (DT)
        if (DomTreeNode *DTN = DT->getNode(BB)) {
            DomTreeNode *PredDTN = DT->getNode(Pred);
            SmallVector<DomTreeNode *, 8> Children(DTN->begin(), DTN->end());
            for (auto *DI : Children)
                DT->changeImmediateDominator(DI, PredDTN);

            DT->eraseNode(BB);
        }

    // ScalarEvolution holds references to loop exit blocks.
    if (SE) {
        if (Loop *L = LI->getLoopFor(BB)) {
            if (ForgottenLoops.insert(L).second)
                SE->forgetLoop(L);
        }
    }
    LI->removeBlock(BB);

    // Inherit predecessor's name if it exists...
    if (!OldName.empty() && !Pred->hasName())
        Pred->setName(OldName);

    BB->eraseFromParent();

    return Pred;
}

// if Count is zero, try to automatically find UnrollCount
// if Threshold equal zero, no threshold is enforced
// returns true if any transformations are performed
bool unrollLoop(Loop *L, unsigned Count, unsigned Threshold,
                LoopInfo *LI, DominatorTree *DT, ScalarEvolution *SE,
                AssumptionCache *AC, const TargetTransformInfo &TTI)
{
    assert(L->isLCSSAForm(*DT));
    // TODO: L->isLoopSimplifyForm() ?

    unsigned TripCount, TripMultiple, LoopSize;

    BasicBlock *Header = L->getHeader();
    BasicBlock *LatchBlock = L->getLoopLatch();
    BranchInst *BI = dyn_cast<BranchInst>(LatchBlock->getTerminator());

    // loop must terminate in a condition branch
    // use `loop-rotate` pass to fix this
    if (!BI || BI->isUnconditional()) {
        errs() << "skipping: loop not terminated by a conditional branch\n";
        return false;
    }

    // determine Trip and TripMultiple count
    TripCount = 0;              // 0 = unknown
    TripMultiple = 1;           // greatest known integer multiple of the trip count

    // TODO: check for large loop counts
    BasicBlock *ExitingBlock = L->getLoopLatch();
    if (!ExitingBlock || !L->isLoopExiting(ExitingBlock))
        ExitingBlock = L->getExitingBlock();
    if (ExitingBlock) {
        TripCount = SE->getSmallConstantTripCount(L, ExitingBlock);
        TripMultiple = SE->getSmallConstantTripMultiple(L, ExitingBlock);
    }

    // print counts
    errs() << "  trip count = ";
    if (TripCount != 0) {
        errs() << TripCount << "\n";
    } else {
        errs() << "unknown" << "\n";
    }
    if (TripMultiple != 1) {
        errs() << "  trip multiple = " << TripMultiple << "\n";
    }

    // try to automatically calculate the UnrollCount
    if (Count == 0) {
        // if we know trip count, try to completely unroll (enforcing threshold)
        // else, bail out
        if (TripCount != 0) {
            Count = TripCount;
        } else {
            errs() << "skipping: cannot determine unroll count\n";
            return false;
        }
    }

    // cant unroll more times than the trip count, if known
    if (TripCount != 0 && Count > TripCount) {
        Count = TripCount;
    }

    // check values before proceeding
    assert(Count > 0);
    assert(TripMultiple > 0);
    assert(TripCount == 0 || TripCount % TripMultiple == 0);

    // calculate loop size
    LoopSize = estimateLoopSize(L, AC, TTI);
    errs() << "  size = " << LoopSize << "\n";

    // enforce the threshold
    if (Threshold > 0) {
        uint64_t Size = (uint64_t) LoopSize *Count;
        if (TripCount != 1 && Size > Threshold) {
            errs() << "skipping: too large to unroll (threshold = "
                   << Threshold << ")\n";
            return false;
        }
    }

    // if TripCount and Count is the same, the loop will be completely unrolled
    bool CompletelyUnroll = Count == TripCount;

    // if we know the trip count, we know the multiple...
    unsigned BreakoutTrip = 0;
    if (TripCount != 0) {
        BreakoutTrip = TripCount % Count;
        TripMultiple = 0;
    } else {
        // Figure out what multiple to use.
        BreakoutTrip = TripMultiple =
            (unsigned) GreatestCommonDivisor64(Count, TripMultiple);
    }

    // print some info
    if (CompletelyUnroll) {
        errs() << "COMPLETELY unrolling\n";
    } else {
        errs() << "PARTIALLY unrolling" << " by " << Count << "\n";

        if (TripMultiple == 0 || BreakoutTrip != TripMultiple) {
            errs() << "  with a breakout at trip " << BreakoutTrip << "\n";
        } else if (TripMultiple != 1) {
            errs() << "  with " << TripMultiple << " trips per branch" << "\n";
        }
    }

    std::vector<BasicBlock*> LoopBlocks = L->getBlocks();

    bool ContinueOnTrue = L->contains(BI->getSuccessor(0));
    BasicBlock *LoopExit = BI->getSuccessor(ContinueOnTrue);

    // first iteration should use precloned values in place of phi nodes
    // typedef DenseMap<const Value*, Value*> ValueMapTy;
    ValueToValueMapTy LastValueMap;
    std::vector<PHINode*> OrigPHINode;
    for (BasicBlock::iterator I = Header->begin(); isa<PHINode>(I); ++I) {
        PHINode *PN = cast<PHINode>(I);
        OrigPHINode.push_back(PN);

        if (Instruction *I =
            dyn_cast<Instruction>(PN->getIncomingValueForBlock(LatchBlock))) {
            if (L->contains(I->getParent())) {
                LastValueMap[I] = I;
            }
        }
    }

    std::vector<BasicBlock*> Headers;
    std::vector<BasicBlock*> Latches;
    Headers.push_back(Header);
    Latches.push_back(LatchBlock);

    // setup DFS module
    // the current on-the-fly SSA update requires blocks to be processed in
    // reverse postorder so that LastValueMap contains the correct value at each
    // exit.
    LoopBlocksDFS DFS(L);
    DFS.perform(LI);

    // stash the DFS iterators before adding blocks to the loop.
    LoopBlocksDFS::RPOIterator BlockBegin = DFS.beginRPO();
    LoopBlocksDFS::RPOIterator BlockEnd = DFS.endRPO();

    std::vector<BasicBlock*> UnrolledLoopBlocks = L->getBlocks();

    // unroll
    for (unsigned It = 1; It != Count; ++It) {
        std::vector<BasicBlock*> NewBlocks;
        SmallDenseMap<const Loop *, Loop *, 4> NewLoops;
        NewLoops[L] = L;

        // for each block, for each iteration
        for (LoopBlocksDFS::RPOIterator BB = BlockBegin; BB != BlockEnd; ++BB) {
            // clone block and insert
            ValueToValueMapTy VMap;
            BasicBlock *New = CloneBasicBlock(*BB, VMap, "." + Twine(It));
            Header->getParent()->getBasicBlockList().push_back(New);

            assert((*BB != Header || LI->getLoopFor(*BB) == L) &&
                   "Header should not be in a sub-loop");

            // add info for new block and forget old since values may have changed
            const Loop *OldLoop = addClonedBlockToLoopInfo(*BB, New, LI, NewLoops);
            if (OldLoop && SE) {
                SE->forgetLoop(OldLoop);
            }

            // loop over all of the PHI nodes in the block, changing them to use
            // the incoming values from the previous block
            if (*BB == Header) {
                for (PHINode *OrigPHI : OrigPHINode) {
                    PHINode *NewPHI = cast<PHINode>(VMap[OrigPHI]);
                    Value *InVal = NewPHI->getIncomingValueForBlock(LatchBlock);

                    if (Instruction *InValI = dyn_cast<Instruction>(InVal)) {
                        if (It > 1 && L->contains(InValI)) {
                            InVal = LastValueMap[InValI];
                        }
                    }
                    VMap[OrigPHI] = InVal;
                    New->getInstList().erase(NewPHI);
                }
            }

            // update our running map of newest clones
            LastValueMap[*BB] = New;
            for (ValueToValueMapTy::iterator VI = VMap.begin(), VE = VMap.end();
                 VI != VE; ++VI) {
                LastValueMap[VI->first] = VI->second;
            }

            // add phi entries for newly created values to all exit blocks
            for (BasicBlock *Succ : successors(*BB)) {
                if (L->contains(Succ))
                    continue;

                for (BasicBlock::iterator BBI = Succ->begin();
                     PHINode *phi = dyn_cast<PHINode>(BBI); ++BBI) {
                    Value *Incoming = phi->getIncomingValueForBlock(*BB);
                    ValueToValueMapTy::iterator It = LastValueMap.find(Incoming);

                    if (It != LastValueMap.end())
                        Incoming = It->second;
                    phi->addIncoming(Incoming, New);
                }
            }

            // keep track of new headers and latches as we create them, so that
            // we can insert the proper branches later
            if (*BB == Header)
                Headers.push_back(New);
            if (*BB == LatchBlock)
                Latches.push_back(New);

            NewBlocks.push_back(New);
            UnrolledLoopBlocks.push_back(New);

            // update DomTree: since we just copy the loop body, and each copy
            // has a dedicated entry block (copy of the header block), this
            // header's copy dominates all copied blocks. that means, dominance
            // relations in the copied body are the same as in the original body
            if (DT) {
                if (*BB == Header) {
                    DT->addNewBlock(New, Latches[It - 1]);
                }
                else {
                    auto BBDomNode = DT->getNode(*BB);
                    auto BBIDom = BBDomNode->getIDom();
                    BasicBlock *OriginalBBIDom = BBIDom->getBlock();
                    BasicBlock *Cast = cast<BasicBlock>(LastValueMap[cast<Value>(OriginalBBIDom)]);

                    DT->addNewBlock(New, Cast);
                }
            }
        } // end for LoopBlocks

        // remap all instructions in the most recent iteration
        for (BasicBlock *NewBlock : NewBlocks) {
            for (Instruction &I : *NewBlock) {
                remapInstruction(&I, LastValueMap);

                // handle intrinsics
                if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
                    if (II->getIntrinsicID() == Intrinsic::assume) {
                        AC->registerAssumption(II);
                    }
                }
            }
        }
    } // end for Count


    // the latch block exits the loop
    // update phi nodes in successor to values in last unroll iteration
    if (Count != 1) {
        // find uses of phi
        SmallPtrSet<PHINode*, 8> Users;
        for (Value::use_iterator UI = LatchBlock->use_begin(),
                 UE = LatchBlock->use_end(); UI != UE; ++UI) {
            if (PHINode *phi = dyn_cast<PHINode>(*UI))
                Users.insert(phi);
        }

        BasicBlock *LastIterationBB = cast<BasicBlock>(LastValueMap[LatchBlock]);
        for (SmallPtrSet<PHINode*,8>::iterator SI = Users.begin(), SE = Users.end();
             SI != SE; ++SI) {
            PHINode *PN = *SI;
            Value *InVal = PN->removeIncomingValue(LatchBlock, false);

            // if this value was defined in the loop, take the value defined by
            // the last iteration of the loop
            if (Instruction *InValI = dyn_cast<Instruction>(InVal)) {
                if (L->contains(InValI->getParent()))
                    InVal = LastValueMap[InVal];
            }
            PN->addIncoming(InVal, LastIterationBB);
        }
    }

    // if doing complete unrolling, loop over the PHI nodes in the original
    // block, setting them to their incoming values
    if (CompletelyUnroll) {
        BasicBlock *Preheader = L->getLoopPreheader();
        for (unsigned i = 0, e = OrigPHINode.size(); i != e; ++i) {
            PHINode *PN = OrigPHINode[i];
            PN->replaceAllUsesWith(PN->getIncomingValueForBlock(Preheader));
            Header->getInstList().erase(PN);
        }
    }

    // connect unrolled blocks with branches
    for (unsigned i = 0, e = Latches.size(); i != e; ++i) {
        // original branch was replicated in each unrolled iteration
        BranchInst *Term = cast<BranchInst>(Latches[i]->getTerminator());

        // branch destination
        unsigned j = (i + 1) % e;
        BasicBlock *Dest = Headers[j];
        bool NeedConditional = true;

        // for a complete unroll, make the last iteration end with a branch
        // to the exit block
        if (CompletelyUnroll && j == 0) {
            Dest = LoopExit;
            NeedConditional = false;
        }

        // if we know the trip count or a multiple of it, we can safely use an
        // unconditional branch for some iterations
        if (j != BreakoutTrip && (TripMultiple == 0 || j % TripMultiple != 0)) {
            NeedConditional = false;
        }

        if (NeedConditional) {
            // update the conditional branch's successor for the following
            // iteration
            Term->setSuccessor(!ContinueOnTrue, Dest);
        } else {
            // remove phi operands at this loop exit
            if (Dest != LoopExit) {
                BasicBlock *BB = Latches[i];
                for (BasicBlock *Succ: successors(BB)) {
                    if (Succ == Headers[i])
                        continue;
                    for (BasicBlock::iterator BBI = Succ->begin();
                         PHINode *Phi = dyn_cast<PHINode>(BBI); ++BBI) {
                        Phi->removeIncomingValue(BB, false);
                    }
                }
            }

            // replace the conditional branch with an unconditional one
            BranchInst::Create(Dest, Term);
            Term->eraseFromParent();
        }
    }

    // try to merge adjacent blocks
    SmallPtrSet<Loop *, 4> ForgottenLoops;
    for (BasicBlock *Latch : Latches) {
        BranchInst *Term = cast<BranchInst>(Latch->getTerminator());
        if (Term->isUnconditional()) {
            BasicBlock *Dest = Term->getSuccessor(0);

            if (BasicBlock *Fold =
                foldBlockIntoPredecessor(Dest, LI, SE, ForgottenLoops, DT)) {
                // dest has been folded into Fold. update our worklists accordingly
                std::replace(Latches.begin(), Latches.end(), Dest, Fold);
                UnrolledLoopBlocks.erase(std::remove(UnrolledLoopBlocks.begin(),
                                                     UnrolledLoopBlocks.end(), Dest),
                                         UnrolledLoopBlocks.end());
            }
        }
    }

    errs() << "\ncode cleanup:\n";

    // code cleanup
    const DataLayout &DL = Header->getModule()->getDataLayout();
    const std::vector<BasicBlock*> &NewLoopBlocks = L->getBlocks();

    for (BasicBlock *BB : NewLoopBlocks) {
        BB->dump();

        for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ) {
            Instruction *Inst = &*I++;

            // constant folding
            if (Value *V = SimplifyInstruction(Inst, DL, nullptr, DT, AC)) {
                if (LI->replacementPreservesLCSSAForm(Inst, V)) {
                    // errs() << "replacing\n";
                    // Inst->dump();
                    // errs() << "with\n  ";
                    // V->dump();

                    Inst->replaceAllUsesWith(V);
                }
            }
            // remove dead instructions
            if (isInstructionTriviallyDead(Inst)) {
                // errs() << "removing dead inst:\n";
                // Inst->dump();

                BB->getInstList().erase(Inst);
            }
        }

        errs() << "after:\n";
        BB->dump();
        errs() << "\n";
    }

    return true;
}


// class functions

char LoopUnroll::ID = 0;

bool LoopUnroll::runOnLoop(Loop *L, LPPassManager &LPM)
{
    BasicBlock *H = L->getHeader();
    // Function &F = *L->getHeader()->getParent();
    Function *F = H->getParent();
    StringRef funcName = F->getName();

    // check if magic function
    if (funcName != MAGIC_FUNC) {
        return false;
    }

    errs() << "Loop Unroll: F[" << funcName
           << "] L%" << H->getName() << "\n";

    auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    const TargetTransformInfo &TTI = getAnalysis<TargetTransformInfoWrapperPass>().getTTI(*F);
    auto &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(*F);

    // try to unroll
    if (!unrollLoop(L, UnrollCount, UnrollThreshold, LI, &DT, SE, &AC, TTI)) {
        errs() << "failed...\n";
        return false;
    }

    // TODO: update the loop info
    // // If we completely unrolled the loop, remove it from the parent.
    // if (L->getNumBackEdges() == 0)
    //     LPM.deleteLoopFromQueue(L);
    // LI->markAsRemoved(L);
    // LPM.markLoopAsDeleted(L);

    errs() << "finished\n";

    return true;
}
