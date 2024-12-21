add_library(common-embedded-platform-core STATIC
	hardware-id.cpp
	TimerTask.cpp
	main.cpp
	)

target_include_directories(common-embedded-platform-core
	PUBLIC ${CEP_ROOT}
	PUBLIC "$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
