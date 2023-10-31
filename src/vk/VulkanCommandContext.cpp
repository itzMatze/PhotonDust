#include "vk/VulkanCommandContext.hpp"

#include "ve_log.hpp"

namespace ve
{
    VulkanCommandContext::VulkanCommandContext(const VulkanMainContext& vmc) : vmc(vmc), command_pools(TYPE_COUNT), one_time_cbs(TYPE_COUNT)
    {}

    void VulkanCommandContext::construct()
    {
        command_pools[GRAPHICS] = CommandPool(vmc.logical_device.get(), vmc.queue_family_indices.graphics);
        command_pools[COMPUTE] = CommandPool(vmc.logical_device.get(), vmc.queue_family_indices.compute);
        command_pools[TRANSFER] = CommandPool(vmc.logical_device.get(), vmc.queue_family_indices.transfer);
        one_time_cbs[GRAPHICS] = command_pools[GRAPHICS].create_command_buffers(1)[0];
        one_time_cbs[COMPUTE] = command_pools[COMPUTE].create_command_buffers(1)[0];
        one_time_cbs[TRANSFER] = command_pools[TRANSFER].create_command_buffers(1)[0];
        spdlog::info("Created VulkanCommandContext");
    }

    void VulkanCommandContext::destruct()
    {
        for (auto& command_pool : command_pools) command_pool.destruct();
        command_pools.clear();
        spdlog::info("Destroyed VulkanCommandContext");
    }

    void VulkanCommandContext::add_graphics_buffers(uint32_t count)
    {
        auto tmp = command_pools[GRAPHICS].create_command_buffers(count);
        graphics_cbs.insert(graphics_cbs.end(), tmp.begin(), tmp.end());
    }

    void VulkanCommandContext::add_compute_buffers(uint32_t count)
    {
        auto tmp = command_pools[COMPUTE].create_command_buffers(count);
        compute_cbs.insert(compute_cbs.end(), tmp.begin(), tmp.end());
    }

    void VulkanCommandContext::add_transfer_buffers(uint32_t count)
    {
        auto tmp = command_pools[TRANSFER].create_command_buffers(count);
        transfer_cbs.insert(transfer_cbs.end(), tmp.begin(), tmp.end());
    }

    vk::CommandBuffer& VulkanCommandContext::get_one_time_graphics_buffer() { return begin(one_time_cbs[GRAPHICS]); }

    vk::CommandBuffer& VulkanCommandContext::get_one_time_compute_buffer() { return begin(one_time_cbs[COMPUTE]); }

    vk::CommandBuffer& VulkanCommandContext::get_one_time_transfer_buffer() { return begin(one_time_cbs[TRANSFER]); }

    vk::CommandBuffer& VulkanCommandContext::begin(vk::CommandBuffer& cb)
    {
        vk::CommandBufferBeginInfo cbbi{};
        cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
        cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        cb.begin(cbbi);
        return cb;
    }

    void VulkanCommandContext::submit_graphics(const vk::CommandBuffer& cb, bool wait_idle) const
    {
        submit(cb, vmc.get_graphics_queue(), wait_idle);
    }

    void VulkanCommandContext::submit_compute(const vk::CommandBuffer& cb, bool wait_idle) const
    {
        submit(cb, vmc.get_compute_queue(), wait_idle);
    }

    void VulkanCommandContext::submit_transfer(const vk::CommandBuffer& cb, bool wait_idle) const
    {
        submit(cb, vmc.get_transfer_queue(), wait_idle);
    }

    void VulkanCommandContext::submit(const vk::CommandBuffer& cb, const vk::Queue& queue, bool wait_idle) const
    {
        cb.end();
        vk::SubmitInfo submit_info{};
        submit_info.sType = vk::StructureType::eSubmitInfo;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cb;
        queue.submit(submit_info);
        if (wait_idle) queue.waitIdle();
        cb.reset();
    }
} // namespace ve
