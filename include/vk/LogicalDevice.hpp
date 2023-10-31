#pragma once

#include <unordered_map>

#include "vk/common.hpp"
#include "vk/PhysicalDevice.hpp"

namespace ve
{
    enum class QueueIndex
    {
        Graphics,
        Compute,
        Transfer,
        Present
    };

    class LogicalDevice
    {
    public:
        LogicalDevice() = default;
        void construct(const PhysicalDevice& p_device, const QueueFamilyIndices& queue_family_indices, std::unordered_map<QueueIndex, vk::Queue>& queues);
        void destruct();
        const vk::Device& get() const;

    private:
        vk::Device device;
    };
} // namespace ve
