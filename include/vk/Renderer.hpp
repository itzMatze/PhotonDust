#pragma once

#include "vk/Pipeline.hpp"
#include "vk/DescriptorSetHandler.hpp"
#include "Storage.hpp"
#include "UI.hpp"

namespace ve
{
    class Renderer
    {
    public:
        Renderer(const VulkanMainContext& vmc, Storage& storage);
        void construct(AppState& app_state, const RenderPass& render_pass);
        void self_destruct();
        void render(vk::CommandBuffer& cb, AppState& app_state, uint32_t read_only_image, const vk::Framebuffer& framebuffer, const vk::RenderPass& render_pass);
    private:
        const VulkanMainContext& vmc;
        Storage& storage;
        Pipeline pipeline;
        DescriptorSetHandler dsh;
        std::vector<uint32_t> render_textures;

        void create_pipeline(const RenderPass& render_pass);
        void create_descriptor_set();
    };
} // namespace ve
