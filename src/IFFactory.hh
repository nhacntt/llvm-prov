//! @file IFFactory.hh  Definition of @ref llvm::prov::IFFactory.
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

#ifndef LLVM_PROV_IFFACTORY_H
#define LLVM_PROV_IFFACTORY_H

#include "Source.hh"

#include <loom/Instrumenter.hh>

#include <memory>


namespace llvm {

class CallInst;
class Module;
class Value;

namespace prov {

class CallSemantics;

/**
 * An information flow mechanism that constructs @ref Source and @ref Sink
 * objects, interprets IF metadata, etc.
 */
class IFFactory
{
  public:
  typedef std::unique_ptr<loom::Instrumenter> InstrPtr;

  //! Create a new FreeBSD-specific @ref IFFactory using metaio.
  static std::unique_ptr<IFFactory> FreeBSDMetaIO(InstrPtr);

  /**
   * What are the call semantics (e.g., which parameters are outputs)
   * on this platform?
   */
  virtual const CallSemantics& CallSemantics() const = 0;

  //! Is this function call a source of information to track?
  virtual bool IsSource(const CallInst*) const = 0;

  //! Can this function call be a sink for tracked information?
  virtual bool CanSink(const CallInst*) const = 0;

  /**
   * Create a @ref Source for information coming out of this @ref CallInst.
   *
   * For example, a system-call-oriented information flow discipline can wrap
   * a `read` system call by treating the data buffer as an output, disregarding
   * the number of bytes returned and extracting metadata with
   * platform-specific techniques.
   */
  virtual Source TranslateSource(CallInst*) = 0;

  /**
   * Add tracing to an information flow sink.
   *
   * After statically following information flows from a source to a sink,
   * add code to link the two by, e.g., propagating tags or other metadata.
   */
  virtual bool TranslateSink(CallInst*, const Source&) = 0;
};

} // namespace prov
} // namespace llvm

#endif // LLVM_PROV_IFFACTORY_H
