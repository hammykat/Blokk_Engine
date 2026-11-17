# Locate DXC inside the Vulkan SDK
if(WIN32)
    set(DXC_EXECUTABLE "$ENV{VULKAN_SDK}/Bin/dxc.exe")
else()
    set(DXC_EXECUTABLE "$ENV{VULKAN_SDK}/bin/dxc")
endif()

if(NOT EXISTS "${DXC_EXECUTABLE}")
    message(FATAL_ERROR "Vulkan SDK DXC compiler not found at: ${DXC_EXECUTABLE}.")
endif()

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Rendering/Shaders")

# Output directly into a visible folder inside your source directory
set(SHADER_HEADER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Rendering/Shaders")

file(GLOB_RECURSE HLSL_FILES "${SHADER_SOURCE_DIR}/*.hlsl")

set(GENERATED_HEADERS "")
set(MASTER_HEADER_CONTENT "#pragma once\n\n// Auto-generated master shader header\n")

foreach(SRC_FILE ${HLSL_FILES})
    # Extracts "Velocities" from "Velocities.comp.hlsl"
    get_filename_component(BASE_NAME "${SRC_FILE}" NAME_WE)
    
    set(VAR_NAME "${BASE_NAME}Shader")
    set(HEADER_FILE_NAME "${VAR_NAME}.h")
    set(DST_HEADER "${SHADER_HEADER_OUTPUT_DIR}/${HEADER_FILE_NAME}")

    add_custom_command(
        OUTPUT "${DST_HEADER}"
        DEPENDS "${SRC_FILE}"
        COMMAND "${DXC_EXECUTABLE}"
            -T cs_6_6
            -E main
            -spirv
            -Fh "${DST_HEADER}"
            -Vn "${VAR_NAME}"
            "${SRC_FILE}"
        COMMENT "Compiling ${BASE_NAME} -> ${HEADER_FILE_NAME}..."
    )

    list(APPEND GENERATED_HEADERS "${DST_HEADER}")
    # Append an #include line for this shader to the master header string
    string(APPEND MASTER_HEADER_CONTENT "#include \"${HEADER_FILE_NAME}\"\n")
endforeach()

# Automatically write the master ShaderBytecodes.h file into your source folder
file(WRITE "${SHADER_HEADER_OUTPUT_DIR}/ShaderBytecodes.h" "${MASTER_HEADER_CONTENT}")

add_custom_target(CompileShaders ALL DEPENDS ${GENERATED_HEADERS})
target_include_directories(Blokk PRIVATE "${SHADER_HEADER_OUTPUT_DIR}")