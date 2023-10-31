#pragma once

#include "vk/common.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class RenderPass
    {
    public:
        RenderPass(const VulkanMainContext& vmc);
        void construct(const vk::Format& color_format, const vk::Format& depth_format);
        vk::RenderPass get() const;
        void destruct();

        uint32_t attachment_count;
        
    private:
        const VulkanMainContext& vmc;
        vk::RenderPass render_pass;
    };
} // namespace ve
