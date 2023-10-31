#include "vk/Histogram.hpp"

namespace ve
{
    Histogram::Histogram(const VulkanMainContext& vmc, Storage& storage) : vmc(vmc), storage(storage), pipeline(vmc), dsh(vmc, frames_in_flight)
    {}

    void Histogram::setup_storage(AppState& app_state)
    {
        // set up histogram buffer
        app_state.histogram = std::vector<uint32_t>(app_state.bin_count_per_channel * 3, 0);
        histogram_buffer = storage.add_named_buffer("histogram_buffer", app_state.histogram, vk::BufferUsageFlagBits::eStorageBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute);
    }

    void Histogram::construct(AppState& app_state)
    {
        create_descriptor_set();
        create_pipeline(app_state.histogram.size());
    }

    void Histogram::destruct()
    {
        storage.destroy_buffer(histogram_buffer);
        pipeline.destruct();
        dsh.destruct();
    }

    void Histogram::compute(vk::CommandBuffer& cb, AppState& app_state, uint32_t read_only_image)
    {
        storage.get_buffer(histogram_buffer).obtain_all_data(app_state.histogram);
        storage.get_buffer(histogram_buffer).update_data_bytes(0, app_state.histogram.size() * sizeof(uint32_t));
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeline.get_layout(), 0, dsh.get_sets()[read_only_image], {});
        cb.dispatch((app_state.render_extent.width + 31) / 32, (app_state.render_extent.height + 31) / 32, 1);
    }

    void Histogram::create_pipeline(uint32_t bin_count)
    {
        std::array<vk::SpecializationMapEntry, 1> histogram_entries;
        histogram_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        std::array<uint32_t, 1> histogram_entries_data{bin_count};
        vk::SpecializationInfo histogram_spec_info(histogram_entries.size(), histogram_entries.data(), sizeof(uint32_t) * histogram_entries_data.size(), histogram_entries_data.data());
        ShaderInfo histogram_shader_info = ShaderInfo{"histogram.comp", vk::ShaderStageFlagBits::eFragment, histogram_spec_info};
        pipeline.construct(dsh.get_layouts()[0], histogram_shader_info, 0);
    }

    void Histogram::create_descriptor_set()
    {
        dsh.add_binding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            dsh.add_descriptor(i, 0, storage.get_image_by_name("path_trace_image_" + std::to_string(i)));
            dsh.add_descriptor(i, 1, storage.get_buffer(histogram_buffer));
        }
        dsh.construct();
    }
} // namespace ve
