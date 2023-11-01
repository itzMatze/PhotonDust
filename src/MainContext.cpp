#include "MainContext.hpp"

MainContext::MainContext() : vcc(vmc), wc(vmc, vcc, app_state) 
{
    if (sc.is_cache_loaded())
    {
        app_state.cam = Camera(60.0f, app_state.aspect_ratio, sc.data.sensor_width, sc.data.focal_length, sc.data.exposure, sc.data.pos, sc.data.euler);
        app_state.cam.update();
        app_state.headless = sc.data.headless && sc.data.sample_count > 0;
    }
    if (app_state.headless)
    {
        // default aspect_ratio is 16:9
        app_state.render_extent = vk::Extent2D(5120, 2880);
        vmc.construct();
    }
    else
    {
        app_state.render_extent = vk::Extent2D(1920, 1080);
        vmc.construct(app_state.window_extent.width, app_state.window_extent.height);
    }
    vcc.construct();
    wc.construct(app_state);
    app_state.devicetimings.resize(ve::DeviceTimer::TIMER_COUNT, 0.0f);
}

MainContext::~MainContext()
{
    wc.destruct();
    vcc.destruct();
    vmc.destruct();
    spdlog::info("Destroyed MainContext");
}

void MainContext::run()
{
    if (app_state.headless)
    {
        run_headless();
    }
    else
    {
        run_ui();
    }
}

void MainContext::dispatch_pressed_keys()
{
    if (eh.is_key_pressed(Key::W)) app_state.cam.move_front(move_amount);
    if (eh.is_key_pressed(Key::A)) app_state.cam.move_right(-move_amount);
    if (eh.is_key_pressed(Key::S)) app_state.cam.move_front(-move_amount);
    if (eh.is_key_pressed(Key::D)) app_state.cam.move_right(move_amount);
    if (eh.is_key_pressed(Key::Q)) app_state.cam.move_up(-move_amount);
    if (eh.is_key_pressed(Key::E)) app_state.cam.move_up(move_amount);
    float panning_speed = eh.is_key_pressed(Key::Shift) ? 200.0f : 600.0f;
    if (eh.is_key_pressed(Key::Left)) app_state.cam.on_mouse_move(-panning_speed * app_state.time_diff, 0.0f);
    if (eh.is_key_pressed(Key::Right)) app_state.cam.on_mouse_move(panning_speed * app_state.time_diff, 0.0f);
    if (eh.is_key_pressed(Key::Up)) app_state.cam.on_mouse_move(0.0f, -panning_speed * app_state.time_diff);
    if (eh.is_key_pressed(Key::Down)) app_state.cam.on_mouse_move(0.0f, panning_speed * app_state.time_diff);

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
    if (eh.is_key_released(Key::C))
    {
        eh.set_released_key(Key::C, false);
        sc.data.scene_name = app_state.scene_names[app_state.current_scene];
        sc.data.pos = app_state.cam.get_position();
        sc.data.euler = app_state.cam.get_euler();
        sc.data.sensor_width = app_state.cam.data.sensor_size.x;
        sc.data.focal_length = app_state.cam.data.focal_length;
        sc.data.exposure = app_state.cam.data.exposure;
        sc.save_cache();
    }
    if (eh.is_key_released(Key::One))
    {
        eh.set_released_key(Key::One, false);
        app_state.current_scene = 0;
    }
    if (eh.is_key_released(Key::Two))
    {
        eh.set_released_key(Key::Two, false);
        app_state.current_scene = 1;
    }
    if (eh.is_key_released(Key::Three))
    {
        eh.set_released_key(Key::Three, false);
        app_state.current_scene = 2;
    }
    if (eh.is_key_released(Key::Four))
    {
        eh.set_released_key(Key::Four, false);
        app_state.current_scene = 3;
    }
    if (eh.is_key_released(Key::Five))
    {
        eh.set_released_key(Key::Five, false);
        app_state.current_scene = 4;
    }
    if (eh.is_key_released(Key::Six))
    {
        eh.set_released_key(Key::Six, false);
        app_state.current_scene = 5;
    }
    if (eh.is_key_released(Key::Seven))
    {
        eh.set_released_key(Key::Seven, false);
        app_state.current_scene = 6;
    }
    if (eh.is_key_released(Key::Eight))
    {
        eh.set_released_key(Key::Eight, false);
        app_state.current_scene = 7;
    }
    if (eh.is_key_released(Key::Nine))
    {
        eh.set_released_key(Key::Nine, false);
        app_state.current_scene = 8;
    }
    if (eh.is_key_released(Key::Zero))
    {
        eh.set_released_key(Key::Zero, false);
        app_state.current_scene = 9;
    }
    if (eh.is_key_released(Key::Return))
    {
        eh.set_released_key(Key::Return, false);
        app_state.load_scene = true;
    }
    if (eh.is_key_pressed(Key::MouseLeft))
    {
        if (!SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);
        app_state.cam.on_mouse_move(eh.mouse_motion.x * 1.5f, eh.mouse_motion.y * 1.5f);
        eh.mouse_motion = glm::vec2(0.0f);
    }
    if (eh.is_key_released(Key::MouseLeft))
    {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_WarpMouseInWindow(vmc.window->get(), app_state.window_extent.width / 2.0f, app_state.window_extent.height / 2.0f);
        eh.set_released_key(Key::MouseLeft, false);
    }
}

void MainContext::run_headless()
{
    wc.load_scene(sc.data.scene_name);
    float progress = 0.0f;
    uint32_t progress_percent = 0;
    ve::HostTimer timer;
    for (uint32_t i = 0; i < sc.data.sample_count; ++i)
    {
        wc.headless_next_sample(app_state);
        progress = float(i) / float(sc.data.sample_count);
        if (progress * 100.0f >= progress_percent) std::cout << progress_percent++ << '\r' << std::flush;
    }
    std::cout << std::endl;
    spdlog::info("Rendering took: {} ms", timer.elapsed<std::milli>());
    app_state.save_screenshot = true;
    wc.headless_next_sample(app_state);
}

void MainContext::run_ui()
{
    std::string default_scene("default.json");
    if (sc.is_cache_loaded())
    {
        default_scene = sc.data.scene_name;
    }
    std::vector<std::string> scene_names;
    app_state.current_scene = 0;
    for (const auto& entry : std::filesystem::directory_iterator("../assets/scenes/"))
    {
        if (entry.path().filename() == default_scene) app_state.current_scene = scene_names.size();
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
        app_state.cam.update();
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
