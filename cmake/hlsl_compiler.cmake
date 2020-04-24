# Compile HLSL files for given target.

# Function: target_shader_sources
# Usage: target_shader_sources(<target> <PRIVATE> [<file> <shader profile> ...])
function(target_shader_sources target_project)
    # Find the shader compiler
    find_program(FXC fxc DOC "DirectX Shader Compiler")
    if ("${FXC}" STREQUAL "FXC-NOTFOUND")
        message(FATAL_ERROR "[Error]: DirectX Shader Compiler not found")
    endif()
    message(STATUS "[Info]: Found DirectX Shader Compiler")

    # Parse the input
    cmake_parse_arguments(TARGET_SHADER_SOURCES "" "" "PRIVATE;PUBLIC" ${ARGN})

    set(FXC_FLAGS
        "/nologo"   # don't display copyright info
        "/Zi"       # include debugging info
        "/Od"       # disable optimizations
        "/Emain"    # entry point is always main()
    )
    
    set(IS_FILE ON)
    set(FXC_SOURCES)
    set(FXC_OUTPUTS)
    set(FXC_PROFILES)
    foreach(file_profile ${TARGET_SHADER_SOURCES_PRIVATE})
        if(IS_FILE)
            get_filename_component(file_abs ${file_profile} ABSOLUTE)
            list(APPEND FXC_SOURCES ${file_abs})

            get_filename_component(file_we ${file_profile} NAME_WE)
            list(APPEND FXC_OUTPUTS ${EXECUTABLE_OUTPUT_PATH}/${file_we}.cso)

            set(IS_FILE OFF)
        else()
            list(APPEND FXC_PROFILES ${file_profile})
            set(IS_FILE ON)
        endif()
    endforeach()

    foreach(src prof out IN ZIP_LISTS FXC_SOURCES FXC_PROFILES FXC_OUTPUTS)
        get_filename_component(file ${src} NAME)
        add_custom_command(
            TARGET ${target_project} POST_BUILD
            COMMAND ${FXC} ${FXC_FLAGS} /T ${prof} /Fo ${out} ${src}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Building Shader ${file}"
        )
    endforeach()
    
endfunction()