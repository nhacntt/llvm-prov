#ifndef ANNOTATIONS_HELPER_SEMANTICS_H
#define ANNOTATIONS_HELPER_SEMANTICS_H

#include "../include/ProvAnnotations.hh"

#include <llvm/Analysis/DeclarationAnalysis.h>
#include <llvm/ADT/StringSet.h>

#include <map>

using namespace std;
using namespace llvm;

string vector2Str(vector<int>);
FuncDeclList getFuncListfromYAML(string Filename);
map<string,vector<int>> getInputArgMap(FuncDeclList& FL);
map<string,vector<int>> getOutputArgMap(FuncDeclList& FL);
StringSet<> getExplicitSourceNames(FuncDeclList& FL);
StringSet<> getExplicitSinkNames(FuncDeclList& FL);

#endif /* ANNOTATIONS_HELPER_SEMANTICS_H */
