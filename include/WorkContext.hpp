#pragma once

#include <glm/mat4x4.hpp>
#include <vector>

#include "UI.hpp"
#include "vk/common.hpp"
#include "vk/Buffer.hpp"
#include "vk/Scene.hpp"
#include "vk/Swapchain.hpp"
#include "vk/Pipeline.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"
#include "Storage.hpp"
#include "vk/Timer.hpp"
#include "vk/DescriptorSetHandler.hpp"
#include "vk/Synchronization.hpp"

namespace ve
{
    class WorkContext
    {
    public:
        WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc, AppState& app_state);
        void self_destruct();
        void reload_shaders();
        void load_scene(const std::string& filename);
        void draw_frame(AppState& app_state);
        vk::Extent2D recreate_swapchain();

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage storage;
        Swapchain swapchain;
        Scene scene;
        UI ui;
        std::vector<Synchronization> syncs;
        std::vector<DeviceTimer> timers;
        Pipeline render_pipeline;
        Pipeline path_tracer_compute_pipeline;
        DescriptorSetHandler render_dsh;
        DescriptorSetHandler path_tracer_dsh;
        std::vector<uint32_t> render_textures;
        std::vector<uint32_t> path_trace_images;
        uint32_t uniform_buffer;

        struct PathTracerPushConstants
        {
            uint32_t sample_count = 0;
        } ptpc;

        void create_render_pipeline();
        void create_render_descriptor_set();
        void create_path_tracer_pipeline();
        void create_path_tracer_descriptor_set();
        void render(uint32_t image_idx, AppState& app_state);
    };
} // namespace ve
