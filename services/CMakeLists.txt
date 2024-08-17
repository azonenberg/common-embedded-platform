add_library(common-embedded-platform-services STATIC
	Iperf3Server.cpp
	STM32NTPClient.cpp
	)

target_include_directories(common-embedded-platform-services
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
