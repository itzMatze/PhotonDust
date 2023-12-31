﻿#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
public:
    Camera(float fov, float aspect_ratio);
    Camera(float fov, float aspect_ratio, float sensor_width, float focal_length, float exposure, glm::vec3 pos, glm::vec3 euler);
    void update_data();
    void update();
    void translate(glm::vec3 v);
    void on_mouse_move(float xRel, float yRel);
    void move_front(float amount);
    void move_right(float amount);
    void move_up(float amount);
    void rotate(float amount);
    void update_screen_size(float aspect_ratio);
    const glm::vec3& get_position() const;
    float get_near() const;
    float get_far() const;
    glm::vec3 get_euler() const;

    struct Data
    {
        alignas(16) glm::vec3 pos;
        alignas(16) glm::vec3 u;
        alignas(16) glm::vec3 v;
        alignas(16) glm::vec3 w;
        alignas(8) glm::vec2 sensor_size;
        alignas(4) float focal_length;
        alignas(4) float exposure;

        bool operator==(const Data& cam_data)
        {
            return (this->pos == cam_data.pos && this->u == cam_data.u && this->v == cam_data.v && this->w == cam_data.w && this->sensor_size == cam_data.sensor_size && this->focal_length == cam_data.focal_length);
        }

        bool operator!=(const Data& cam_data)
        {
            return !(*this == cam_data);
        }
    } data;

private:
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 u, v, w;
    float near, far;
    float yaw, pitch, roll;
    float fov;
    float mouse_sensitivity = 0.25f;
};
