cmake_minimum_required(VERSION 3.23)
project(vulkan_experiments)

set(CMAKE_CXX_STANDARD 20)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

add_executable(vulkan_experiments main.cpp thirdParty/stb_image.h core/Application.cpp core/Application.hpp render/vulkan/VulkanBackend.cpp render/vulkan/VulkanBackend.hpp render/vulkan/VulkanPipelineBuilder.cpp render/vulkan/VulkanPipelineBuilder.hpp render/vulkan/VulkanBuffer.cpp render/vulkan/VulkanBuffer.hpp render/vulkan/VulkanMesh.cpp render/vulkan/VulkanMesh.hpp core/Mesh.hpp core/Shader.hpp render/vulkan/VulkanShader.cpp render/vulkan/VulkanShader.hpp)

add_subdirectory(${PROJECT_SOURCE_DIR}/thirdParty/glfw)
target_include_directories(
        ${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/thirdParty/glfw/include
        ${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/thirdParty/glm
        ${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/thirdParty/VulkanMemoryAllocator/include
        ${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/thirdParty/tinyobjloader
        ${CMAKE_PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(vulkan_experiments ${Vulkan_LIBRARY})

set(IMGUI_INCLUDE   ${PROJECT_SOURCE_DIR}/thirdParty/imgui/backends
        ${PROJECT_SOURCE_DIR}/thirdParty/imgui)

set(IMGUI_SRC       ${PROJECT_SOURCE_DIR}/thirdParty/imgui/imgui_draw.cpp
        ${PROJECT_SOURCE_DIR}/thirdParty/imgui/imgui_tables.cpp
        ${PROJECT_SOURCE_DIR}/thirdParty/imgui/imgui_widgets.cpp
        ${PROJECT_SOURCE_DIR}/thirdParty/imgui/imgui.cpp
        ${PROJECT_SOURCE_DIR}/thirdParty/imgui/backends/imgui_impl_glfw.cpp
        ${PROJECT_SOURCE_DIR}/thirdParty/imgui/backends/imgui_impl_vulkan.cpp)

add_library(imgui STATIC ${IMGUI_SRC})
set(CMAKE_EXE_LINKER_FLAGS " -static")
target_include_directories(imgui PUBLIC
        ${PROJECT_SOURCE_DIR}/thirdParty/glfw/include}
        ${Vulkan_INCLUDE_DIR}
        ${IMGUI_INCLUDE})
include_directories(${VULKAN_INCLUDE_DIR})
target_link_libraries(imgui glfw ${Vulkan_LIBRARY})

target_link_libraries(${CMAKE_PROJECT_NAME} -static-libgcc -static-libstdc++ winpthread
        Threads::Threads Vulkan::Vulkan
        ${GLFW_LIBRARIES}
        glfw
        imgui)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${IMGUI_INCLUDE})

#==============================================================================
# COMPILE SHADERS
#

set(SHADER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/assets/shaders)
set(SHADER_BINARY_DIR ${PROJECT_BINARY_DIR}/assets/shaders)
file(GLOB SHADERS
        ${SHADER_SOURCE_DIR}/*.vert
        ${SHADER_SOURCE_DIR}/*.frag
        ${SHADER_SOURCE_DIR}/*.comp
        ${SHADER_SOURCE_DIR}/*.geom
        ${SHADER_SOURCE_DIR}/*.tesc
        ${SHADER_SOURCE_DIR}/*.tese
        ${SHADER_SOURCE_DIR}/*.mesh
        ${SHADER_SOURCE_DIR}/*.task
        ${SHADER_SOURCE_DIR}/*.rgen
        ${SHADER_SOURCE_DIR}/*.rchit
        ${SHADER_SOURCE_DIR}/*.rmiss)

add_custom_command(
        COMMAND
        ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
        OUTPUT ${SHADER_BINARY_DIR}
        COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(source IN LISTS SHADERS)
    get_filename_component(FILENAME ${source} NAME)
    add_custom_command(
            COMMAND
            ${glslc_executable}
            #      -MD -MF ${SHADER_BINARY_DIR}/${FILENAME}.d
            -o ${SHADER_BINARY_DIR}/${FILENAME}.spv
            ${source}
            OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
            DEPENDS ${source} ${SHADER_BINARY_DIR}
            COMMENT "Compiling ${FILENAME}"
    )
    list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endForeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})