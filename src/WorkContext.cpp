#include "WorkContext.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace ve
{
    WorkContext::WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc, AppState& app_state) : vmc(vmc), vcc(vcc), storage(vmc, vcc), swapchain(vmc, vcc, storage), scene(vmc, vcc, storage), ui(vmc, swapchain.get_render_pass(), frames_in_flight), render_pipeline(vmc), path_tracer_compute_pipeline(vmc), render_dsh(vmc), path_tracer_dsh(vmc)
    {
        vcc.add_graphics_buffers(frames_in_flight);
        vcc.add_compute_buffers(1);
        vcc.add_transfer_buffers(1);
        ui.upload_font_textures(vcc);
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            timers.emplace_back(vmc);
            syncs.emplace_back(vmc.logical_device.get());
        }
        swapchain.construct();
        app_state.window_extent = swapchain.get_extent();
        // set up images for path tracing
        std::vector<unsigned char> initial_image(app_state.render_extent.width * app_state.render_extent.height * 4, 0);
        path_trace_images.push_back(storage.add_named_image("path_trace_image_0", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc));
        path_trace_images.push_back(storage.add_named_image("path_trace_image_1", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc));
        for (uint32_t i : path_trace_images) storage.get_image(i).transition_image_layout(vcc, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eNone);

        // set up uniform buffer
        uniform_buffer = storage.add_named_buffer("uniform_buffer", sizeof(Camera::Data), vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.compute, vmc.queue_family_indices.transfer);

        // set up buffers for path tracing

        // set up textures that will be rendered
        render_textures.push_back(storage.add_named_image("render_texture_0", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc));
        render_textures.push_back(storage.add_named_image("render_texture_1", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc));
        create_render_descriptor_set();
        create_render_pipeline();
        spdlog::info("Created WorkContext");
    }

    void WorkContext::self_destruct()
    {
        vmc.logical_device.get().waitIdle();
        for (auto& sync : syncs) sync.self_destruct();
        syncs.clear();
        for (auto& timer : timers) timer.self_destruct();
        timers.clear();
        for (uint32_t i : render_textures) storage.destroy_image(i);
        render_textures.clear();
        for (uint32_t i : path_trace_images) storage.destroy_image(i);
        path_trace_images.clear();
        storage.destroy_buffer(uniform_buffer);
        ui.self_destruct();
        scene.self_destruct();
        swapchain.self_destruct(true);
        render_pipeline.self_destruct();
        path_tracer_compute_pipeline.self_destruct();
        render_dsh.self_destruct();
        path_tracer_dsh.self_destruct();
        spdlog::info("Destroyed WorkContext");
    }

    void WorkContext::reload_shaders()
    {
        vmc.logical_device.get().waitIdle();
        path_tracer_compute_pipeline.self_destruct();
        create_path_tracer_pipeline();
    }

    void WorkContext::load_scene(const std::string& filename)
    {
        HostTimer timer;
        vmc.logical_device.get().waitIdle();
        if (scene.loaded)
        {
            scene.self_destruct();
            path_tracer_dsh.self_destruct();
            path_tracer_compute_pipeline.self_destruct();
        }
        scene.load(std::string("../assets/scenes/") + filename);
        scene.construct();
        spdlog::info("Loading scene took: {} ms", (timer.elapsed()));
        create_path_tracer_descriptor_set();
        create_path_tracer_pipeline();
    }

    void WorkContext::create_render_pipeline()
    {
        std::vector<ShaderInfo> render_shader_infos(2);
        render_shader_infos[0] = ShaderInfo{"image.vert", vk::ShaderStageFlagBits::eVertex};
        render_shader_infos[1] = ShaderInfo{"image.frag", vk::ShaderStageFlagBits::eFragment};
        render_pipeline.construct(swapchain.get_render_pass(), render_dsh.get_layouts()[0], render_shader_infos, vk::PolygonMode::eFill, std::vector<vk::VertexInputBindingDescription>(), std::vector<vk::VertexInputAttributeDescription>(), vk::PrimitiveTopology::eTriangleList, {});
    }

    void WorkContext::create_render_descriptor_set()
    {
        render_dsh.add_binding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            render_dsh.new_set();
            render_dsh.add_descriptor(0, storage.get_image(render_textures[i]));
        }
        render_dsh.construct();
    }

    void WorkContext::create_path_tracer_pipeline()
    {
        std::array<vk::SpecializationMapEntry, 1> path_tracer_entries;
        std::array<uint32_t, 1> path_tracer_entries_data{0};
        vk::SpecializationInfo path_tracer_spec_info(path_tracer_entries.size(), path_tracer_entries.data(), sizeof(uint32_t) * path_tracer_entries_data.size(), path_tracer_entries_data.data());
        ShaderInfo path_tracer_shader_info = ShaderInfo{"path_trace.comp", vk::ShaderStageFlagBits::eFragment, path_tracer_spec_info};
        path_tracer_compute_pipeline.construct(path_tracer_dsh.get_layouts()[0], path_tracer_shader_info, sizeof(PathTracerPushConstants));
    }

    void WorkContext::create_path_tracer_descriptor_set()
    {
        path_tracer_dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(1, vk::DescriptorType::eAccelerationStructureKHR, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(3, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(10, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(11, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(12, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(13, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(14, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(15, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute);
        path_tracer_dsh.add_binding(16, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            path_tracer_dsh.new_set();
            path_tracer_dsh.add_descriptor(0, storage.get_buffer(uniform_buffer));
            path_tracer_dsh.add_descriptor(1, storage.get_buffer_by_name("tlas"));
            path_tracer_dsh.add_descriptor(2, storage.get_image(path_trace_images[i]));
            path_tracer_dsh.add_descriptor(3, storage.get_image(path_trace_images[1 - i]));
            path_tracer_dsh.add_descriptor(10, storage.get_buffer_by_name("vertices"));
            path_tracer_dsh.add_descriptor(11, storage.get_buffer_by_name("indices"));
            path_tracer_dsh.add_descriptor(12, storage.get_buffer_by_name("materials"));
            path_tracer_dsh.add_descriptor(13, storage.get_buffer_by_name("mesh_render_data"));
            path_tracer_dsh.add_descriptor(14, storage.get_buffer_by_name("model_mrd_indices"));
            path_tracer_dsh.add_descriptor(15, storage.get_image_by_name("textures"));
            path_tracer_dsh.add_descriptor(16, storage.get_buffer_by_name("lights"));
        }
        path_tracer_dsh.construct();
    }

    void WorkContext::draw_frame(AppState& app_state)
    {
        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), syncs[app_state.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        syncs[app_state.current_frame].wait_for_fence(Synchronization::F_RENDER_FINISHED);
        syncs[app_state.current_frame].reset_fence(Synchronization::F_RENDER_FINISHED);
        if (app_state.current_frame == 0)
        {
            syncs[0].wait_for_fence(Synchronization::F_COMPUTE_FINISHED);
            syncs[0].reset_fence(Synchronization::F_COMPUTE_FINISHED);
            app_state.cam.data.pos = app_state.cam.getPosition();
            app_state.cam.data.u = app_state.cam.getRight();
            app_state.cam.data.v = app_state.cam.getUp();
            app_state.cam.data.w = app_state.cam.getFront();
            storage.get_buffer(uniform_buffer).update_data_bytes(&app_state.cam.data, sizeof(Camera::Data));
        }
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i)
        {
            double timing = timers[app_state.current_frame].get_result_by_idx(i);
            app_state.devicetimings[i] = timing;
        }
        if (app_state.save_screenshot)
        {
            app_state.save_screenshot = false;
        }
        render(image_idx.value, app_state);
        app_state.current_frame = (app_state.current_frame + 1) % frames_in_flight;
        app_state.total_frames++;
    }

    vk::Extent2D WorkContext::recreate_swapchain()
    {
        vmc.logical_device.get().waitIdle();
        swapchain.self_destruct(false);
        swapchain.construct();
        render_pipeline.self_destruct();
        path_tracer_compute_pipeline.self_destruct();
        create_render_pipeline();
        create_path_tracer_pipeline();
        return swapchain.get_extent();
    }

    void WorkContext::render(uint32_t image_idx, AppState& app_state)
    {
        uint32_t read_only_image = (app_state.total_frames / frames_in_flight) % frames_in_flight;
        if (app_state.current_frame == 0)
        {
            vk::CommandBuffer& compute_cb = vcc.begin(vcc.compute_cb[0]);
            compute_cb.bindPipeline(vk::PipelineBindPoint::eCompute, path_tracer_compute_pipeline.get());
            compute_cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, path_tracer_compute_pipeline.get_layout(), 0, path_tracer_dsh.get_sets()[read_only_image], {});
            // compute_cb.pushConstants(path_tracer_compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(PathTracerPushConstants), &ptpc);
            compute_cb.dispatch((app_state.render_extent.width + 31) / 32, (app_state.render_extent.height + 31) / 32, 1);
            compute_cb.end();
        }

        vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[app_state.current_frame]);
        perform_image_layout_transition(cb, storage.get_image(render_textures[app_state.current_frame]).get_image(), vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite, 0, 1, 1);
        perform_image_layout_transition(cb, storage.get_image(path_trace_images[read_only_image]).get_image(), vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eTransferRead, 0, 1, 1);

        copy_image(cb, storage.get_image(path_trace_images[read_only_image]).get_image(), storage.get_image(render_textures[app_state.current_frame]).get_image(), app_state.render_extent.width, app_state.render_extent.height, 1);

        perform_image_layout_transition(cb, storage.get_image(render_textures[app_state.current_frame]).get_image(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead, 0, 1, 1);
        perform_image_layout_transition(cb, storage.get_image(path_trace_images[read_only_image]).get_image(), vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone, 0, 1, 1);

        timers[app_state.current_frame].reset(cb, {DeviceTimer::RENDERING_ALL});
        timers[app_state.current_frame].start(cb, DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
        vk::RenderPassBeginInfo rpbi{};
        rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
        rpbi.renderPass = swapchain.get_render_pass().get();
        rpbi.framebuffer = swapchain.get_framebuffer(image_idx);
        rpbi.renderArea.offset = vk::Offset2D(0, 0);
        rpbi.renderArea.extent = swapchain.get_extent();
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
        viewport.width = swapchain.get_extent().width;
        viewport.height = swapchain.get_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cb.setViewport(0, viewport);
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = swapchain.get_extent();
        cb.setScissor(0, scissor);

        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.get_layout(), 0, {render_dsh.get_sets()[0]}, {});
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.get());
        cb.draw(3, 1, 0, 0);
        if (app_state.show_ui) ui.draw(cb, app_state);
        cb.endRenderPass();
        timers[app_state.current_frame].stop(cb, DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
        cb.end();

        // submission
        if (app_state.current_frame == 0)
        {
            vk::SubmitInfo compute_si{};
            compute_si.sType = vk::StructureType::eSubmitInfo;
            compute_si.waitSemaphoreCount = 0;
            compute_si.commandBufferCount = 1;
            compute_si.pCommandBuffers = &vcc.compute_cb[0];
            compute_si.signalSemaphoreCount = 0;
            vmc.get_compute_queue().submit(compute_si, syncs[0].get_fence(Synchronization::F_COMPUTE_FINISHED));
        }

        vk::SubmitInfo render_si;
        std::vector<vk::PipelineStageFlags> render_wait_stages;
        std::vector<vk::Semaphore> render_wait_semaphores;
        render_wait_stages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        render_wait_semaphores.push_back(syncs[app_state.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        render_si.sType = vk::StructureType::eSubmitInfo;
        render_si.waitSemaphoreCount = render_wait_semaphores.size();
        render_si.pWaitSemaphores = render_wait_semaphores.data();
        render_si.pWaitDstStageMask = render_wait_stages.data();
        render_si.commandBufferCount = 1;
        render_si.pCommandBuffers = &vcc.graphics_cb[app_state.current_frame];
        render_si.signalSemaphoreCount = 1;
        render_si.pSignalSemaphores = &syncs[app_state.current_frame].get_semaphore(Synchronization::S_RENDER_FINISHED);
        vmc.get_graphics_queue().submit(render_si, syncs[app_state.current_frame].get_fence(Synchronization::F_RENDER_FINISHED));

        vk::PresentInfoKHR present_info{};
        present_info.sType = vk::StructureType::ePresentInfoKHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &syncs[app_state.current_frame].get_semaphore(Synchronization::S_RENDER_FINISHED);
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain.get();
        present_info.pImageIndices = &image_idx;
        present_info.pResults = nullptr;
        VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
    }
} // namespace ve
