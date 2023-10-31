#pragma once

#include "vk/CommandPool.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class VulkanCommandContext
    {
    public:
        VulkanCommandContext(const VulkanMainContext& vmc);
        void construct();
        void destruct();
        void add_graphics_buffers(uint32_t count);
        void add_compute_buffers(uint32_t count);
        void add_transfer_buffers(uint32_t count);
        vk::CommandBuffer& get_one_time_graphics_buffer();
        vk::CommandBuffer& get_one_time_compute_buffer();
        vk::CommandBuffer& get_one_time_transfer_buffer();
        vk::CommandBuffer& begin(vk::CommandBuffer& cb);
        void submit_graphics(const vk::CommandBuffer& cb, bool wait_idle) const;
        void submit_compute(const vk::CommandBuffer& cb, bool wait_idle) const;
        void submit_transfer(const vk::CommandBuffer& cb, bool wait_idle) const;

        const VulkanMainContext& vmc;
        std::vector<CommandPool> command_pools;
        std::vector<vk::CommandBuffer> graphics_cbs;
        std::vector<vk::CommandBuffer> compute_cbs;
        std::vector<vk::CommandBuffer> transfer_cbs;
        std::vector<vk::CommandBuffer> one_time_cbs;

    private:
        enum Type
        {
            GRAPHICS = 0,
            COMPUTE = 1,
            TRANSFER = 2,
            TYPE_COUNT
        };

        void submit(const vk::CommandBuffer& cb, const vk::Queue& queue, bool wait_idle) const;
    };
} // namespace ve
