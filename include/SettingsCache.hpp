#pragma once

#include <string>
#include <iostream>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

class SettingsCache
{
public:
    struct Data
    {
        bool headless;
        uint32_t sample_count;
        std::string scene_name;
        // camera data
        glm::vec3 pos;
        glm::vec3 euler;
        float sensor_width;
        float focal_length;
        float exposure;

        friend std::istream& operator>>(std::istream& is, Data& data);
        friend std::ostream& operator<<(std::ostream& os, const Data& data);
    } data;

    SettingsCache();
    void load_cache();
    bool save_cache();
    bool is_cache_loaded() { return cache_loaded; }

private:
    bool cache_loaded = false;
};
