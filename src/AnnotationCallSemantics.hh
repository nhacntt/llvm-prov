#ifndef LLVM_PROV_ANNOTATION_CALL_SEMANTICS_H
#define LLVM_PROV_ANNOTATION_CALL_SEMANTICS_H

#include "CallSemantics.hh"
#include "AnnotationsHelper.hh"

#include <llvm/Analysis/DeclarationAnalysis.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/SmallVector.h>

#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace llvm;

class AnnotationCallSemantics : public prov::CallSemantics{
	public:
  		AnnotationCallSemantics()=default;
		
  		AnnotationCallSemantics(string Filename);

  		SmallVector<Value*, 2> CallOutputs(CallInst*) const override;
  		bool IsSource(const CallInst*) const override;
  		bool CanSink(const CallInst*) const override;

	private:
  		const map<string,vector<int>> InputArgMap;
		const map<string,vector<int>> OutputArgMap;
		const StringSet<> ExplicitSourceNames;
		const StringSet<> ExplicitSinkNames;

		AnnotationCallSemantics(const map<string,vector<int>>,
								const map<string,vector<int>>,
								const StringSet<>,
								const StringSet<>);

  		AnnotationCallSemantics(FuncDeclList FL);

};


#endif /* !LLVM_PROV_ANNOTATION_CALL_SEMANTICS_H */
