cmake_minimum_required(VERSION 3.23)
project(vulkan_experiments)

set(CMAKE_CXX_STANDARD 20)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(vulkan_experiments main.cpp thirdParty/stb_image.h core/Application.cpp core/Application.hpp render/vulkan/VulkanBackend.cpp render/vulkan/VulkanBackend.hpp)

add_subdirectory(${PROJECT_SOURCE_DIR}/thirdParty/glfw)
target_include_directories(
        ${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/thirdParty/glfw/include
        ${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/thirdParty/glm
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