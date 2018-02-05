add_llvm_loadable_module(LLVMProv
	CallSemantics.cc
	FlowFinder.cc
	CallGraphPass.cc
	GraphFlowsPass.cc
	IFFactory.cc
	IFFactory-FreeBSD.cc
	PosixCallSemantics.cc
	ProvPass.cc
	DeclarationAnalysis.cc
	AnnotationCallSemantics.cc
	AnnotationsHelper.cc

	# Link explicitly against the library's full path, as CMake's normal
	# target_link_libraries() mechanism likes to change paths into -L/-l
	# flags that don't work properly with unusually-named libraries like
	# LLVMLoom.so (i.e., not libLLVMLoom.so).
	${LOOM_LIBRARY}
)
#target_link_libraries(LLVMProv libLLVMAnalysis.a)
