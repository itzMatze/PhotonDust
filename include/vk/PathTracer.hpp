#pragma once

#include "vk/Pipeline.hpp"
#include "vk/DescriptorSetHandler.hpp"
#include "Storage.hpp"
#include "UI.hpp"

namespace ve
{
    class PathTracer
    {
    public:
        PathTracer(const VulkanMainContext& vmc, Storage& storage);
        void setup_storage(AppState& app_state);
        void construct(VulkanCommandContext& vcc);
        void destruct();
        void reload_shaders();
        void set_scene(uint32_t scene_texture_image_count, bool init);
        void compute(vk::CommandBuffer& cb, AppState& app_state, uint32_t read_only_image);
    private:
        const VulkanMainContext& vmc;
        Storage& storage;
        Pipeline pipeline;
        DescriptorSetHandler dsh;
        std::vector<uint32_t> path_trace_images;
        std::vector<uint32_t> path_trace_buffers;

        uint32_t scene_texture_count;

        struct PathTracerPushConstants
        {
            uint32_t sample_count = 0;
            uint32_t attenuation_view = 0;
            uint32_t emission_view = 0;
            uint32_t normal_view = 0;
            uint32_t tex_view = 0;
        } ptpc;

        void create_pipeline();
        void create_descriptor_set();
    };
} // namespace ve
