add_library(common-embedded-platform-drivers STATIC
	TCA6424A.cpp
	VSC8512.cpp
	)

target_include_directories(common-embedded-platform-drivers
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
