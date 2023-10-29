#include "UI.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl2.h"
#include "implot.h"
#include "implot_internal.h"

#include "ve_log.hpp"
#include "vk/Timer.hpp"

namespace ve
{
    constexpr uint32_t plot_value_count = 1024;
    constexpr float update_weight = 0.1f;
    constexpr ImU32 color_data[3] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };

    UI::UI(const VulkanMainContext& vmc, const RenderPass& render_pass, uint32_t frames) : vmc(vmc), frametime_values(plot_value_count, 0.0f), devicetimings(DeviceTimer::TIMER_COUNT, 0.0f)
    {
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i) devicetiming_values.push_back(FixVector<float>(plot_value_count, 0.0f));

        std::vector<vk::DescriptorPoolSize> pool_sizes =
        {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        };

        vk::DescriptorPoolCreateInfo dpci{};
        dpci.sType = vk::StructureType::eDescriptorPoolCreateInfo;
        dpci.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        dpci.maxSets = 1000;
        dpci.poolSizeCount = pool_sizes.size();
        dpci.pPoolSizes = pool_sizes.data();

        imgui_pool = vmc.logical_device.get().createDescriptorPool(dpci);

        ImGui::CreateContext();
        ImPlot::CreateContext();
        implot_custom_colormap = ImPlot::AddColormap("RGBColors", color_data, 32);

        //this initializes imgui for SDL
        ImGui_ImplSDL2_InitForVulkan(vmc.window.value().get());

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo ii{};
        ii.Instance = vmc.instance.get();
        ii.PhysicalDevice = vmc.physical_device.get();
        ii.Device = vmc.logical_device.get();
        ii.Queue = vmc.get_graphics_queue();
        ii.DescriptorPool = imgui_pool;
        ii.MinImageCount = frames;
        ii.ImageCount = frames;
        ii.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&ii, render_pass.get());
        ImGui::StyleColorsDark();
    }

    void UI::self_destruct()
    {
        vmc.logical_device.get().destroyDescriptorPool(imgui_pool);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    void UI::upload_font_textures(VulkanCommandContext& vcc)
    {
        vk::CommandBuffer cb = vcc.get_one_time_graphics_buffer();
        ImGui_ImplVulkan_CreateFontsTexture(cb);
        vcc.submit_graphics(cb, true);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void UI::draw(vk::CommandBuffer& cb, AppState& app_state)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(vmc.window.value().get());
        ImGui::NewFrame();
        ImGui::Begin("Vulkan_Engine");
        if (ImGui::CollapsingHeader("Navigation"))
        {
            ImGui::Text("'W'A'S'D'Q'E': movement");
            ImGui::Text("Mouse_L || Arrow-Keys: panning");
            ImGui::Text("'+'-': change movement speed");
            ImGui::Text("'R': reload shaders");
            ImGui::Text("'V': toggle VSync");
            ImGui::Text("'G': Show/Hide UI");
            ImGui::Text("'F1': Screenshot");
        }
        // scene
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Scene");
        ImGui::Combo("Scene", &app_state.current_scene, app_state.scene_names.data(), app_state.scene_names.size());
        app_state.load_scene |= ImGui::Button("Load scene");
        // camera
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Camera");
        ImGui::DragFloat("Camera sensor width", &app_state.cam.data.sensor_size.x, 0.001f, 0.001f, 0.05f);
        app_state.cam.data.sensor_size.y = app_state.cam.data.sensor_size.x / app_state.aspect_ratio;
        ImGui::DragFloat("Camera focal length", &app_state.cam.data.focal_length, 0.001f, 0.001f, 0.5f);
        ImGui::DragFloat("Exposure", &app_state.cam.data.exposure, 0.1f, 0.0f, 50.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
        app_state.bin_count_changed |= ImGui::SliderInt("Bin count", &app_state.bin_count_per_channel, 1, 512);
        ImGui::SliderInt("Histogram update rate", &app_state.histogram_update_rate, 1, 512);
        if (ImPlot::BeginPlot("Histogram"))
        {
            ImPlot::PushColormap(implot_custom_colormap);
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
            // set y-axis maximum with maximum histogram value, excluding 0 and maximum brightness
            uint32_t max = 0;
            for (uint32_t i = 1; i < app_state.histogram.size() - 1; ++i)
            {
                if (i % app_state.bin_count_per_channel != 0 && (i + 1) % app_state.bin_count_per_channel != 0 && app_state.histogram[i] > max) max = app_state.histogram[i];
            }
            ImPlot::SetupAxesLimits(0.0, 1.0, 0.0, max, ImPlotCond_Always);
            ImPlot::SetupAxes("Brightness", "Count", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines);
            ImPlot::PlotLine("R", app_state.histogram.data(), app_state.bin_count_per_channel, 1.0 / app_state.bin_count_per_channel);
            ImPlot::PlotLine("G", app_state.histogram.data() + app_state.bin_count_per_channel, app_state.bin_count_per_channel, 1.0 / app_state.bin_count_per_channel);
            ImPlot::PlotLine("B", app_state.histogram.data() + app_state.bin_count_per_channel * 2, app_state.bin_count_per_channel, 1.0 / app_state.bin_count_per_channel);
            ImPlot::PopColormap();
            ImPlot::EndPlot();
        }
        // debug views
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Debug Views");
        ImGui::Checkbox("Attenuation view", &(app_state.attenuation_view));
        ImGui::Checkbox("Emission view", &(app_state.emission_view));
        ImGui::Checkbox("Normal view", &(app_state.normal_view));
        ImGui::Checkbox("Texel view", &(app_state.tex_view));
        // path tracing
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Path Tracing");
        ImGui::Checkbox("Accumulate samples", &app_state.accumulate_samples);
        ImGui::Checkbox("Force accumulate samples", &app_state.force_accumulate_samples);
        ImGui::Text((std::string("VSync: ") + (app_state.vsync ? std::string("on") : std::string("off"))).c_str());
        ImGui::Text((std::string("Sample count: ") + std::to_string(app_state.sample_count)).c_str());
        time_diff = time_diff * (1 - update_weight) + app_state.time_diff * update_weight;
        frametime_values.push_back(app_state.time_diff);
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i)
        {
            if (!std::signbit(app_state.devicetimings[i])) 
            {
                devicetimings[i] = devicetimings[i] * (1 - update_weight) + app_state.devicetimings[i] * update_weight;
                devicetiming_values[i].push_back(app_state.devicetimings[i]);
            }
        }
        ImGui::Text((ve::to_string(time_diff * 1000, 4) + " ms; FPS: " + ve::to_string(1.0 / time_diff)).c_str());
        if (ImGui::CollapsingHeader("Timings"))
        {
            ImGui::Text(("RENDERING_ALL: " + ve::to_string(devicetimings[DeviceTimer::RENDERING_ALL], 4) + " ms").c_str());
        }
        if (ImGui::CollapsingHeader("Plots"))
        {
            if (ImPlot::BeginPlot("Timings"))
            {
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, 100.0);
                ImPlot::SetupAxes("Frame", "Time [ms]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit);
                ImPlot::PlotLine("FRAMETIME", frametime_values.data(), frametime_values.size());
                ImPlot::PlotLine("RENDERING_ALL", devicetiming_values[DeviceTimer::RENDERING_ALL].data(), devicetiming_values[DeviceTimer::RENDERING_ALL].size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
        ImGui::EndFrame();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
    }
} // namespace ve
