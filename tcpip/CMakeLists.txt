add_library(common-embedded-platform-tcpip STATIC
	CommonTCPIP.cpp
	IPAgingTask.cpp
	PhyPollTask.cpp
	)

target_include_directories(common-embedded-platform-tcpip
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
