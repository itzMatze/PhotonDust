cmake_minimum_required(VERSION 3.19)
project(PathTracerGPU)
set(CMAKE_CXX_STANDARD 23)

set(SOURCE_FILES src/main.cpp src/Camera.cpp src/EventHandler.cpp src/Window.cpp src/UI.cpp
src/vk/CommandPool.cpp src/vk/DescriptorSetHandler.cpp src/vk/ExtensionsHandler.cpp
src/vk/Instance.cpp src/vk/LogicalDevice.cpp src/vk/PhysicalDevice.cpp
src/vk/Pipeline.cpp src/vk/RenderPass.cpp src/vk/Swapchain.cpp
src/vk/Shader.cpp src/vk/Synchronization.cpp src/vk/Image.cpp
src/vk/PathTracer.cpp
src/vk/Scene.cpp src/vk/Model.cpp src/vk/Mesh.cpp src/vk/Timer.cpp
src/vk/VulkanCommandContext.cpp src/vk/VulkanMainContext.cpp src/WorkContext.cpp src/Storage.cpp
"${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.9/imgui.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.9/imgui_draw.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.9/imgui_widgets.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.9/imgui_tables.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.9/backends/imgui_impl_vulkan.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.9/backends/imgui_impl_sdl2.cpp" "${PROJECT_SOURCE_DIR}/dependencies/implot-0.16/implot.cpp" "${PROJECT_SOURCE_DIR}/dependencies/implot-0.16/implot_items.cpp")

add_executable(PathTracerGPU ${SOURCE_FILES})
include_directories(PathTracerGPU PUBLIC "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator-3.0.1/include" "${PROJECT_SOURCE_DIR}/dependencies/tinygltf-2.8.18/" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.9/" "${PROJECT_SOURCE_DIR}/dependencies/implot-0.16/")

find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED)
find_program(GLSLC glslc REQUIRED)

target_link_libraries(PathTracerGPU SDL2::SDL2main SDL2::SDL2 ${Vulkan_LIBRARIES} spdlog::spdlog)
