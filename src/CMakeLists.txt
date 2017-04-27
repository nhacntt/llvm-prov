add_llvm_loadable_module(LLVMProv
	FlowFinder.cc
	IFFactory-FreeBSD.cc
	PosixCallSemantics.cc
	ProvPass.cc

	# Link explicitly against the library's full path, as CMake's normal
	# target_link_libraries() mechanism likes to change paths into -L/-l
	# flags that don't work properly with unusually-named libraries like
	# LLVMLoom.so (i.e., not libLLVMLoom.so).
	${LOOM_LIBRARY}
)
