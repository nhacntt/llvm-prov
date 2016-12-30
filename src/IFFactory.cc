//! @file IFFactory.cc  Definition of @ref llvm::prov::IFFactory.
/*
 * Copyright (c) 2016 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by BAE Systems, the University of Cambridge
 * Computer Laboratory, and Memorial University under DARPA/AFRL contract
 * FA8650-15-C-7558 ("IFFactory"), as part of the DARPA Transparent Computing
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

#include <llvm/ADT/StringSet.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>

using namespace llvm;
using namespace llvm::prov;


namespace {

class MetaIO : public IFFactory {
public:
  bool IsSource(CallInst*);
  bool CanSink(CallInst*);

  std::unique_ptr<Source> TranslateSource(CallInst*);
};

} // anonymous namespace


std::unique_ptr<IFFactory> IFFactory::Create(Kind K) {
  switch (K) {
  case Kind::FreeBSDMetaIO:
    return std::unique_ptr<IFFactory>(new MetaIO());
  }
}


bool MetaIO::IsSource(CallInst *C) {
  Function *F = C->getCalledFunction();
  if (not (F and F->hasName())) {
    return false;
  }

  StringRef Name = F->getName();

  static StringSet SourceNames({ "read", "mmap" });
  return SourceNames.contains(Name);
}

bool MetaIO::CanSink(CallInst*) {
  return false;
}

std::unique_ptr<Source> MetaIO::TranslateSource(CallInst*) {
  return std::unique_ptr<Source>();
}
