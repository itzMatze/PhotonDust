#pragma once

#include "Camera.hpp"
#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "FixVector.hpp"

namespace ve
{
    struct AppState {
        std::vector<const char*> scene_names;
        std::vector<float> devicetimings;
        std::vector<uint32_t> histogram;
        int32_t bin_count_per_channel = 128;
        bool bin_count_changed = false;
        int32_t histogram_update_rate = 50;
        vk::Extent2D render_extent = vk::Extent2D(1920, 1080);
        float aspect_ratio = float(render_extent.width) / float(render_extent.height);
        vk::Extent2D window_extent = vk::Extent2D(aspect_ratio * 1000, 1000);
        Camera cam = Camera(60.0f, aspect_ratio);
        float time_diff = 0.000001f;
        float time = 0.0f;
        int32_t current_scene = 0;
        uint32_t current_frame = 0;
        uint32_t total_frames = 0;
        uint32_t sample_count = 0;
        bool load_scene = false;
        bool show_ui = true;
        bool attenuation_view = false;
        bool emission_view = false;
        bool normal_view = false;
        bool tex_view = false;
        bool path_depth_view = false;
        bool save_screenshot = false;
        bool accumulate_samples = true;
        bool force_accumulate_samples = false;
        bool vsync = true;
        bool headless = false;
    };

    class UI
    {
    public:
        explicit UI(const VulkanMainContext& vmc);
        void construct(VulkanCommandContext& vcc, const RenderPass& render_pass, uint32_t frames);
        void destruct();
        void draw(vk::CommandBuffer& cb, AppState& app_state);
    private:
        const VulkanMainContext& vmc;
        vk::DescriptorPool imgui_pool;
        FixVector<float> frametime_values;
        float time_diff = 0.0f;
        std::vector<FixVector<float>> devicetiming_values;
        std::vector<float> devicetimings;
        int implot_custom_colormap;
    };
} // namespace ve
