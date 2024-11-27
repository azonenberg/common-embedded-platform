file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CEP_ROOT)

if(NOT DEFINED CEP_BUILD_SERVICES)
  set(CEP_BUILD_SERVICES 1)
endif()

if(NOT DEFINED CEP_BUILD_SUPER)
  set(CEP_BUILD_SUPER 1)
endif()

if(NOT DEFINED CEP_BUILD_FPGA)
  set(CEP_BUILD_FPGA 0)
endif()

if(NOT DEFINED CEP_BUILD_BOILERPLATE)
  set(CEP_BUILD_BOILERPLATE 0)
endif()

add_subdirectory(bootloader)
add_subdirectory(core)

if(CEP_BUILD_SERVICES)
	add_subdirectory(services)
endif()

if(CEP_BUILD_SUPER)
	add_subdirectory(supervisor)
endif()

if(CEP_BUILD_FPGA)
	add_subdirectory(fpga)
endif()

if(CEP_BUILD_BOILERPLATE)
	add_subdirectory(boilerplate)
endif()
