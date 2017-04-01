//! @file CallSemantics.hh  Declaration of @ref llvm::prov::CallSemantics.
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

#ifndef LLVM_PROV_CALL_SEMANTICS_H
#define LLVM_PROV_CALL_SEMANTICS_H

#include "Source.hh"

#include <loom/Instrumenter.hh>

#include <memory>


namespace llvm {

class CallInst;
class Module;
class Value;

namespace prov {

/**
 * A description of the information-flow semantics of a platform's functions.
 *
 * Information can flow out of function calls in a variety of ways besides
 * direct return values. This type can be used to query all of the outputs
 * from a function provided by some platform API. This could be POSIX
 * (e.g., the `read(2)` system call outputs data to a buffer), but other
 * implementations are possible for libraries, etc.
 *
 * @todo Future subclasses of this type could include a mechanism for dealing
 *       with functions bearing in/out annotations on parameters.
 */
class CallSemantics
{
  public:
  //! Create an object to describe POSIX information-flow semantics.
  static std::unique_ptr<CallSemantics> Posix();

  /**
   * Report which values associated with a call are semantic outputs.
   *
   * When we pass arguments to a function call, some represent semantic output
   * even though they are syntactically inputs. For example, when we pass a
   * pointer to the `read(2)` system call, we are outputting data into that
   * buffer, so we should treat that pointer as an output rather than an input.
   */
  virtual SmallVector<Value*, 2> CallOutputs(CallInst*) const = 0;

};

} // namespace prov
} // namespace llvm

#endif // LLVM_PROV_CALL_SEMANTICS_H
