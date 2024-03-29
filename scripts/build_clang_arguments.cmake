cmake_minimum_required(VERSION 3.8) # For conditional generator expression

function(putils_build_clang_arguments out_var target)
    # Build clang arguments
    set(clang_args)
    get_target_property(cxx_version ${target} CXX_STANDARD)
    list(APPEND clang_args -std=c++${cxx_version})
    list(APPEND clang_args ${ARGN})

    function(get_joined_property out_var property_name separator)
        set(property_value "$<TARGET_PROPERTY:${target},${property_name}>")
        set(cleaned_property_value "$<REMOVE_DUPLICATES:${property_value}>")

        set(property_condition "$<BOOL:${property_value}>")
        set(joined_property "${separator}$<JOIN:${cleaned_property_value}, ${separator}>")

        set(${out_var} "$<${property_condition}:${joined_property}>" PARENT_SCOPE)
    endfunction()

    get_joined_property(include_directories INCLUDE_DIRECTORIES -I)
    list(APPEND clang_args ${include_directories})
    get_joined_property(include_directories INTERFACE_INCLUDE_DIRECTORIES -I)
    list(APPEND clang_args ${include_directories})

    get_joined_property(compile_definitions COMPILE_DEFINITIONS -D)
    list(APPEND clang_args ${compile_definitions})
    get_joined_property(interface_compile_definitions INTERFACE_COMPILE_DEFINITIONS -D)
    list(APPEND clang_args ${interface_compile_definitions})

    # Get the export headers generated by putils_export_symbols.cmake
    set(processed)
    function(get_export_headers out_var_name current_target)
        if (NOT TARGET ${current_target})
            return()
        endif()

        set(out_list)

        get_target_property(export_file_name ${current_target} PUTILS_EXPORT_FILE_NAME)
        if(NOT "${export_file_name}" STREQUAL export_file_name-NOTFOUND)
            list(APPEND out_list ${export_file_name})
        endif()

        get_target_property(dependencies ${current_target} LINK_LIBRARIES)
        if("${dependencies}" STREQUAL dependencies-NOTFOUND)
            set(dependencies)
        endif()

        get_target_property(interface_dependencies ${current_target} INTERFACE_LINK_LIBRARIES)
        if(NOT "${interface_dependencies}" STREQUAL interface_dependencies-NOTFOUND)
            list(APPEND dependencies ${interface_dependencies})
        endif()

        list(REMOVE_DUPLICATES dependencies)

        foreach(dependency ${dependencies})
            if (${dependency} IN_LIST processed)
                continue()
            endif()
            list(APPEND processed ${dependency})
            set(processed ${processed} PARENT_SCOPE)

            get_export_headers(dependency_export_headers ${dependency})
            list(APPEND out_list ${dependency_export_headers})
        endforeach()

        list(REMOVE_DUPLICATES out_list)
        set(${out_var_name} ${out_list} PARENT_SCOPE)
    endfunction()

    # Force include export headers
    get_export_headers(export_headers ${target})
    foreach(header ${export_headers})
        list(APPEND clang_args -include ${header})
    endforeach()

    set(${out_var} ${clang_args} PARENT_SCOPE)
endfunction()