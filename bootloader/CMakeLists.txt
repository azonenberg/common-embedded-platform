add_library(common-embedded-platform-bootloader STATIC
	BootApp.S
	bootloader.cpp
	)

target_include_directories(common-embedded-platform-bootloader
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
