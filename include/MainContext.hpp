#pragma once

#include <cstdint>
#include <filesystem>

#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "WorkContext.hpp"
#include "SettingsCache.hpp"
#include "EventHandler.hpp"
#include "UI.hpp"
#include "vk/Timer.hpp"

class MainContext
{
public:
    MainContext();
    ~MainContext();
    void run();

private:
    SettingsCache sc;
    ve::AppState app_state;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::WorkContext wc;
    EventHandler eh;
    float move_amount;
    float move_speed = 20.0f;

    void dispatch_pressed_keys();
    void run_headless();
    void run_ui();
};
