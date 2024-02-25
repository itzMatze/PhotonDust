#pragma once

#include <vector>
#include <glm/mat4x4.hpp>

#include "json.hpp"

#include "vk/Mesh.hpp"
#include "Storage.hpp"

namespace ve
{
    struct Material {
        glm::vec4 base_color = glm::vec4(1.0f);
        glm::vec4 emission = glm::vec4(0.0f);
        float emission_strength = 1.0f;
        float metallic = 0.0f;
        float roughness = 1.0f;
        int32_t base_texture = -1;
        // use borosilicate crown glass (BK7) by default
        glm::vec3 B = glm::vec3(1.03961212, 0.231792344, 1.01046945);
        float transmission = 0.0f;
        alignas(16) glm::vec3 C = glm::vec3(0.00600069867, 0.0200179144, 103.560653);
        //int32_t emissive_texture = -1;
        //int32_t metallic_roughness_texture = -1;
        //int32_t normal_texture = -1;
        //int32_t occlusion_texture = -1;
    };

    struct Light {
        glm::vec3 dir;
        float intensity = 0.0f;
        glm::vec3 pos;
        float innerConeAngle;
        glm::vec3 color;
        float outerConeAngle;
    };

    struct Model
    {
        void apply_transformation(const glm::mat4& transformation);

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Material> materials;
        std::vector<Light> lights;
        std::vector<Mesh> meshes;
        std::vector<uint32_t> texture_image_indices;
    };

    namespace ModelLoader
    {
        void reset();
        Model load(const VulkanMainContext& vmc, Storage& storage, const nlohmann::json& model);
        Model load_custom(const VulkanMainContext& vmc, Storage& storage, const nlohmann::json& model);
    };
} // namespace ve
