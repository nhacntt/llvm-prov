#include "AnnotationsHelper.hh"
#include <sstream>
#include <algorithm>

using namespace std;
using namespace llvm;

FuncDeclList getFuncListfromYAML(string Filename){
	FuncDeclList FL;
	
	auto F=FL.fromYAML(Filename);		
	if (std::error_code ec = F.getError()){
	       	errs()<<"Error when opening "<< Filename<<": ";	
		errs()<<ec.message();
	}
	else 
		FL=F.get();

	return FL;
}


map<string,vector<int>> getInputArgMap(FuncDeclList& FL){
	map<string,vector<int>> InputArgMap;

	for (const auto& FD:FL.List) {
		vector<int> ArgVec;
		for (auto it=FD.ParamVec.begin(); it != FD.ParamVec.end(); ++it) {
			auto Annotations=it->Annotations;
			if (find(Annotations.begin(), Annotations.end(),INPUT_ANNOTATION)!=Annotations.end()) {
				ArgVec.push_back(it-FD.ParamVec.begin());
			}
		}
	
		
		if (not ArgVec.empty()) {
			InputArgMap.insert(make_pair(FD.MangledName,ArgVec));
		}
	}

	return InputArgMap;
}
map<string,vector<int>> getOutputArgMap(FuncDeclList& FL) {
	map<string,vector<int>> OutputArgMap;

	for (const auto& FD:FL.List) {
		vector<int> ArgVec;
		for (auto it=FD.ParamVec.begin(); it != FD.ParamVec.end(); ++it) {
			auto Annotations=it->Annotations;
			if (find(Annotations.begin(), Annotations.end(),OUTPUT_ANNOTATION)!=Annotations.end()) {
				ArgVec.push_back(it-FD.ParamVec.begin());
			}
		}
	
		if (not ArgVec.empty()) {
			OutputArgMap.insert(make_pair(FD.MangledName,ArgVec));
		}
	}

	return OutputArgMap;
}

StringSet<> getExplicitSourceNames(FuncDeclList& FL) {
	StringSet<> SourceNames;

	for (const auto& FD:FL.List) {
		auto Annotations = FD.Annotations;
		if (find(Annotations.begin(), Annotations.end(),SOURCE_ANNO)!=Annotations.end()) {
			SourceNames.insert(FD.MangledName);
		}
	}
	return SourceNames;
}

StringSet<> getExplicitSinkNames(FuncDeclList& FL) {
	StringSet<> SinkNames;

	for (const auto& FD:FL.List) {
		auto Annotations = FD.Annotations;
		if (find(Annotations.begin(), Annotations.end(),SINK_ANNO)!=Annotations.end()) {
			SinkNames.insert(FD.MangledName);
		}
	}
	
	return SinkNames;
}

string vector2Str(vector<int> my_vector){
	stringstream result;
	copy(my_vector.begin(), my_vector.end(), std::ostream_iterator<int>(result, " "));
	return result.str().c_str();
}
