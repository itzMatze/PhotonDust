#include "vk/Scene.hpp"

#include <fstream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>

#include "json.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage), path_tracer(vmc, vcc, storage)
    {}

    void Scene::construct()
    {
        if (!loaded) VE_THROW("Cannot construct scene before loading one!");
        vk::CommandBuffer& cb = vcc.get_one_time_compute_buffer();
        path_tracer.create_tlas(cb);
        vcc.submit_compute(cb, true);
    }

    void Scene::destruct()
    {
        path_tracer.destruct();
        storage.destroy_buffer(emissive_mesh_indices_buffer);
        storage.destroy_buffer(model_mrd_indices_buffer);
        storage.destroy_buffer(mesh_render_data_buffer);
        storage.destroy_buffer(light_buffer);
        storage.destroy_buffer(material_buffer);
        storage.destroy_buffer(index_buffer);
        storage.destroy_buffer(vertex_buffer);
        for (uint32_t i : texture_image_indices) storage.destroy_image(i);
        texture_image_indices.clear();
        loaded = false;
    }

    void Scene::load(const std::string& path)
    {
        ModelLoader::reset();
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Material> materials;
        std::vector<MeshRenderData> mesh_render_data;
        std::vector<uint32_t> emissive_mesh_indices;
        std::vector<Light> lights;
        std::vector<ModelInfo> model_infos;

        auto add_model = [&](Model& model, const std::string& name, const glm::mat4& transformation) -> void
        {
            model_infos.push_back({});
            model_infos.back().index_buffer_idx = indices.size();
            vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
            indices.insert(indices.end(), model.indices.begin(), model.indices.end());
            model_infos.back().num_indices = indices.size() - model_infos.back().index_buffer_idx;
            materials.insert(materials.end(), model.materials.begin(), model.materials.end());
            lights.insert(lights.end(), model.lights.begin(), model.lights.end());
            texture_image_indices.insert(texture_image_indices.end(), model.texture_image_indices.begin(), model.texture_image_indices.end());
            model_infos.back().mesh_render_data_idx = mesh_render_data.size();
            for (Mesh& mesh : model.meshes)
            {
                mesh_render_data.push_back(MeshRenderData{.mat_idx = mesh.material_idx, .indices_idx = mesh.index_offset, .idx_count = mesh.index_count});
                model_infos.back().mesh_index_offsets.push_back(mesh.index_offset);
                model_infos.back().mesh_index_count.push_back(mesh.index_count);
                if (glm::length(materials[mesh.material_idx].emission) > 0.0 && materials[mesh.material_idx].emission_strength > 0.0)
                {
                    emissive_mesh_indices.push_back(mesh_render_data.size() - 1);
                }
            }
        };

        // load scene from custom json file
        using json = nlohmann::json;
        std::ifstream file(path);
        json data = json::parse(file);
        if (data.contains("model_files"))
        {
            // load referenced model files
            for (const auto& d : data.at("model_files"))
            {
                const std::string name = d.value("name", "");
                Model model = ModelLoader::load(vmc, storage, d);

                // apply transformations to model
                glm::mat4 transformation(1.0f);
                if (d.contains("scale"))
                {
                    transformation[0][0] = d.at("scale")[0];
                    transformation[1][1] = d.at("scale")[1];
                    transformation[2][2] = d.at("scale")[2];
                }
                if (d.contains("rotation"))
                {
                    transformation = glm::rotate(transformation, glm::radians(float(d.at("rotation")[0])), glm::vec3(d.at("rotation")[1], d.at("rotation")[2], d.at("rotation")[3]));
                }
                if (d.contains("translation"))
                {
                    transformation[3][0] = d.at("translation")[0];
                    transformation[3][1] = d.at("translation")[1];
                    transformation[3][2] = d.at("translation")[2];
                }
                model.apply_transformation(transformation);
                add_model(model, name, transformation);
            }
        }
        // load custom models (vertices and indices directly contained in json file)
        if (data.contains("custom_models"))
        {
            for (const auto& d : data["custom_models"])
            {
                std::string name = d.value("name", "");
                Model model = ModelLoader::load_custom(vmc, storage, d);
                add_model(model, name, glm::mat4(1.0f));
            }
        }
        vertex_buffer = storage.add_named_buffer(std::string("vertices"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);
        index_buffer = storage.add_named_buffer(std::string("indices"), indices, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);
        vk::CommandBuffer& cb = vcc.get_one_time_compute_buffer();
        for (uint32_t i = 0; i < model_infos.size(); ++i)
        {
            ModelInfo& mi = model_infos[i];
            mi.blas_idx = path_tracer.add_blas(cb, vertex_buffer, index_buffer, mi.mesh_index_offsets, mi.mesh_index_count, sizeof(Vertex));
            mi.instance_idx = path_tracer.add_instance(mi.blas_idx, glm::mat4(1.0f), i);
        }
        vcc.submit_compute(cb, true);
        if (materials.empty()) materials.push_back(Material());
        material_buffer = storage.add_named_buffer(std::string("materials"), materials, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        materials.clear();
        if (lights.empty()) lights.push_back(Light());
        light_buffer = storage.add_named_buffer(std::string("lights"), lights, vk::BufferUsageFlagBits::eStorageBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        // delete vertices and indices on host
        indices.clear();
        vertices.clear();
        mesh_render_data_buffer = storage.add_named_buffer("mesh_render_data", mesh_render_data, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        std::vector<uint32_t> model_mrd_indices;
        for (const auto& model : model_infos) model_mrd_indices.push_back(model.mesh_render_data_idx);
        model_mrd_indices_buffer = storage.add_named_buffer("model_mrd_indices", model_mrd_indices, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        emissive_mesh_indices_buffer = storage.add_named_buffer("emissive_mesh_indices", emissive_mesh_indices, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);

        std::vector<unsigned char> texture_data(4, 0);
        texture_image_indices.push_back(storage.add_named_image("texture_" + std::to_string(texture_image_indices.size()), texture_data.data(), 1, 1, true, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eSampled));

        loaded = true;
    }

    uint32_t Scene::get_texture_image_count() const
    {
        return texture_image_indices.size();
    }
} // namespace ve
