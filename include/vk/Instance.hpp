#pragma once

#include <vector>

#include "vk/common.hpp"
#include "vk/ExtensionsHandler.hpp"

namespace ve
{
    class Instance
    {
    public:
        Instance() = default;
        void construct(std::vector<const char*> required_extensions);
        void destruct();
        const vk::Instance& get() const;
        const std::vector<const char*>& get_missing_extensions() const;
        std::vector<vk::PhysicalDevice> get_physical_devices() const;

    private:
        vk::Instance instance;
        ExtensionsHandler extensions_handler;
        ExtensionsHandler validation_handler;
        vk::DynamicLoader dl;
    };
} // namespace ve
