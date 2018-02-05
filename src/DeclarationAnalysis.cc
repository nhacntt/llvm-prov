#include "DeclarationAnalysis.hh"

#include <llvm/Support/YAMLTraits.h>
#include <llvm/Support/MemoryBuffer.h>

using namespace llvm;


template<class T>
struct yaml::SequenceTraits<std::vector<T>> {

	static size_t size(yaml::IO&, std::vector<T>& V) { return V.size(); }

	static T& element(yaml::IO&, std::vector<T>& V, size_t index) {
		if ( index >= V.size() )
			V.resize(index+1);
		return V[index];
	}
};

template<>
struct yaml::SequenceTraits<SmallVector<std::string,4>> {
	static size_t size(yaml::IO&, SmallVector<std::string,4>& V) {
		return V.size();
	}

	static std::string& element(yaml::IO&, SmallVector<std::string,4>& V, size_t index) {
		if ( index >= V.size() )
			V.resize(index+1);
		return V[index];
	}
	static const bool flow = true;
};


template <>
struct yaml::MappingTraits<Location> {
	static void mapping(yaml::IO &io, Location &Loc) {
		io.mapOptional("SourceFile", Loc.SourceFile);
		io.mapOptional("Line", Loc.LineNum);
		io.mapOptional("Column",Loc.ColNum);
	}
	static const bool flow = true;
};


template <>
struct yaml::MappingTraits<ParamDecl> {
	static void mapping(yaml::IO &io, ParamDecl &P) {
		io.mapOptional("Name", P.Name);
		io.mapOptional("Location", P.Loc);
		io.mapOptional("Type",P.Type);
		io.mapOptional("Annotations", P.Annotations);
 	}
};

template <>
struct yaml::MappingTraits<FuncDecl> {
	static void mapping(yaml::IO &io, FuncDecl &F) {
  		io.mapOptional("MangledName", F.MangledName);
		io.mapOptional("Name", F.Name);
		io.mapOptional("Location", F.Loc);
		io.mapOptional("ReturnType",F.RType);
		io.mapOptional("Annotations", F.Annotations);
		io.mapOptional("Parameters", F.ParamVec);

	}
};

template <>
struct yaml::MappingTraits<FuncDeclList> {
  static void mapping(yaml::IO &io, FuncDeclList &FL) {
	io.mapOptional("Function Declaration List", FL.List);

  }
};

void FuncDeclList::toYAML(raw_ostream &OS) {
	yaml::Output yout(OS);
	yout << this->List;
}

ErrorOr<FuncDeclList> FuncDeclList::fromYAML(std::string Filename) {
  	auto F = llvm::MemoryBuffer::getFile(Filename);
  	if (std::error_code ec = F.getError()) return ec;

	FuncDeclList FL;
	llvm::StringRef Str = F.get()->getBuffer();
	yaml::Input In(Str);
	In >> FL.List;

	if (std::error_code ec = In.error()) return ec;
	return FL;
}
