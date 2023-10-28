#pragma once

#include "vk/Pipeline.hpp"
#include "vk/DescriptorSetHandler.hpp"
#include "Storage.hpp"
#include "UI.hpp"

namespace ve
{
    class Histogram
    {
    public:
        Histogram(const VulkanMainContext& vmc, Storage& storage);
        void construct(AppState& app_state);
        void self_destruct();
        void compute(vk::CommandBuffer& cb, AppState& app_state, uint32_t read_only_image);
    private:
        const VulkanMainContext& vmc;
        Storage& storage;
        Pipeline pipeline;
        DescriptorSetHandler dsh;
        uint32_t histogram_buffer;

        void create_pipeline(uint32_t bin_count);
        void create_descriptor_set();
    };
} // namespace ve
