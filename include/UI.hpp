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
        vk::Extent2D render_extent;
        vk::Extent2D window_extent;
        float aspect_ratio;
        Camera cam;
        float time_diff = 0.000001f;
        float time = 0.0f;
        int32_t current_scene = 0;
        uint32_t current_frame = 0;
        uint32_t total_frames = 0;
        bool load_scene = false;
        bool show_ui = true;
        bool mesh_view = false;
        bool normal_view = false;
        bool color_view = false;
        bool tex_view = false;
        bool save_screenshot = false;
        bool accumulate_samples = true;
        bool force_accumulate_samples = false;
    };

    class UI
    {
	public:
		UI(const VulkanMainContext& vmc, const RenderPass& render_pass, uint32_t frames);
        void self_destruct();
		void upload_font_textures(VulkanCommandContext& vcc);
		void draw(vk::CommandBuffer& cb, AppState& app_state);
	private:
        const VulkanMainContext& vmc;
		vk::DescriptorPool imgui_pool;
        FixVector<float> frametime_values;
        float time_diff = 0.0f;
        std::vector<FixVector<float>> devicetiming_values;
        std::vector<float> devicetimings;
	};
} // namespace ve
