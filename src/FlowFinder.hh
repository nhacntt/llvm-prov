//! @file FlowFinder.hh  Definition of @ref llvm::prov::FlowFinder.
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

#ifndef LLVM_PROV_FLOW_FINDER_H
#define LLVM_PROV_FLOW_FINDER_H

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <map>
#include <unordered_set>


namespace llvm {

class CallInst;
class MemorySSA;
class Module;
class Value;

namespace prov {

class CallSemantics;

/**
 * A type for discovering intraprocedural data flows.
 *
 * Objects of this type can be used to find pairwise data flows within
 * procedures and (eventually) to join pairwise flows into transitive closures
 * of data flow.
 */
class FlowFinder {
public:
  FlowFinder(const CallSemantics &CS) : CS(CS) {}

  //! A set of pairwise value-to-value flows (Dest -> Source).
  using FlowSet = std::multimap<Value*, Value*>;

  //! Find all pairwise data flows within a function.
  FlowSet FindPairwise(Function&, llvm::MemorySSA&);

  using ValueSet = std::unordered_set<Value*>;
  using ValuePredicate = std::function<bool (const Value*)>;

  /**
   * Find macro (not necessarily pairwise) flows within a procedure from a
   * source to the set of eventual sinks that satisfy a predicate.
   *
   * This method computes over the transitive closure of pairwise "flows-to"
   * relations described in @b Pairs. When a flowed-to Value satisfies the 
   * ValuePredicate P, it is added to the resulting ValueSet and the transitive
   * closure operation continues; intermediate sinks are included in the result
   * alongside final sinks. That is, in the following value chain:
   *
   * [Source] -> A -> B -> [Sink1] -> C -> D -> [Sink2]
   *
   * `FindEventual(Pairs, Source, IsSink)` will return both Sink1 and Sink2.
   */
  ValueSet FindEventual(const FlowSet& Pairs, Value *Source, ValuePredicate P);

  //! Output a GraphViz dot representation of a set of pairwise flows.
  void Graph(const FlowSet&, llvm::raw_ostream&) const;

private:
  //! Collect transitive closure of pairwise information flows to @ref V.
  void CollectPairwise(Value *V, MemorySSA&, FlowSet&) const;

  const CallSemantics &CS;
};

} // namespace prov
} // namespace llvm

#endif // LLVM_PROV_FLOW_FINDER_H
