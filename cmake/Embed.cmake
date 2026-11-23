# Helpers for the embed tool: resource and shader embedding. Each function
# registers a custom command and returns the generated header path through
# <out_var>.

find_program(GLSLC_PROGRAM glslc)
if(NOT GLSLC_PROGRAM)
    message(FATAL_ERROR
        "glslc was not found. It is included in the LunarG Vulkan SDK, and is "
        "required to compile the shaders. Install the SDK or set "
        "-DCMAKE_PROGRAM_PATH to a directory containing glslc.")
endif()

function(hg_embed_resource path out_var)
    set(src ${CMAKE_CURRENT_SOURCE_DIR}/${path})
    set(header ${CMAKE_CURRENT_BINARY_DIR}/${path}.h)
    get_filename_component(dir ${header} DIRECTORY)
    add_custom_command(
        OUTPUT ${header}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
        COMMAND embed ${src} ${path} ${header}
        DEPENDS embed ${src}
        COMMENT "Embedding ${path}"
    )
    set(${out_var} ${header} PARENT_SCOPE)
endfunction()

function(hg_compile_shader path out_var)
    set(src ${CMAKE_CURRENT_SOURCE_DIR}/${path})
    set(spv_dir ${CMAKE_CURRENT_BINARY_DIR}/${path})
    get_filename_component(spv_dir ${spv_dir} DIRECTORY)
    get_filename_component(name ${path} NAME)
    set(spv ${spv_dir}/${name}.spv)
    set(header ${spv}.h)
    add_custom_command(
        OUTPUT ${header}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${spv_dir}
        COMMAND ${GLSLC_PROGRAM} -o ${spv} ${src} -I${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMAND embed ${spv} ${name}.spv ${header}
        DEPENDS embed ${src}
            ${CMAKE_CURRENT_SOURCE_DIR}/include/hurdygurdy.glsl
        COMMENT "Compiling ${path}"
    )
    set(${out_var} ${header} PARENT_SCOPE)
endfunction()
