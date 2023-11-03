#include "vk/PathTracer.hpp"

namespace ve
{
    PathTracer::PathTracer(const VulkanMainContext& vmc, Storage& storage) : vmc(vmc), storage(storage), pipeline(vmc), dsh(vmc, frames_in_flight)
    {}

    void PathTracer::setup_storage(AppState& app_state)
    {
        // set up images for path tracing
        std::vector<unsigned char> initial_image(app_state.render_extent.width * app_state.render_extent.height * 4, 0);
        path_trace_images.push_back(storage.add_named_image("path_trace_image_0", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc));
        path_trace_images.push_back(storage.add_named_image("path_trace_image_1", initial_image.data(), app_state.render_extent.width, app_state.render_extent.height, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc));

        // set up buffers for path tracing
        std::vector<float> initial_buffer_data(app_state.render_extent.width * app_state.render_extent.height * 4, 0);
        path_trace_buffers.push_back(storage.add_named_buffer("path_trace_buffer_0", initial_buffer_data, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute));
        path_trace_buffers.push_back(storage.add_named_buffer("path_trace_buffer_1", initial_buffer_data, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute));
        initial_buffer_data.resize(app_state.render_extent.width * app_state.render_extent.height, 0.0);
        path_depth_buffers.push_back(storage.add_named_buffer("path_depth_buffer_0", initial_buffer_data, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute));
        path_depth_buffers.push_back(storage.add_named_buffer("path_depth_buffer_1", initial_buffer_data, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute));
    }

    void PathTracer::construct(VulkanCommandContext& vcc)
    {
        for (uint32_t i : path_trace_images) storage.get_image(i).transition_image_layout(vcc, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eNone);
    }

    void PathTracer::destruct()
    {
        for (uint32_t i : path_trace_images) storage.destroy_image(i);
        path_trace_images.clear();
        for (uint32_t i : path_trace_buffers) storage.destroy_buffer(i);
        path_trace_buffers.clear();
        for (uint32_t i : path_depth_buffers) storage.destroy_buffer(i);
        path_depth_buffers.clear();
        pipeline.destruct();
        dsh.destruct();
    }

    void PathTracer::reload_shaders()
    {
        pipeline.destruct();
        create_pipeline();
    }

    void PathTracer::set_scene(uint32_t scene_texture_image_count, bool init)
    {
        scene_texture_count = scene_texture_image_count;
        if (!init)
        {
            dsh.destruct();
            pipeline.destruct();
        }
        create_descriptor_set();
        create_pipeline();
    }

    void PathTracer::compute(vk::CommandBuffer& cb, AppState& app_state, uint32_t read_only_image)
    {
        ptpc.attenuation_view = app_state.attenuation_view;
        ptpc.emission_view = app_state.emission_view;
        ptpc.normal_view = app_state.normal_view;
        ptpc.tex_view = app_state.tex_view;
        ptpc.path_depth_view = app_state.path_depth_view;
        if ((ptpc.attenuation_view | ptpc.emission_view | ptpc.normal_view | ptpc.tex_view) != 0) app_state.sample_count = 0;
        ptpc.sample_count = app_state.sample_count;
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeline.get_layout(), 0, dsh.get_sets()[read_only_image], {});
        cb.pushConstants(pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(PathTracerPushConstants), &ptpc);
        cb.dispatch((app_state.render_extent.width + 31) / 32, (app_state.render_extent.height + 31) / 32, 1);
    }

    void PathTracer::create_pipeline()
    {
        std::array<vk::SpecializationMapEntry, 2> path_tracer_entries;
        path_tracer_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        path_tracer_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        std::array<uint32_t, 2> path_tracer_entries_data{scene_texture_count, uint32_t(storage.get_buffer_by_name("emissive_mesh_indices").get_element_count())};
        vk::SpecializationInfo path_tracer_spec_info(path_tracer_entries.size(), path_tracer_entries.data(), sizeof(uint32_t) * path_tracer_entries_data.size(), path_tracer_entries_data.data());
        ShaderInfo path_tracer_shader_info = ShaderInfo{"path_trace.comp", vk::ShaderStageFlagBits::eFragment, path_tracer_spec_info};
        pipeline.construct(dsh.get_layouts()[0], path_tracer_shader_info, sizeof(PathTracerPushConstants));
    }

    void PathTracer::create_descriptor_set()
    {
        dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(1, vk::DescriptorType::eAccelerationStructureKHR, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(3, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(6, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(7, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(10, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(11, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(12, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(13, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(14, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(15, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(16, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute, scene_texture_count);
        dsh.add_binding(17, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            dsh.add_descriptor(i, 0, storage.get_buffer_by_name("uniform_buffer"));
            dsh.add_descriptor(i, 1, storage.get_buffer_by_name("tlas"));
            dsh.add_descriptor(i, 2, storage.get_image(path_trace_images[i]));
            dsh.add_descriptor(i, 3, storage.get_image(path_trace_images[1 - i]));
            dsh.add_descriptor(i, 4, storage.get_buffer(path_trace_buffers[i]));
            dsh.add_descriptor(i, 5, storage.get_buffer(path_trace_buffers[1 - i]));
            dsh.add_descriptor(i, 6, storage.get_buffer(path_depth_buffers[i]));
            dsh.add_descriptor(i, 7, storage.get_buffer(path_depth_buffers[1 - i]));
            dsh.add_descriptor(i, 10, storage.get_buffer_by_name("vertices"));
            dsh.add_descriptor(i, 11, storage.get_buffer_by_name("indices"));
            dsh.add_descriptor(i, 12, storage.get_buffer_by_name("materials"));
            dsh.add_descriptor(i, 13, storage.get_buffer_by_name("mesh_render_data"));
            dsh.add_descriptor(i, 14, storage.get_buffer_by_name("model_mrd_indices"));
            dsh.add_descriptor(i, 15, storage.get_buffer_by_name("emissive_mesh_indices"));
            std::vector<Image> images;
            for (uint32_t i = 0; i < scene_texture_count; ++i) images.push_back(storage.get_image_by_name("texture_" + std::to_string(i)));
            dsh.add_descriptor(i, 16, images);
            dsh.add_descriptor(i, 17, storage.get_buffer_by_name("lights"));
        }
        dsh.construct();
    }
} // namespace ve
