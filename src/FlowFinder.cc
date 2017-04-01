//! @file FlowFinder.cc  Definition of @ref llvm::prov::FlowFinder.
/*
 * Copyright (c) 2016-2017 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by BAE Systems, the University of Cambridge
 * Computer Laboratory, and Memorial University under DARPA/AFRL contract
 * FA8650-15-C-7558 ("CADETS"), as part of the DARPA Transparent Computing
 * (TC) research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "CallSemantics.hh"
#include "FlowFinder.hh"

#include <llvm/IR/InstIterator.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_set>

using namespace llvm;
using namespace llvm::prov;


typedef std::unordered_set<User*> UserSet;

/**
 * Find all Users that are directly derived from a source Value.
 *
 * @param   Source      the Value data is flowing from
 * @param   After       only include Users that are found after this Instruction
 *                      (if set, otherwise include all derived values)
 */
static UserSet DerivationsFrom(Value *Source, Instruction *After = nullptr);

/**
 * Find all of the User values that are contained in a BasicBlock.
 */
static void FindInBlock(const UserSet& Needles, UserSet& ResultUsers,
                        BasicBlock *BB);


FlowFinder::FlowSet
FlowFinder::FindPairwise(Function &Fn, const AliasAnalysis &AA) {
  FlowFinder::FlowSet Flows;

  for (auto &I : instructions(Fn)) {
    CollectPairwise(&I, AA, Flows);
  }

  return Flows;
}

void FlowFinder::CollectPairwise(Value *Src, const AliasAnalysis &AA,
                                 FlowSet& Flows) const {
  // Ignore constants and already-considered nodes.
  if (isa<Constant>(Src) or Flows.find(Src) != Flows.end()) {
    return;
  }

  UserSet InfluencedUsers;

  if (CallInst *Call = dyn_cast<CallInst>(Src)) {
    for (Value *Dest : CS.CallOutputs(Call)) {
      UserSet DestUsers = DerivationsFrom(Dest->stripPointerCasts(), Call);
      InfluencedUsers.insert(DestUsers.begin(), DestUsers.end());
    }

  } else if (StoreInst *Store = dyn_cast<StoreInst>(Src)) {
    Value *Ptr = Store->getPointerOperand()->stripPointerCasts();
    InfluencedUsers = DerivationsFrom(Ptr, Store);

  } else {
    InfluencedUsers = DerivationsFrom(Src, dyn_cast<Instruction>(Src));
  }

  for (Value *V : InfluencedUsers) {
    Flows.insert({ Src, V });
    CollectPairwise(V, AA, Flows);
  }
}

static void Describe(const Value *V, llvm::raw_ostream &Out) {
  std::string Colour = "#eeeeee";
  std::string Shape = "box";

  if (isa<AllocaInst>(V)) {
    Colour = "#9999ff";
    Shape = "ellipse";
  } else if (isa<BranchInst>(V)) {
    Colour = "#ff99ff";
    Shape = "cds";
  } else if (isa<GetElementPtrInst>(V)) {
    Colour = "#999999";
    Shape = "tab";
  } else if (isa<CallInst>(V)) {
    Colour = "#ffff99";
    Shape = "cds";
  } else if (isa<LoadInst>(V)) {
    Colour = "#99ff99";
    Shape = "house";
  } else if (isa<StoreInst>(V)) {
    Colour = "#ff9999";
    Shape = "invhouse";
  }

  Out << "\t\"" << V << "\" [ style = \"filled\", label = \"";
  V->print(Out);
  Out
    << "\", fillcolor = \"" << Colour << "99\""
    << ", shape = \"" << Shape << "\""
    << " ];\n"
    ;
}

void FlowFinder::Graph(const FlowSet& Flows, llvm::raw_ostream &Out) const {
  Out << "digraph {\n";

  for (auto& Flow : Flows) {
    const Value *Src = Flow.first;
    const Value *Dest = Flow.second;

    Describe(Src, Out);
    Describe(Dest, Out);

    Out << "\t\"" << Src << "\" -> \"" << Dest << "\";\n";
  }

  Out << "}\n";
}

static UserSet DerivationsFrom(Value *Source, Instruction *After) {
  // Start by finding all users. This set will be trimmed down as we find its
  // elements in the call graph and put them in DirectUsers.
  UserSet Users;
  for (Use &U : Source->uses()) {
    Users.insert(U.getUser());
  }

  // Now we will look for users that come after the specified instruction
  // (whether or not they are in dominance relationships).
  UserSet DirectUsers;

  // Start with successive users within the same BasicBlock.
  BasicBlock *BB = After->getParent();
  bool FoundValue = (After == nullptr);

  for (Instruction &I : *BB) {
    // Look for the value itself within this BasicBlock: the instruction
    // immediately after this one is the first potential successive use.
    if (not FoundValue) {
      FoundValue = (&I == After);
      continue;
    }

    // If this instruction (which comes after `After`) is a User, note it.
    User *U = &I;
    if (Users.count(U) == 1) {
      DirectUsers.insert(U);
    }
  }

  // Next, find all users in successor blocks.
  for (BasicBlock *Successor : BB->getTerminator()->successors()) {
    FindInBlock(Users, DirectUsers, Successor);
  }

  return DirectUsers;
}

static void FindInBlock(const UserSet& Needles, UserSet& ResultUsers,
                        BasicBlock *BB) {

  for (Instruction &I : *BB) {
    if (Needles.count(&I) == 1) {
      ResultUsers.insert(&I);
    }
  }
}
