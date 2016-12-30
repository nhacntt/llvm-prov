add_llvm_loadable_module(LLVMProv
	OptPass.cc
)

target_link_libraries(LLVMProv ${LOOM_LIBRARY})
