#ifndef LLVM_ANALYSIS_DECLARATION_ANALYSIS_H
#define LLVM_ANALYSIS_DECLARATION_ANALYSIS_H

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ErrorOr.h>

#include <string>
#include <vector>

//using namespace std;

namespace llvm {

struct Location {
	std::string SourceFile;
	unsigned LineNum;
	unsigned ColNum;
	
	//Constructors
	Location() = default;

	Location(const Location&) = default;

	Location(std::string SourceFile, unsigned LineNum, unsigned ColNum):
		SourceFile(SourceFile),
		LineNum(LineNum),
		ColNum(ColNum){};

};


struct Declaration {
	Location Loc;
	std::string Name;
	llvm::SmallVector<std::string,4> Annotations;
	
	//Constructors
	Declaration() = default;

	Declaration(const Declaration&) = default;

	Declaration(Location Loc, std::string Name, llvm::SmallVector<std::string,4> Annotations):
		Loc(Loc),
		Name(Name),
		Annotations(Annotations){};

};

//Information of a parameter's declaration
struct ParamDecl : public Declaration {
	std::string Type;
	
	//Constructor
	ParamDecl() = default;

	ParamDecl(const Declaration& D, std::string Type):
		Declaration(D),
		Type(Type){};

};

//Information of a function's declaration
struct FuncDecl : public Declaration {
	std::string MangledName;				//Mangled Name
	std::string RType;					//Return Type
	std::vector<ParamDecl> ParamVec;		//Function's parameters
	bool hasAnnotation;

	//Constructor
	FuncDecl()=default;
	
	FuncDecl(const Declaration& D, std::string MangledName, std::string RType, std::vector<ParamDecl> ParamVec):
		Declaration(D),
		MangledName(MangledName),
		RType(RType),
		ParamVec(ParamVec){

		hasAnnotation = !Annotations.empty();
		if (hasAnnotation) return;
		for (const auto& P:ParamVec){
			hasAnnotation = not P.Annotations.empty();
			if (hasAnnotation) return;
		}
	}
};

struct FuncDeclList{

	std::vector<FuncDecl> List;
	llvm::ErrorOr<FuncDeclList> fromYAML(std::string FileName);
	void toYAML(llvm::raw_ostream &OS);
};

} //namespace

#endif
