add_library(common-embedded-platform-cli STATIC
	CommonCommands.cpp
	)

target_include_directories(common-embedded-platform-cli
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
