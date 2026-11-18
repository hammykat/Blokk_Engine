# Locate DXC inside the Vulkan SDK
if(WIN32)
    set(DXC_EXECUTABLE "$ENV{VULKAN_SDK}/Bin/dxc.exe")
else()
    set(DXC_EXECUTABLE "$ENV{VULKAN_SDK}/bin/dxc")
endif()

if(NOT EXISTS "${DXC_EXECUTABLE}")
    message(FATAL_ERROR "Vulkan SDK DXC compiler not found at: ${DXC_EXECUTABLE}.")
endif()

# Resolves to Blokk/Engine/Systems/Rendering/Shaders
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../Rendering/Shaders")
set(COMPILED_SHADERS_DIR "${SHADER_SOURCE_DIR}/CompiledShaders")

file(MAKE_DIRECTORY "${COMPILED_SHADERS_DIR}")

file(GLOB_RECURSE HLSL_FILES CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/*.comp.hlsl")
file(GLOB_RECURSE HLSL_HEADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/*Bindings*.hlsl")

set(GENERATED_HEADERS "")
set(MASTER_HEADER_CONTENT "#pragma once\n\n// Auto-generated master shader header\n")

foreach(SRC_FILE ${HLSL_FILES})
    # Extracts "Velocities" from "Velocities.comp.hlsl"
    get_filename_component(BASE_NAME "${SRC_FILE}" NAME_WE)

    set(VAR_NAME "${BASE_NAME}Shader")
    set(HEADER_FILE_NAME "${VAR_NAME}.h")
    set(DST_HEADER "${COMPILED_SHADERS_DIR}/${HEADER_FILE_NAME}")

    # 1. Create the header right away if it doesn't exist yet (or if source is newer)
    if(NOT EXISTS "${DST_HEADER}" OR "${SRC_FILE}" IS_NEWER_THAN "${DST_HEADER}")
        execute_process(
            COMMAND "${DXC_EXECUTABLE}"
                -T cs_6_6
                -E main
                -spirv
                -Fh "${DST_HEADER}"
                -Vn "${VAR_NAME}"
                "${SRC_FILE}"
            RESULT_VARIABLE DXC_RESULT
            ERROR_VARIABLE DXC_ERROR
        )
        if(NOT DXC_RESULT EQUAL 0)
            message(WARNING "DXC failed on ${BASE_NAME}.comp.hlsl:\n${DXC_ERROR}")
        endif()
    endif()

    # 2. Keep it updated automatically on every subsequent Build when .hlsl files change
    add_custom_command(
        OUTPUT "${DST_HEADER}"
        DEPENDS "${SRC_FILE}" ${HLSL_HEADERS}
        COMMAND "${DXC_EXECUTABLE}"
            -T cs_6_6
            -E main
            -spirv
            -Fh "${DST_HEADER}"
            -Vn "${VAR_NAME}"
            "${SRC_FILE}"
        COMMENT "Compiling ${BASE_NAME} -> CompiledShaders/${HEADER_FILE_NAME}..."
        VERBATIM
    )

    list(APPEND GENERATED_HEADERS "${DST_HEADER}")
    string(APPEND MASTER_HEADER_CONTENT "#include \"CompiledShaders/${HEADER_FILE_NAME}\"\n")
endforeach()

file(WRITE "${SHADER_SOURCE_DIR}/ShaderBytecodes.h" "${MASTER_HEADER_CONTENT}")

add_custom_target(CompileShaders ALL DEPENDS ${GENERATED_HEADERS})
add_dependencies(Blokk CompileShaders)
target_include_directories(Blokk PUBLIC "${SHADER_SOURCE_DIR}")
