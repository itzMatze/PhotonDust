#include "vk/Renderer.hpp"

namespace ve
{
    Renderer::Renderer(const VulkanMainContext& vmc, Storage& storage) : vmc(vmc), storage(storage), pipeline(vmc), dsh(vmc, frames_in_flight)
    {}

    void Renderer::construct(AppState& app_state, const RenderPass& render_pass)
    {
        // set up textures that will be rendered
        std::vector<unsigned char> initial_image(app_state.render_extent.width * app_state.render_extent.height * 4, 0);
        render_textures.push_back(storage.add_named_image("render_texture_0", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc));
        render_textures.push_back(storage.add_named_image("render_texture_1", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc));
        create_descriptor_set();
        create_pipeline(render_pass);
    }

    void Renderer::self_destruct()
    {
        for (uint32_t i : render_textures) storage.destroy_image(i);
        render_textures.clear();
        pipeline.self_destruct();
        dsh.self_destruct();
    }

    void Renderer::create_pipeline(const RenderPass& render_pass)
    {
        std::vector<ShaderInfo> render_shader_infos(2);
        render_shader_infos[0] = ShaderInfo{"image.vert", vk::ShaderStageFlagBits::eVertex};
        render_shader_infos[1] = ShaderInfo{"image.frag", vk::ShaderStageFlagBits::eFragment};
        pipeline.construct(render_pass, dsh.get_layouts()[0], render_shader_infos, vk::PolygonMode::eFill, std::vector<vk::VertexInputBindingDescription>(), std::vector<vk::VertexInputAttributeDescription>(), vk::PrimitiveTopology::eTriangleList, {});
    }

    void Renderer::create_descriptor_set()
    {
        dsh.add_binding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            dsh.add_descriptor(i, 0, storage.get_image(render_textures[i]));
        }
        dsh.construct();
    }

    void Renderer::render(vk::CommandBuffer& cb, AppState& app_state, uint32_t read_only_image, const vk::Framebuffer& framebuffer, const vk::RenderPass& render_pass)
    {
        perform_image_layout_transition(cb, storage.get_image(render_textures[app_state.current_frame]).get_image(), vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite, 0, 1, 1);
        perform_image_layout_transition(cb, storage.get_image_by_name("path_trace_image_" + std::to_string(read_only_image)).get_image(), vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eTransferRead, 0, 1, 1);

        copy_image(cb, storage.get_image_by_name("path_trace_image_" + std::to_string(read_only_image)).get_image(), storage.get_image(render_textures[app_state.current_frame]).get_image(), app_state.render_extent.width, app_state.render_extent.height, 1);

        perform_image_layout_transition(cb, storage.get_image(render_textures[app_state.current_frame]).get_image(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead, 0, 1, 1);
        perform_image_layout_transition(cb, storage.get_image_by_name("path_trace_image_" + std::to_string(read_only_image)).get_image(), vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone, 0, 1, 1);

        vk::RenderPassBeginInfo rpbi{};
        rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
        rpbi.renderPass = render_pass;
        rpbi.framebuffer = framebuffer;
        rpbi.renderArea.offset = vk::Offset2D(0, 0);
        rpbi.renderArea.extent = app_state.window_extent;
        std::vector<vk::ClearValue> clear_values(2);
        clear_values[0].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;
        rpbi.clearValueCount = clear_values.size();
        rpbi.pClearValues = clear_values.data();
        cb.beginRenderPass(rpbi, vk::SubpassContents::eInline);

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = app_state.window_extent.width;
        viewport.height = app_state.window_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cb.setViewport(0, viewport);
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = app_state.window_extent;
        cb.setScissor(0, scissor);

        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_layout(), 0, {dsh.get_sets()[0]}, {});
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        cb.draw(3, 1, 0, 0);
    }
} // namespace ve
