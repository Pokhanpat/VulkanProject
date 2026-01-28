#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

constexpr float PI = 3.1415926f;

struct CameraProjectionData {
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

class Camera {
public:
	glm::vec3 m_pos;
	float m_pitch;
	float m_yaw;
	float m_FOV = PI / 2;
	float m_near = 0.01f;
	float m_far = 100000.0f;

	glm::mat4 getViewMatrix();
	CameraProjectionData fetchGPUData(float viewPortWidth, float viewPortHeight);
};