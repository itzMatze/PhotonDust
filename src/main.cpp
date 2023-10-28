#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <thread>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "EventHandler.hpp"
#include "ve_log.hpp"
#include "vk/Timer.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "WorkContext.hpp"

constexpr uint32_t render_width = 3840;
constexpr uint32_t render_height = 2160;
constexpr float aspect_ratio = float(render_width) / float(render_height);

class MainContext
{
public:
    MainContext() : app_state{.render_extent = vk::Extent2D(render_width, render_height), .window_extent = vk::Extent2D(aspect_ratio * 1000, 1000), .aspect_ratio = aspect_ratio, .cam = Camera(60.0f, aspect_ratio)}, vmc(app_state.window_extent.width, app_state.window_extent.height), vcc(vmc), wc(vmc, vcc, app_state)
    {
        app_state.devicetimings.resize(ve::DeviceTimer::TIMER_COUNT, 0.0f);
    }

    ~MainContext()
    {
        wc.self_destruct();
        vcc.self_destruct();
        vmc.self_destruct();
        spdlog::info("Destroyed MainContext");
    }

    void run()
    {
        std::vector<std::string> scene_names;
        app_state.current_scene = 0;
        for (const auto& entry : std::filesystem::directory_iterator("../assets/scenes/"))
        {
            if (entry.path().filename() == "default.json") app_state.current_scene = scene_names.size();
            scene_names.push_back(entry.path().filename());
        }
        for (const auto& name : scene_names) app_state.scene_names.push_back(&name.front());
        wc.load_scene(app_state.scene_names[app_state.current_scene]);
        ve::HostTimer timer;
        bool quit = false;
        SDL_Event e;

        while (!quit)
        {
            move_amount = app_state.time_diff * move_speed;
            dispatch_pressed_keys();
            app_state.cam.update(app_state.time_diff);
            try
            {
                wc.draw_frame(app_state);
            }
            catch (const vk::OutOfDateKHRError e)
            {
                app_state.window_extent = wc.recreate_swapchain(app_state.vsync);
            }
            while (SDL_PollEvent(&e))
            {
                quit = e.window.event == SDL_WINDOWEVENT_CLOSE;
                eh.dispatch_event(e);
            }
            app_state.time_diff = timer.restart();
            app_state.time += app_state.time_diff;
            if (app_state.load_scene)
            {
                app_state.load_scene = false;
                wc.load_scene(app_state.scene_names[app_state.current_scene]);
                app_state.sample_count = 0;
                timer.restart();
            }
        }
    }

private:
    ve::AppState app_state;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::WorkContext wc;
    EventHandler eh;
    float move_amount;
    float move_speed = 20.0f;

    void dispatch_pressed_keys()
    {
        if (eh.is_key_pressed(Key::W)) app_state.cam.moveFront(move_amount);
        if (eh.is_key_pressed(Key::A)) app_state.cam.moveRight(-move_amount);
        if (eh.is_key_pressed(Key::S)) app_state.cam.moveFront(-move_amount);
        if (eh.is_key_pressed(Key::D)) app_state.cam.moveRight(move_amount);
        if (eh.is_key_pressed(Key::Q)) app_state.cam.moveUp(-move_amount);
        if (eh.is_key_pressed(Key::E)) app_state.cam.moveUp(move_amount);
        float panning_speed = eh.is_key_pressed(Key::Shift) ? 200.0f : 600.0f;
        if (eh.is_key_pressed(Key::Left)) app_state.cam.onMouseMove(-panning_speed * app_state.time_diff, 0.0f);
        if (eh.is_key_pressed(Key::Right)) app_state.cam.onMouseMove(panning_speed * app_state.time_diff, 0.0f);
        if (eh.is_key_pressed(Key::Up)) app_state.cam.onMouseMove(0.0f, -panning_speed * app_state.time_diff);
        if (eh.is_key_pressed(Key::Down)) app_state.cam.onMouseMove(0.0f, panning_speed * app_state.time_diff);

        // reset state of keys that are used to execute a one time action
        if (eh.is_key_released(Key::Plus))
        {
            move_speed *= 2.0f;
            eh.set_released_key(Key::Plus, false);
        }
        if (eh.is_key_released(Key::Minus))
        {
            move_speed /= 2.0f;
            eh.set_released_key(Key::Minus, false);
        }
        if (eh.is_key_released(Key::G))
        {
            app_state.show_ui = !app_state.show_ui;
            eh.set_released_key(Key::G, false);
        }
        if (eh.is_key_released(Key::H))
        {
            app_state.attenuation_view = !app_state.attenuation_view;
            eh.set_released_key(Key::H, false);
        }
        if (eh.is_key_released(Key::J))
        {
            app_state.emission_view = !app_state.emission_view;
            eh.set_released_key(Key::J, false);
        }
        if (eh.is_key_released(Key::K))
        {
            app_state.normal_view = !app_state.normal_view;
            eh.set_released_key(Key::K, false);
        }
        if (eh.is_key_released(Key::L))
        {
            app_state.tex_view = !app_state.tex_view;
            eh.set_released_key(Key::L, false);
        }
        if (eh.is_key_released(Key::F1))
        {
            app_state.save_screenshot = true;
            eh.set_released_key(Key::F1, false);
        }
        if (eh.is_key_released(Key::R))
        {
            eh.set_released_key(Key::R, false);
            wc.reload_shaders();
            app_state.sample_count = 0;
        }
        if (eh.is_key_released(Key::V))
        {
            eh.set_released_key(Key::V, false);
            app_state.vsync = !app_state.vsync;
            wc.recreate_swapchain(app_state.vsync);
        }
        if (eh.is_key_pressed(Key::MouseLeft))
        {
            if (!SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);
            app_state.cam.onMouseMove(eh.mouse_motion.x * 1.5f, eh.mouse_motion.y * 1.5f);
            eh.mouse_motion = glm::vec2(0.0f);
        }
        if (eh.is_key_released(Key::MouseLeft))
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            SDL_WarpMouseInWindow(vmc.window->get(), app_state.window_extent.width / 2.0f, app_state.window_extent.height / 2.0f);
            eh.set_released_key(Key::MouseLeft, false);
        }
    }
};

int main(int argc, char** argv)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("ve.log", true));
    auto combined_logger = std::make_shared<spdlog::logger>("default_logger", sinks.begin(), sinks.end());
    spdlog::set_default_logger(combined_logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%L] %v");
    spdlog::info("Starting");
    auto t1 = std::chrono::high_resolution_clock::now();
    MainContext mc;
    auto t2 = std::chrono::high_resolution_clock::now();
    spdlog::info("Setup took: {} ms", (std::chrono::duration<double, std::milli>(t2 - t1).count()));
    mc.run();
    return 0;
}
