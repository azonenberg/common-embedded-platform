add_library(common-embedded-platform-scpi STATIC
	SCPITask.cpp
	)

target_include_directories(common-embedded-platform-scpi
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
