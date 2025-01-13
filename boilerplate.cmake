# This file contains common boilerplate that most users will want to include in their top level cmakelists

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

########################################################################################################################
# Build types

if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# always leave debug info on, we can strip separately if required
# define NDEBUG even in debug builds so newlib doesnt try to compile 10+ kB of asserts!!
add_compile_options("$<$<CONFIG:RELEASE>:-O3;-g>")
add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:-O3;-g>")
add_compile_options("$<$<CONFIG:DEBUG>:-Og;-D_DEBUG;-DNDEBUG;-g>")
add_compile_options("$<$<CONFIG:DEBUGNOOPT>:-O0;-D_DEBUG;-DNDEBUG;-g>")

########################################################################################################################
# Helper for defining post-build steps

function(common_postbuild TGT)

    add_custom_command(TARGET ${TGT} POST_BUILD
        COMMAND ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/postbuild/${TARGET_MCU}.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TGT}
        COMMENT "Calculating flash usage")

endfunction()
