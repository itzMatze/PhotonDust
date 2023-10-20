#pragma once

#include <string>
#include <cstdint>

namespace ve
{
    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(int32_t material_idx, uint32_t idx_offset, uint32_t idx_count, const std::string& name);

        int32_t material_idx;
        uint32_t index_offset;
        uint32_t index_count;
        std::string name;
    };
} // namespace ve
