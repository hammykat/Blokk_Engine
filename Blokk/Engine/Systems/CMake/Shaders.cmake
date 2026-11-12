# Locate the Vulkan SDK DXC executable
if(DEFINED ENV{VULKAN_SDK})
    set(DXC_EXECUTABLE "$ENV{VULKAN_SDK}/Bin/dxc.exe")
else()
    set(DXC_EXECUTABLE "dxc")
endif()

# Set paths relative to the engine source directory
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Systems/Rendering/Shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders")

file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR}/Culling)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR}/Updates)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR}/Velocities)

# List all your HLSL shaders with their relative subfolders
set(HLSL_SHADERS
    Culling/Culling.comp.hlsl
    Updates/Deletion.comp.hlsl
    Velocities/Velocities.comp.hlsl
)

set(SPV_SHADERS "")

foreach(SHADER ${HLSL_SHADERS})
    set(SRC_FILE "${SHADER_SOURCE_DIR}/${SHADER}")
    
    string(REPLACE ".hlsl" ".spv" SPV_FILE_NAME ${SHADER})
    set(DST_FILE "${SHADER_OUTPUT_DIR}/${SPV_FILE_NAME}")

    add_custom_command(
        OUTPUT ${DST_FILE}
        DEPENDS ${SRC_FILE}
        COMMAND ${DXC_EXECUTABLE} 
            -T cs_6_0 
            -E main 
            -spirv 
            ${SRC_FILE} 
            -Fo ${DST_FILE}
        COMMENT "Compiling shader ${SHADER} to SPIR-V..."
        VERBATIM
    )

    list(APPEND SPV_SHADERS ${DST_FILE})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPV_SHADERS})