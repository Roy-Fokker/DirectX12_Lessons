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
        /nologo    # don't display copyright info
        /Zi        # include debugging info
        /Od        # disable optimizations
        /E main    # entry point is always main()
    )
    
    set(IS_FILE ON)     # a hacky means to alternate between
    set(FXC_SOURCES)    # source file name  and  
    set(FXC_PROFILES)   # profile to use when compiling
    set(FXC_OUTPUTS)    # output file list
    # Loop through the <file profile> pairs
    foreach(file_profile ${TARGET_SHADER_SOURCES_PRIVATE})
        # assume File is 1st param
        if(IS_FILE)
            # Get absolute path of this file and append to Sources list
            get_filename_component(file_abs ${file_profile} ABSOLUTE)
            list(APPEND FXC_SOURCES ${file_abs})

            # Get file name and append .cso extension, and add to Outputs List
            # Put the output file in same dir as executable.
            get_filename_component(file_we ${file_profile} NAME_WE)
            list(APPEND FXC_OUTPUTS ${EXECUTABLE_OUTPUT_PATH}/${file_we}.cso)

            # Flip flag for next item
            set(IS_FILE OFF)
        # profile is 2nd param
        else()
            # Append the profile to Profiles List
            list(APPEND FXC_PROFILES ${file_profile})
            set(IS_FILE ON)
        endif()
    endforeach()

    # Lenght of Source, Profile and Output Lists should match
    foreach(src prof out IN ZIP_LISTS FXC_SOURCES FXC_PROFILES FXC_OUTPUTS)
        get_filename_component(file ${src} NAME)
        # Call FXC and pass the parameters src, prof and out along with fxc_flags
        add_custom_command(
            TARGET ${target_project} POST_BUILD
            COMMAND ${FXC} ${FXC_FLAGS} /T ${prof} /Fo ${out} ${src}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Building Shader ${file}"
        )
    endforeach()
    
endfunction()