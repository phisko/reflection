include(${CMAKE_CURRENT_LIST_DIR}/build_clang_arguments.cmake)

function(putils_generate_reflection_headers)
    set(options)
    set(oneValueArgs TARGET EXTENSION)
    set(multiValueArgs SOURCES CLANG_ARGS)
    cmake_parse_arguments(ARGUMENTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(arg_name TARGET SOURCES)
        if(NOT ARGUMENTS_${arg_name})
            message(FATAL_ERROR "Missing ${arg_name} argument to putils_generate_reflection_headers")
        endif()
    endforeach()

    set(python_script ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generate_reflection_headers.py)
    putils_build_clang_arguments(clang_args ${ARGUMENTS_TARGET} ${ARGUMENTS_CLANG_ARGS})

    # Add a command for each reflection header we need to generate
    # This lets us have "atomic" commands that only run when their specific source file is modified
    # Each command will create a "time_marker_file" used as a timestamp for the last time reflection code was generated
    set(time_marker_files)
    foreach(source_file ${ARGUMENTS_SOURCES})
        set(command python ${python_script} ${source_file} --clang-args ${clang_args})
        if (${ARGUMENTS_EXTENSION})
            list(APPEND command --extension ${ARGUMENTS_EXTENSION})
        endif()

        # Dummy file to avoid re-running the command if `source_file` hasn't changed
        get_target_property(binary_dir ${ARGUMENTS_TARGET} BINARY_DIR)
        get_target_property(source_dir ${ARGUMENTS_TARGET} SOURCE_DIR)
        file(RELATIVE_PATH header_relative_path ${source_dir} ${source_file})
        set(time_marker_file ${binary_dir}/${header_relative_path}.generated_reflection)

        get_filename_component(time_marker_directory ${time_marker_file} DIRECTORY)

        add_custom_command(
                OUTPUT ${time_marker_file}
                COMMENT "Generating reflection code for ${source_file}"
                COMMAND ${command}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${time_marker_directory}
                COMMAND ${CMAKE_COMMAND} -E touch ${time_marker_file}
                DEPENDS ${source_file} ${python_script}
        )
        list(APPEND time_marker_files ${time_marker_file})
    endforeach()

    # Create a new target depending on all the time marker files, and depended upon by `ARGUMENTS_TARGET`
    set(reflection_target ${ARGUMENTS_TARGET}_reflection)
    add_custom_target(
            ${reflection_target}
            COMMENT "Generated reflection code for ${ARGUMENTS_TARGET}"
            DEPENDS ${time_marker_files}
    )
    add_dependencies(${ARGUMENTS_TARGET} ${reflection_target})
endfunction()