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

#include "loom/Instrumenter.hh"

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>

using namespace llvm;
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


static string JoinVec(const std::vector<string>&);


bool OptPass::runOnModule(Module &Mod)
{
  auto S = InstrStrategy::Create(loom::InstrStrategy::Kind::Inline, false);

  unique_ptr<Instrumenter> Instr(
    Instrumenter::Create(Mod, JoinVec, std::move(S)));

  bool ModifiedIR = false;

  for (auto& Fn : Mod) {
    for (auto& Inst : instructions(Fn)) {
      if (CallInst* Call = dyn_cast<CallInst>(&Inst)) {
        Function *Target = Call->getCalledFunction();
        if (not Target)
          continue; // TODO: support indirect targets
      }
    }
  }

  return ModifiedIR;
}

static string JoinVec(const std::vector<string>& V) {
    std::ostringstream oss;
    std::copy(V.begin(), V.end() - 1, std::ostream_iterator<string>(oss, "_"));
    oss << *V.rbegin();
    return oss.str();
}

char OptPass::ID = 0;
static RegisterPass<OptPass> X("prov", "Provenance tracking", false, false);
