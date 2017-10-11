//! @file GraphFlowsPass.cc  @b opt pass to graph data flows
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
#include "PosixCallSemantics.hh"

#include "loom/Instrumenter.hh"

#include <llvm/Pass.h>
#include <llvm/Analysis/MemorySSA.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <sstream>

using namespace llvm;
using namespace llvm::prov;
using namespace loom;
using std::string;


namespace llvm {
  struct GraphFlowsPass : public FunctionPass {
    static char ID;
    GraphFlowsPass() : FunctionPass(ID) {}

    bool runOnFunction(Function&) override;
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      // Our instrumentation injects instructions and may extend system calls,
      // but it doesn't modify the control-flow graph of *our* code (i.e.,
      // it doesn't add/remove BasicBlocks or modify our own branches/returns).
      AU.setPreservesCFG();

      // We need analysis of which loads depend on which stores.
      AU.addRequiredTransitive<AAResultsWrapperPass>();
      AU.addPreserved<AAResultsWrapperPass>();
      AU.addRequiredTransitive<MemorySSAWrapperPass>();
    }
  };
}


cl::opt<string> OutputDirectory("flow-dir", cl::init("data-flow-graphs"),
    cl::desc("Directory for data flow graphs"), cl::value_desc("dir"));

cl::opt<bool> ShowBasicBlocks("show-bbs", cl::init(true),
    cl::desc("Show basic blocks in data flow graphs"));

static string JoinVec(const std::vector<string>&);


bool GraphFlowsPass::runOnFunction(Function &Fn)
{
  // Create output directory (if it doesn't exist); open output file.
  std::error_code Err = sys::fs::create_directory(OutputDirectory);
  if (Err) {
    errs() << "Error creating output directory '" << OutputDirectory << "': "
      << Err.message() << "\n";
    return false;
  }

  std::string Filename = (OutputDirectory + "/" + Fn.getName() + ".dot").str();
  auto Flags = sys::fs::OpenFlags::F_RW | sys::fs::OpenFlags::F_Text;

  raw_fd_ostream FlowGraph(Filename, Err, Flags);

  if (Err) {
    errs() << "Error opening graph file: " << Err.message() << "\n";
    return false;
  }

  PosixCallSemantics CS;
  FlowFinder FF(CS);

  auto IsSink = [&CS](const Value *V) {
    if (auto *Call = dyn_cast<CallInst>(V)) {
        return CS.CanSink(Call);
    }

    return false;
  };

  MemorySSA &MSSA = getAnalysis<MemorySSAWrapperPass>().getMSSA();
  FlowFinder::FlowSet Flows = FF.FindPairwise(Fn, MSSA);
  FF.Graph(Flows, Fn.getName(), ShowBasicBlocks, FlowGraph);

  for (auto& I : instructions(Fn)) {
    if (CallInst* Source = dyn_cast<CallInst>(&I)) {
      if (not CS.IsSource(Source)) {
        continue;
      }

      for (Value *Sink : FF.FindEventual(Flows, Source, IsSink)) {
        Flows.insert({ Sink, { Source, FlowFinder::FlowKind::Meta }});
      }
    }
  }

  return false;
}

static string JoinVec(const std::vector<string>& V) {
    std::ostringstream oss;
    std::copy(V.begin(), V.end() - 1, std::ostream_iterator<string>(oss, "_"));
    oss << *V.rbegin();
    return oss.str();
}

char GraphFlowsPass::ID = 0;

static RegisterPass<GraphFlowsPass> X("graph-flows", "GraphFlowsPass tracking");
