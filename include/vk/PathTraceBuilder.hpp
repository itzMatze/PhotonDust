#pragma once

#include <glm/mat4x4.hpp>
#include <vk/common.hpp>
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "Storage.hpp"

namespace ve
{
    struct AccelerationStructure {
        vk::AccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        uint32_t buffer;
        uint32_t scratch_buffer;
    };

    class PathTraceBuilder
    {
    public:
        PathTraceBuilder(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void destruct();
        uint32_t add_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, vk::DeviceSize vertex_stride);
        uint32_t add_instance(uint32_t blas_idx, const glm::mat4& M, uint32_t custom_index);
        void update_instance(uint32_t instance_idx, const glm::mat4& M);
        void create_tlas(vk::CommandBuffer& cb);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        vk::WriteDescriptorSetAccelerationStructureKHR wdsas;
        std::vector<AccelerationStructure> bottomLevelAS;
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        uint32_t instances_buffer;
        AccelerationStructure topLevelAS;

        void create_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, vk::DeviceSize vertex_stride, AccelerationStructure& blas);
    };
} // namespace ve
