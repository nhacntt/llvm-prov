//! @file OptPass.cc  LLVM @b opt pass for adding provenance tracing to IR
/*
 * Copyright (c) 2016 Jonathan Anderson
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

#include "IFFactory.hh"

#include "loom/Instrumenter.hh"

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/raw_ostream.h>

#include <sstream>

using namespace llvm;
using namespace llvm::prov;
using namespace loom;
using std::string;
using std::unique_ptr;


namespace {
  struct OptPass : public ModulePass {
    static char ID;
    OptPass() : ModulePass(ID) {}

    bool runOnModule(Module&) override;
  };
}


/**
 * Collect the transitive closure of @ref V's `User`s.
 *
 * @param[in]   V        the @ref Value to collect `User`s of
 * @param[out]  Users    the set to store the `User`s in
 */
static void CollectUsers(const Value *V, SmallPtrSetImpl<const User*>& Users);

static string JoinVec(const std::vector<string>&);


bool OptPass::runOnModule(Module &Mod)
{
  auto S = InstrStrategy::Create(loom::InstrStrategy::Kind::Inline, false);

  unique_ptr<Instrumenter> Instr(
    Instrumenter::Create(Mod, JoinVec, std::move(S)));

  unique_ptr<IFFactory> IF = IFFactory::FreeBSDMetaIO(Mod);
  SmallVector<CallInst*, 8> Sources;

  for (auto& Fn : Mod) {
    for (auto& Inst : instructions(Fn)) {
      if (CallInst* Call = dyn_cast<CallInst>(&Inst)) {
        if (IF->IsSource(Call)) {
          Sources.push_back(Call);
        }
      }
    }
  }

  bool ModifiedIR = false;
  for (CallInst *Call : Sources) {
    unique_ptr<Source> Source = IF->TranslateSource(Call);
    assert(Source && "IFFactory didn't translate the IF source");

    for (const Value *V : Source->Outputs()) {
      SmallPtrSet<const User*, 4> Users;
      CollectUsers(V, Users);
    }

    ModifiedIR = true;
  }

  return ModifiedIR;
}

static void CollectUsers(const Value *V, SmallPtrSetImpl<const User*>& Users) {
  for (const Use& U : V->uses()) {
    const User *U2 = U.getUser();
    if (Users.count(U2) == 0) {
      Users.insert(U2);
      CollectUsers(U2, Users);
    }
  }
}

static string JoinVec(const std::vector<string>& V) {
    std::ostringstream oss;
    std::copy(V.begin(), V.end() - 1, std::ostream_iterator<string>(oss, "_"));
    oss << *V.rbegin();
    return oss.str();
}

char OptPass::ID = 0;
static RegisterPass<OptPass> X("prov", "Provenance tracking", false, false);
