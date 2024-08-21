# This file contains common boilerplate that most users will want to include in their top level cmakelists

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

########################################################################################################################
# Build types

if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# always leave debug info on, we can strip separately if required
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3")

########################################################################################################################
# Helper for defining a post-build step

function(common_postbuild TGT)

    add_custom_command(TARGET ${TGT} POST_BUILD
        COMMAND ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/postbuild/${TARGET_MCU}.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TGT}
        COMMENT "Calculating flash usage")

endfunction()
