#include "SettingsCache.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>

constexpr std::string_view cache_filename("../assets/settings_cache.txt");

std::istream& operator>>(std::istream& is, SettingsCache::Data& data)
{
    std::string buffer;
    auto get = [&buffer, &is]() -> std::string& {
        std::getline(is, buffer);
        return buffer;
    };
    data.scene_name = get();
    data.pos.x = std::stof(get());
    data.pos.y = std::stof(get());
    data.pos.z = std::stof(get());
    data.euler.x = std::stof(get());
    data.euler.y = std::stof(get());
    data.euler.z = std::stof(get());
    data.sensor_width = std::stof(get());
    data.focal_length = std::stof(get());
    data.exposure = std::stof(get());
    return is;
}

std::ostream& operator<<(std::ostream& os, const SettingsCache::Data& data)
{
    auto print_float = [&os](float f) { os << std::fixed << std::setprecision(10) << f << '\n'; };
    os << data.scene_name << '\n';
    print_float(data.pos.x);
    print_float(data.pos.y);
    print_float(data.pos.z);
    print_float(data.euler.x);
    print_float(data.euler.y);
    print_float(data.euler.z);
    print_float(data.sensor_width);
    print_float(data.focal_length);
    print_float(data.exposure);
    return os;
}

bool SettingsCache::load_cache()
{
    if (!std::filesystem::exists(cache_filename)) return false;
    std::ifstream file(cache_filename.data());
    if (!file.is_open()) return false;
    file >> data;
    file.close();
    return true;
}

bool SettingsCache::save_cache()
{
    std::ofstream file(cache_filename.data(), std::ios::trunc);
    if (!file.is_open()) return false;
    file << data;
    file.close();
    return true;
}
