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

#include <llvm/ADT/iterator_range.h>
#include <llvm/Analysis/MemorySSA.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/User.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace llvm::prov;


typedef std::unordered_set<Value*> ValueSet;

/**
 * Find all memory operations that may have clobbered the location being
 * accessed by an Instruction.
 */
static ValueSet ClobberersOf(Instruction *, MemorySSA &);

/**
 * Recursively walk backwards through MemoryPhi operations until we reach
 * MemoryDef operations and real Instruction values that clobber memory.
 */
static ValueSet PhiClobberers(MemoryPhi *, MemorySSA &);


FlowFinder::FlowSet
FlowFinder::FindPairwise(Function &Fn, MemorySSA& MSSA) {
  FlowFinder::FlowSet Flows;

  for (auto &I : instructions(Fn)) {
    CollectPairwise(&I, MSSA, Flows);
  }

  return Flows;
}

FlowFinder::ValueSet
FlowFinder::FindEventual(const FlowSet& Pairs, Value *Source, ValuePredicate F)
{
  ValueSet Sinks;

  auto Range = Pairs.equal_range(Source);
  for (auto i = Range.first; i != Range.second; i++) {
    Value *Dest = i->second.first;
    assert(Dest != Source);

    if (F(Dest)) {
      Sinks.emplace(Dest);
    }

    ValueSet Next = FindEventual(Pairs, Dest, F);
    Sinks.insert(Next.begin(), Next.end());
  }

  return Sinks;
}

void FlowFinder::CollectPairwise(Value *V, MemorySSA &MSSA,
                                 FlowSet& Flows) const {

  auto *Dest = dyn_cast<User>(V);
  if (not Dest) {
    return;
  }

  // Ignore constants and already-considered nodes.
  if (isa<Constant>(Dest) or Flows.find(Dest) != Flows.end()) {
    return;
  }

  for (Value *Operand : Dest->operands()) {
    // Ignore constants and pseudo-Value types like debug metadata.
    if (not isa<Argument>(Operand) and not isa<User>(Operand)
        or isa<Constant>(Operand)) {
      continue;
    }

    Flows.insert({ Dest, { Operand, FlowKind::Operand }});
  }

  // Load instructions have an implicit dependency on instructions that have
  // clobbered the location being loaded from. If the value we're inspecting is
  // an Instruction, and if it has significance to MemorySSA, and if that
  // significance is that it's a MemoryUse, figure out who clobbered the memory.
  if (auto *Inst = dyn_cast<Instruction>(Dest)) {
    for (Value *V : ClobberersOf(Inst, MSSA)) {
      Flows.insert({ Dest, { V, FlowKind::Memory }});
    }
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

static std::string LineAttrs(FlowFinder::FlowKind Kind)
{
  switch (Kind) {
  case FlowFinder::FlowKind::Operand:
    return "[ color = \"lightsteelblue4\", style = \"solid\" ]";

  case FlowFinder::FlowKind::Memory:
    return "[ color = \"orangered3\", style = \"dashed\" ]";
  }
}

static void Describe(const Value *Source, const Value *Dest,
                     FlowFinder::FlowKind Kind, llvm::raw_ostream &Out)
{
  Out << "\t\"" << Source << "\" -> \"" << Dest << "\" " << LineAttrs(Kind)
    << "\n"
    ;
}

void FlowFinder::Graph(const FlowSet& Flows, llvm::raw_ostream &Out) const {
  Out << "digraph {\n"
    << "\tnode [ fontname = \"Inconsolata\" ];\n"
    << "\n"
    ;

  for (auto& Flow : Flows) {
    const Value *Dest = Flow.first;
    const Value *Src = Flow.second.first;
    const FlowKind Kind = Flow.second.second;

    Describe(Src, Out);
    Describe(Dest, Out);
    Describe(Src, Dest, Kind, Out);
  }

  Out << "}\n";
}

static ValueSet ClobberersOf(Instruction *I, MemorySSA &MSSA)
{
  MemoryAccess *MA = MSSA.getMemoryAccess(I);
  if (not MA) {
    return {};
  }

  auto *MU = dyn_cast<MemoryUse>(MA);
  if (not MU) {
    return {};
  }

  ValueSet Clobberers;
  MemoryAccess *Clobberer = MSSA.getWalker()->getClobberingMemoryAccess(MA);

  if (auto *Def = dyn_cast<MemoryDef>(Clobberer)) {
    // The memory was written to by an easily-discernable instruction like
    // a store that comes earlier in the function.
    Clobberers.insert(Def->getMemoryInst());

  } else if (auto *Phi = dyn_cast<MemoryPhi>(Clobberer)) {
    // Is there a potentially more complex scenario in which multiple stores
    // can clobber the memory location? If so, we'll need to (recursively)
    // chase down all of the possible clobbering instructions.
    auto Sub = PhiClobberers(Phi, MSSA);
    Clobberers.insert(Sub.begin(), Sub.end());
  }

  return Clobberers;
}


static ValueSet PhiClobberers(MemoryPhi *Phi, MemorySSA &MSSA)
{
  ValueSet Clobberers;

  for (Use &U : Phi->incoming_values()) {
    Value *V = U.get();

    if (auto *SubPhi = dyn_cast<MemoryPhi>(V)) {
      auto Sub = PhiClobberers(SubPhi, MSSA);
      Clobberers.insert(Sub.begin(), Sub.end());
      continue;
    }

    auto *MD = dyn_cast<MemoryDef>(U.get());
    assert(MD);

    if (MSSA.isLiveOnEntryDef(MD)) {
      continue;
    }

    assert(MD->getMemoryInst());
    Clobberers.insert(MD->getMemoryInst());
  }

  return Clobberers;
}


