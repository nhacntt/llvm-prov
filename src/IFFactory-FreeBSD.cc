//! @file FreeBSD.cc  FreeBSD implementation of @ref llvm::prov::IFFactory
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
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;
using namespace llvm::prov;


namespace {

class MetaIO : public IFFactory {
public:
  MetaIO(Module&);

  bool IsSource(CallInst*) override;
  bool CanSink(CallInst*) override;

  std::unique_ptr<Source> TranslateSource(CallInst*) override;

private:
  //! Find or construct the `struct metaio` type.
  StructType* MetadataType();

  //! Find or construct the `struct uuid` type.
  StructType* UUIDType();

  Module& Mod;
  LLVMContext& Ctx;
  IntegerType *i32, *i64;
};

} // anonymous namespace


std::unique_ptr<IFFactory> IFFactory::FreeBSDMetaIO(Module& Mod) {
  return std::unique_ptr<IFFactory>(new MetaIO(Mod));
}


MetaIO::MetaIO(Module& Mod)
  : Mod(Mod), Ctx(Mod.getContext()),
    i32(IntegerType::get(Ctx, 32)), i64(IntegerType::get(Ctx, 32))
{
}


bool MetaIO::IsSource(CallInst *Call) {
  Function *F = Call->getCalledFunction();
  if (not (F and F->hasName())) {
    return false;
  }

  StringRef Name = F->getName();

  static llvm::StringSet<> SourceNames({
    "mmap",
    "pread", "preadv",
    "read", "readv",
    /* recv, */ "recvfrom", "recvmsg", /* recvmmsg */
  });

  return (SourceNames.find(Name) != SourceNames.end());
}

bool MetaIO::CanSink(CallInst *Call) {
  Function *F = Call->getCalledFunction();
  if (not (F and F->hasName())) {
    return false;
  }

  StringRef Name = F->getName();

  static llvm::StringSet<> SinkNames({
    /* "mmap", */ // information actually flows when we write into the memory
    "pwrite", "pwritev",
    "sendmsg", "sendto",
    "write", "writev",
  });

  return (SinkNames.find(Name) != SinkNames.end());
}

std::unique_ptr<Source> MetaIO::TranslateSource(CallInst *Call) {
  // Identify the function being called
  Function *F = Call->getCalledFunction();
  assert(F and F->hasName());
  StringRef Name = F->getName();

  FunctionType *FT = F->getFunctionType();
  assert(not FT->isVarArg()); // TODO(JA): do we need/use varargs?

  // Allocate a `struct metaio` on the stack
  Value *MetaIOPtr =
    IRBuilder<>(Call).CreateAlloca(MetadataType(), nullptr, "metaio");

  // Construct the metaio variant of the system call
  std::vector<Type*> ParamTypes = FT->params();
  SmallVector<Value*, 4> Arguments(Call->arg_begin(), Call->arg_end());
  Arguments.push_back(MetaIOPtr);
  ParamTypes.push_back(MetaIOPtr->getType());

  Constant *MetaFn = Mod.getOrInsertFunction(("metaio_" + Name).str(),
    FunctionType::get(FT->getReturnType(), ParamTypes, false));

  // Create a call to the metaio system call to replace the original call
  CallInst *MetaCall = CallInst::Create(MetaFn, Arguments);
  ReplaceInstWithInst(Call, MetaCall);

  // Finally, determine which value(s) constitute outputs from this IF source.
  // We have to do this after replacing the original CallInst, since the return
  // value might be the output value in question (so we need the replaced call).
  SmallVector<const Value*, 4> OutputValues;

  if (Name == "mmap") {
    // The "output" of mmap(2) is the memory being mapped (the return value):
    OutputValues.push_back(MetaCall);

  } else if (Name.find("read") != StringRef::npos) {
    //
    // The "output" of all four read(2)-derived syscalls is the first parameter:
    //
    // read:   fd, buf, nbytes
    // pread:  fd, buf, nbytes, offset
    // readv:  fd, iov, iovcnt
    // preadv: fd, iov, iovcnt, offset
    //
    OutputValues.push_back(Call->getArgOperand(1));

  } else if (Name.find("recv") != StringRef::npos) {
    //
    // The "output" of all four recv(2)-derived syscalls is the first parameter:
    //
    // recv:      s, buf, len, flags
    // recvfrom:  s, buf, len, flags, from, fromlen
    // recvmsg:   s, msg, flags
    // recvmmsg:  s, msgvec, vlen, flags, timeout
    //
    OutputValues.push_back(Call->getArgOperand(1));

  } else {
    assert(false && "unhandled source function");
  }

  return MakeSource(OutputValues, MetaIOPtr);
}


StructType* MetaIO::MetadataType() {
  if (StructType *T = Mod.getTypeByName("metaio")) {
    return T;
  }

  IntegerType *lwpidTy = i32;
  IntegerType *msgidTy = i64;

  std::vector<Type*> FieldTypes = {
    lwpidTy,    // mio_tid
    i32,        // _mio_pad0
    msgidTy,    // mio_syscallid
    msgidTy,    // mio_msgid
    i64,        // _mio_pad1
    UUIDType(), // mio_uuid
  };

  return StructType::create(FieldTypes, "metaio");
}

StructType* MetaIO::UUIDType() {
  if (StructType *T = Mod.getTypeByName("uuid")) {
    return T;
  }

  IntegerType *i8 = IntegerType::get(Ctx, 8);
  IntegerType *i16 = IntegerType::get(Ctx, 16);
  ArrayType *nodeTy = ArrayType::get(i8, 6);

  std::vector<Type*> FieldTypes = {
    i32,        // time_low
    i16,        // time_mid
    i16,        // time_hi_and_version
    i8,         // clock_seq_hi_and_reserved
    i8,         // clock_seq_low
    nodeTy,     // node[_UUID_NODE_LEN]
  };

  return StructType::create(FieldTypes, "uuid");
}
