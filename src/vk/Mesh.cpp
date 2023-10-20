#include "vk/Mesh.hpp"

namespace ve
{
    Mesh::Mesh(int32_t material_idx, uint32_t idx_offset, uint32_t idx_count, const std::string& name) : material_idx(material_idx), index_offset(idx_offset), index_count(idx_count), name(name)
    {}
} // namespace ve
