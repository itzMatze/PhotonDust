#include "Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

inline constexpr glm::vec3 back(0.0f, 0.0f, 1.0f);
inline constexpr glm::vec3 right(1.0f, 0.0f, 0.0f);
inline constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);

Camera::Camera(float fov, float aspect_ratio) : fov(fov), yaw(0.0f), pitch(0.0f), roll(0.0f), near(0.1f), far(10000.0f), position(0.0f, 0.0f, 10.0f)
{
    data.sensor_size = glm::vec2(0.036, 0.036 / aspect_ratio);
    data.focal_length = 0.03;
    data.exposure = 1.0f;
    orientation = glm::quatLookAt(-back, up);
}

Camera::Camera(float fov, float aspect_ratio, float sensor_width, float focal_length, float exposure, glm::vec3 pos, glm::vec3 euler) : fov(fov), yaw(0.0f), pitch(0.0f), roll(0.0f), near(0.1f), far(10000.0f), position(pos)
{
    data.sensor_size = glm::vec2(sensor_width, sensor_width / aspect_ratio);
    data.focal_length = focal_length;
    data.exposure = exposure;
    pitch = euler.x;
    yaw = euler.y;
    roll = euler.z;
    glm::quat q_pitch = glm::angleAxis(glm::radians(pitch), right);
    glm::quat q_yaw = glm::angleAxis(glm::radians(yaw), up);
    glm::quat q_roll = glm::angleAxis(glm::radians(roll), back);
    orientation = glm::normalize(q_yaw * q_pitch * q_roll);
}

void Camera::update_data()
{
    data.pos = position;
    data.u = u;
    data.v = v;
    data.w = w;
}

void Camera::update()
{
    constexpr bool use_free_cam = false;
    // rotate initial coordinate system to camera orientation
    glm::quat q_back = glm::normalize(orientation * glm::quat(0.0f, back) * glm::conjugate(orientation));
    glm::quat q_right = glm::normalize(orientation * glm::quat(0.0f, right) * glm::conjugate(orientation));
    glm::quat q_up = glm::normalize(orientation * glm::quat(0.0f, up) * glm::conjugate(orientation));
    w = glm::normalize(glm::vec3(q_back.x, q_back.y, q_back.z));
    u = glm::normalize(glm::vec3(q_right.x, q_right.y, q_right.z));
    v = glm::normalize(glm::vec3(q_up.x, q_up.y, q_up.z));

    if (use_free_cam)
    {
        // calculate incremental change in angles with respect to camera coordinate system
        glm::quat q_pitch = glm::angleAxis(glm::radians(pitch), u);
        glm::quat q_yaw = glm::angleAxis(glm::radians(yaw), v);
        glm::quat q_roll = glm::angleAxis(glm::radians(roll), w);

        // apply incremental change to camera orientation
        orientation = glm::normalize(q_yaw * q_pitch * q_roll * orientation);
        // reset angles as the changes were applied to camera orientation
        pitch = 0;
        yaw = 0;
        roll = 0;
    }
    else
    {
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
        glm::quat q_pitch = glm::angleAxis(glm::radians(pitch), right);
        glm::quat q_yaw = glm::angleAxis(glm::radians(yaw), up);
        glm::quat q_roll = glm::angleAxis(glm::radians(roll), back);
        orientation = glm::normalize(q_yaw * q_pitch * q_roll);
    }

    // calculate view matrix
    glm::quat revers_orient = glm::conjugate(orientation);
    glm::mat4 rotation = glm::mat4_cast(revers_orient);
}

void Camera::translate(glm::vec3 amount)
{
    position += amount;
}

void Camera::on_mouse_move(float xRel, float yRel)
{
    yaw -= xRel * mouse_sensitivity;
    pitch -= yRel * mouse_sensitivity;
}

void Camera::move_front(float amount)
{
    translate(-amount * w);
}

void Camera::move_right(float amount)
{
    translate(amount * u);
}

void Camera::move_up(float amount)
{
    translate(up * amount);
}

void Camera::rotate(float amount) 
{
    roll -= amount;
}

const glm::vec3& Camera::get_position() const
{
    return position;
}

float Camera::get_near() const
{
    return near;
}

float Camera::get_far() const
{
    return far;
}

glm::vec3 Camera::get_euler() const
{
    return glm::vec3(pitch, yaw, roll);
}
