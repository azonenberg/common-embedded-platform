file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CEP_ROOT)

if(NOT DEFINED CEP_BUILD_SERVICES)
  set(CEP_BUILD_SERVICES 1)
endif()

add_subdirectory(bootloader)
add_subdirectory(core)

if(CEP_BUILD_SERVICES)
	add_subdirectory(services)
endif()

add_subdirectory(supervisor)
