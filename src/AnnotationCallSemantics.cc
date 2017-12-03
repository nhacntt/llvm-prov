#include "AnnotationCallSemantics.hh"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/StringRef.h>

//#define DEBUG
using namespace std;
using namespace llvm;

unique_ptr<prov::CallSemantics> prov::CallSemantics::Posix() {
	return NULL ;//unique_ptr<CallSemantics>(new AnnotationCallSemantics());
}

AnnotationCallSemantics::AnnotationCallSemantics(	const map<string,vector<int>> InputArg,
													const map<string,vector<int>> OutputArg,
													const StringSet<> SourceNames,
													const StringSet<> SinkNames)
																:
																InputArgMap(InputArg),
																OutputArgMap(OutputArg),
																ExplicitSourceNames(SourceNames),
																ExplicitSinkNames(SinkNames)
																{};

AnnotationCallSemantics::AnnotationCallSemantics(FuncDeclList FL):AnnotationCallSemantics(	getInputArgMap(FL),
																							getOutputArgMap(FL),
																							getExplicitSourceNames(FL),
																							getExplicitSinkNames(FL)){};

	

AnnotationCallSemantics::AnnotationCallSemantics(string Filename):AnnotationCallSemantics(getFuncListfromYAML(Filename)){
#ifdef DEBUG
	errs()<<"Initializing CallSemantic...\n";
	errs()<<"Input arguments:\n";
	for (const auto& e:InputArgMap){
		errs()<<"\t"<<e.first<<":"<<vector2Str(e.second)<<"\n";
	}
	errs()<<"Output arguments:\n";
	for (const auto& e:OutputArgMap){
		errs()<<"\t"<<e.first<<":"<<vector2Str(e.second)<<"\n";
	}
	errs()<<"Source names: ";
	for (const auto& e:ExplicitSourceNames) errs()<<e.getKey()<<", ";
	errs()<<"\nSink names: ";
	for (const auto& e:ExplicitSinkNames) errs()<<e.getKey()<<", ";
	errs()<<"\n";
#endif
};

SmallVector<Value*, 2> AnnotationCallSemantics::CallOutputs(CallInst* Call) const {
	SmallVector<Value*, 2> Outputs = { Call };
	
	Function *F = Call->getCalledFunction();
	if (not F or not F->hasName()) {
    		return Outputs;
	}

  	StringRef FnName = F->getName();
  	auto i = OutputArgMap.find(FnName);
  	if (i == OutputArgMap.end()) {
    		return Outputs;
  	}
  	const vector<int> ArgVec = i->second;
  	for (const auto& ArgNum:ArgVec){
  		if (ArgNum >= Call->getNumArgOperands()) {
    		errs()
      			<< "ERROR: " << FnName << " has only "
      			<< Call->getNumArgOperands() << " params, expected at least "
      			<< (ArgNum + 1)
      			;

    		return {};
  		}

		Outputs.push_back(Call->getArgOperand(ArgNum));
	}

	return Outputs;
}

bool AnnotationCallSemantics::IsSource(const CallInst *Call) const {
	Function *F = Call->getCalledFunction();
	if (not (F and F->hasName())) {
    		return false;
  	}
	StringRef FnName = F->getName();

	if (ExplicitSourceNames.find(FnName) != ExplicitSourceNames.end()) return true;

  	return (OutputArgMap.find(FnName) != OutputArgMap.end());
}

bool AnnotationCallSemantics::CanSink(const CallInst *Call) const {
	Function *F = Call->getCalledFunction();
	if (not (F and F->hasName())) {
    		return false;
  	}
	StringRef FnName = F->getName();

	if (ExplicitSinkNames.find(FnName) != ExplicitSinkNames.end()) return true;

  	return (InputArgMap.find(FnName) != InputArgMap.end());

}
