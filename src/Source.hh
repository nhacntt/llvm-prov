//! @file Source.hh  Definition of @ref llvm::prov::Source.
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

#ifndef LLVM_PROV_SOURCE_H
#define LLVM_PROV_SOURCE_H

#include <llvm/ADT/TinyPtrVector.h>
#include <llvm/IR/Value.h>

#include <string>
#include <vector>


namespace llvm {

class CallInst;

namespace prov {

/**
 * A source for information flow / provenance within a procedure.
 *
 * For the moment, we only support functions as sources of information.
 * This allows us to track data coming from system calls, which is sufficient
 * for our current purposes.
 */
class Source
{
  public:
  /**
   * The values that are output from this source.
   *
   * Determining the values that are output from a source requires some
   * knowledge of the source's semantics: it's not as simple as treating
   * arguments and inputs and returned values as outputs.  For example,
   * it may be desirable to disregard the size returned from the `read`
   * system call, whereas the buffer that `read` copies data into should be
   * treated as an output.
   */
  TinyPtrVector<const Value*> Outputs() const { return OutputValues; }

  /**
   * Get the metadata (if any) associated with a metadata source.
   *
   * This may be a tag that fits in a register or a pointer to a memory
   * containing more complex metadata.  The semantics of this metadata
   * are implementation-defined.
   */
  Value* Metadata() const { return MetadataValue; }

private:
  Source(ArrayRef<const Value*> Outputs, Value *MetadataValue)
    : OutputValues(Outputs), MetadataValue(MetadataValue)
  {
  }

  TinyPtrVector<const Value*> OutputValues;
  Value *MetadataValue;

  friend class IFFactory;
};

} // namespace prov
} // namespace llvm

#endif // LLVM_PROV_SOURCE_H
