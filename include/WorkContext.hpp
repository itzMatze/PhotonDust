#pragma once

#include <glm/mat4x4.hpp>
#include <vector>

#include "UI.hpp"
#include "vk/common.hpp"
#include "vk/Buffer.hpp"
#include "vk/Scene.hpp"
#include "vk/Swapchain.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"
#include "Storage.hpp"
#include "vk/Timer.hpp"
#include "vk/PathTracer.hpp"
#include "vk/Renderer.hpp"
#include "vk/Histogram.hpp"
#include "vk/Synchronization.hpp"

namespace ve
{
    class WorkContext
    {
    public:
        WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc, AppState& app_state);
        void construct(AppState& app_state);
        void destruct();
        void reload_shaders();
        void load_scene(const std::string& filename);
        void draw_frame(AppState& app_state);
        vk::Extent2D recreate_swapchain(bool vsync);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage storage;
        Swapchain swapchain;
        Scene scene;
        UI ui;
        std::vector<Synchronization> syncs;
        std::vector<DeviceTimer> timers;
        PathTracer path_tracer;
        Renderer renderer;
        Histogram histogram;
        uint32_t uniform_buffer;
        Camera::Data old_cam_data;

        void create_histogram_pipeline(uint32_t bin_count);
        void create_histogram_descriptor_set();
        void render(uint32_t image_idx, uint32_t read_only_image, AppState& app_state);
    };
} // namespace ve
