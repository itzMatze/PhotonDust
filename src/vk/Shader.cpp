#include "vk/Shader.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>

#include "ve_log.hpp"

namespace ve
{
    Shader::Shader(const vk::Device& device, const std::string filename, vk::ShaderStageFlagBits shader_stage_flag) : name(filename), device(device)
    {
        std::filesystem::path shader_dir("../shader/");
        std::filesystem::path shader_bin_dir(shader_dir / "bin/");
        if (!std::filesystem::exists(shader_bin_dir)) std::filesystem::create_directory(shader_bin_dir);
        std::filesystem::path shader_file(shader_dir / filename);
        std::filesystem::path shader_bin_file(shader_bin_dir / (filename + ".spv"));
        spdlog::debug("Loading shader \"{}\"", filename);
        VE_ASSERT(std::filesystem::exists(shader_file), "Failed to find shader file \"{}\"", filename);
        system(std::format("glslc --target-env=vulkan1.2 -O -o {0} {1}", shader_bin_file.string(), shader_file.string()).c_str());
        std::string source = read_shader_file(shader_bin_file);
        vk::ShaderModuleCreateInfo smci{};
        smci.sType = vk::StructureType::eShaderModuleCreateInfo;
        smci.codeSize = source.size();
        smci.pCode = reinterpret_cast<const uint32_t*>(source.c_str());
        shader_module = device.createShaderModule(smci);

        pssci.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
        pssci.stage = shader_stage_flag;
        pssci.module = shader_module;
        pssci.pName = "main";
    }

    void Shader::destruct()
    {
        device.destroyShaderModule(shader_module);
    }

    const vk::ShaderModule Shader::get() const
    {
        return shader_module;
    }

    const vk::PipelineShaderStageCreateInfo& Shader::get_stage_create_info() const
    {
        return pssci;
    }

    std::string Shader::read_shader_file(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        VE_ASSERT(file.is_open(), "Failed to open shader file \"{}\"", filename);
        std::ostringstream file_stream;
        file_stream << file.rdbuf();
        return file_stream.str();
    }
} // namespace ve
