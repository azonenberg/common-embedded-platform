add_library(common-embedded-platform-supervisor STATIC
	IBCRegisterReader.cpp
	PowerResetSupervisor.cpp
	TempSensorReader.cpp
	supervisor.cpp
	supervisor-bsp.cpp
	)

target_include_directories(common-embedded-platform-supervisor
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
