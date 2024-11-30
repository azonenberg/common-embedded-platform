add_library(common-embedded-platform-fpga STATIC
	AcceleratedCryptoEngine.cpp
	DeviceID.cpp
	Ethernet.cpp
	FMCUtils.cpp
	)

target_include_directories(common-embedded-platform-fpga
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
