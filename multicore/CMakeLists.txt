add_library(common-embedded-platform-multicore STATIC
	IPCDescriptorTable.cpp
	MulticoreLogDevice.cpp
	)

target_include_directories(common-embedded-platform-multicore
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
