#include "WorkContext.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace ve
{
    WorkContext::WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc, AppState& app_state) : vmc(vmc), vcc(vcc), storage(vmc, vcc), swapchain(vmc, vcc, storage), scene(vmc, vcc, storage), ui(vmc), path_tracer(vmc, storage), renderer(vmc, storage), histogram(vmc, storage)
    {}

    void WorkContext::construct(AppState& app_state)
    {
        vcc.add_graphics_buffers(frames_in_flight);
        vcc.add_compute_buffers(1);
        vcc.add_transfer_buffers(1);
        path_tracer.setup_storage(app_state);
        renderer.setup_storage(app_state);
        histogram.setup_storage(app_state);
        swapchain.construct(app_state.vsync);
        app_state.window_extent = swapchain.get_extent();
        ui.construct(vcc, swapchain.get_render_pass(), frames_in_flight);
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            timers.emplace_back(vmc, vcc);
            syncs.emplace_back(vmc.logical_device.get());
        }

        // set up uniform buffer
        uniform_buffer = storage.add_named_buffer("uniform_buffer", sizeof(Camera::Data), vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.compute, vmc.queue_family_indices.transfer);

        path_tracer.construct(vcc);
        renderer.construct(swapchain.get_render_pass());
        histogram.construct(app_state);

        spdlog::info("Created WorkContext");
    }

    void WorkContext::destruct()
    {
        vmc.logical_device.get().waitIdle();
        for (auto& sync : syncs) sync.destruct();
        syncs.clear();
        for (auto& timer : timers) timer.destruct();
        timers.clear();
        storage.destroy_buffer(uniform_buffer);
        ui.destruct();
        scene.destruct();
        swapchain.destruct();
        renderer.destruct();
        histogram.destruct();
        path_tracer.destruct();
        spdlog::info("Destroyed WorkContext");
    }

    void WorkContext::reload_shaders()
    {
        vmc.logical_device.get().waitIdle();
        path_tracer.reload_shaders();
    }

    void WorkContext::load_scene(const std::string& filename)
    {
        bool init = !scene.loaded;
        HostTimer timer;
        vmc.logical_device.get().waitIdle();
        if (scene.loaded) scene.destruct();
        scene.load(std::string("../assets/scenes/") + filename);
        scene.construct();
        spdlog::info("Loading scene took: {} ms", (timer.elapsed<std::milli>()));
        path_tracer.set_scene(scene.get_texture_image_count(), init);
    }

    void WorkContext::draw_frame(AppState& app_state)
    {
        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), syncs[app_state.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        syncs[app_state.current_frame].wait_for_fence(Synchronization::F_RENDER_FINISHED);
        syncs[app_state.current_frame].reset_fence(Synchronization::F_RENDER_FINISHED);
        if (app_state.current_frame == 0)
        {
            app_state.cam.update_data();
            if (!app_state.force_accumulate_samples && (old_cam_data != app_state.cam.data || !app_state.accumulate_samples)) app_state.sample_count = 0;
            old_cam_data = app_state.cam.data;
            syncs[0].wait_for_fence(Synchronization::F_COMPUTE_FINISHED);
            syncs[0].reset_fence(Synchronization::F_COMPUTE_FINISHED);
            storage.get_buffer(uniform_buffer).update_data_bytes(&app_state.cam.data, sizeof(Camera::Data));
        }
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i)
        {
            double timing = timers[app_state.current_frame].get_result_by_idx(i);
            app_state.devicetimings[i] = timing;
        }
        uint32_t read_only_image = (app_state.total_frames / frames_in_flight) % frames_in_flight;
        if (app_state.save_screenshot)
        {
            storage.get_image_by_name("path_trace_image_" + std::to_string(read_only_image)).save_to_file(vcc);
            app_state.save_screenshot = false;
        }
        render(image_idx.value, read_only_image, app_state);
        app_state.current_frame = (app_state.current_frame + 1) % frames_in_flight;
        app_state.total_frames++;
    }

    vk::Extent2D WorkContext::recreate_swapchain(bool vsync)
    {
        vmc.logical_device.get().waitIdle();
        swapchain.recreate(vsync);
        return swapchain.get_extent();
    }

    void WorkContext::render(uint32_t image_idx, uint32_t read_only_image, AppState& app_state)
    {
        if (app_state.current_frame == 0)
        {
            vk::CommandBuffer& compute_cb = vcc.begin(vcc.compute_cbs[0]);
            path_tracer.compute(compute_cb, app_state, read_only_image);
            if (app_state.bin_count_changed)
            {
                app_state.bin_count_changed = false;
                histogram.destruct();
                histogram.setup_storage(app_state);
                histogram.construct(app_state);
            }
            if (app_state.sample_count % app_state.histogram_update_rate == 0) histogram.compute(compute_cb, app_state, read_only_image);
            compute_cb.end();
            app_state.sample_count++;
        }

        vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cbs[app_state.current_frame]);
        timers[app_state.current_frame].reset(cb, {DeviceTimer::RENDERING_ALL});
        timers[app_state.current_frame].start(cb, DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
        renderer.render(cb, app_state, read_only_image, swapchain.get_framebuffer(image_idx), swapchain.get_render_pass().get());
        if (app_state.show_ui) ui.draw(cb, app_state);
        cb.endRenderPass();
        timers[app_state.current_frame].stop(cb, DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
        cb.end();

        // submission
        if (app_state.current_frame == 0)
        {
            vk::SubmitInfo compute_si(0, nullptr, nullptr, 1, &vcc.compute_cbs[0]);
            vmc.get_compute_queue().submit(compute_si, syncs[0].get_fence(Synchronization::F_COMPUTE_FINISHED));
        }

        std::vector<vk::Semaphore> render_wait_semaphores;
        std::vector<vk::PipelineStageFlags> render_wait_stages;
        render_wait_semaphores.push_back(syncs[app_state.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        render_wait_stages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo render_si(render_wait_semaphores.size(), render_wait_semaphores.data(), render_wait_stages.data(), 1, &vcc.graphics_cbs[app_state.current_frame], 1, &syncs[app_state.current_frame].get_semaphore(Synchronization::S_RENDER_FINISHED));
        vmc.get_graphics_queue().submit(render_si, syncs[app_state.current_frame].get_fence(Synchronization::F_RENDER_FINISHED));

        vk::PresentInfoKHR present_info(1, &syncs[app_state.current_frame].get_semaphore(Synchronization::S_RENDER_FINISHED), 1, &swapchain.get(), &image_idx);
        VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
    }
} // namespace ve
