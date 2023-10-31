#include "vk/RenderPass.hpp"

namespace ve
{
    RenderPass::RenderPass(const VulkanMainContext& vmc) : vmc(vmc)
    {}

    void RenderPass::construct(const vk::Format& color_format, const vk::Format& depth_format)
    {
        attachment_count = 1;
        vk::AttachmentDescription color_ad{};
        color_ad.format = color_format;
        color_ad.samples = vk::SampleCountFlagBits::e1;
        color_ad.loadOp = vk::AttachmentLoadOp::eClear;
        color_ad.storeOp = vk::AttachmentStoreOp::eStore;
        color_ad.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        color_ad.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        color_ad.initialLayout = vk::ImageLayout::eUndefined;
        color_ad.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentDescription depth_ad{};
        depth_ad.format = depth_format;
        depth_ad.samples = vk::SampleCountFlagBits::e1;
        depth_ad.loadOp = vk::AttachmentLoadOp::eClear;
        depth_ad.storeOp = vk::AttachmentStoreOp::eDontCare;
        depth_ad.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depth_ad.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depth_ad.initialLayout = vk::ImageLayout::eUndefined;
        depth_ad.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference color_ar{};
        color_ar.attachment = 0;
        color_ar.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depth_ar{};
        depth_ar.attachment = 1;
        depth_ar.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription spd{};
        spd.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        spd.colorAttachmentCount = 1;
        spd.pColorAttachments = &color_ar;
        spd.pDepthStencilAttachment = &depth_ar;

        std::array<vk::SubpassDependency, 1> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        std::vector<vk::AttachmentDescription> attachments{color_ad, depth_ad};
        vk::RenderPassCreateInfo rpci{};
        rpci.sType = vk::StructureType::eRenderPassCreateInfo;
        rpci.attachmentCount = attachments.size();
        rpci.pAttachments = attachments.data();
        rpci.subpassCount = 1;
        rpci.pSubpasses = &spd;
        rpci.dependencyCount = dependencies.size();
        rpci.pDependencies = dependencies.data();

        render_pass = vmc.logical_device.get().createRenderPass(rpci);
    }

    void RenderPass::destruct()
    {
        vmc.logical_device.get().destroyRenderPass(render_pass);
    }

    vk::RenderPass RenderPass::get() const
    {
        return render_pass;
    }
} // namespace ve
