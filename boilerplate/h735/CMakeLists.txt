add_library(common-embedded-platform-boilerplate-h735 STATIC
	StandardBSP.cpp
	)

target_include_directories(common-embedded-platform-boilerplate-h735
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
