#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
public:
    Camera(float fov, float aspect_ratio);
    void updateVP(float time_diff);
    glm::mat4 getV();
    glm::mat4 getVP();
    void translate(glm::vec3 v);
    void onMouseMove(float xRel, float yRel);
    void moveFront(float amount);
    void moveRight(float amount);
    void moveUp(float amount);
    void rotate(float amount);
    void updateScreenSize(float aspect_ratio);
    const glm::vec3& getPosition() const;
    float getNear() const;
    float getFar() const;
    glm::vec3 getFront() const;
    glm::vec3 getUp() const;
    glm::vec3 getRight() const;

    struct Data
    {
        alignas(16) glm::vec3 pos;
        alignas(16) glm::vec3 u;
        alignas(16) glm::vec3 v;
        alignas(16) glm::vec3 w;
        alignas(8) glm::vec2 sensor_size;
        alignas(4) float focal_length;

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
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 vp;
    glm::vec3 u, v, w;
    float near, far;
    float yaw, pitch, roll;
    float fov;
    const float mouse_sensitivity = 0.25f;
};
