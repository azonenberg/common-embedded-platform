if(NOT DEFINED CEP_BUILD_SUPER)
  set(CEP_NO_IBC 0)
endif()

if(CEP_NO_IBC)
	add_compile_definitions(NO_IBC=1)
endif()

add_library(common-embedded-platform-supervisor STATIC
	IBCRegisterReader.cpp
	PowerResetSupervisor.cpp
	SupervisorSPIServer.cpp
	TempSensorReader.cpp
	supervisor.cpp
	supervisor-bsp.cpp
	)

target_include_directories(common-embedded-platform-supervisor
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
