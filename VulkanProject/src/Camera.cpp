#include "Camera.h"

glm::mat4 Camera::getViewMatrix() {
	glm::mat4 transpose = glm::translate(glm::mat4(1.0), m_pos);
	glm::mat4 pitchRotate = glm::rotate(glm::mat4(1.0), m_pitch, glm::vec3(1,0,0));
	glm::mat4 yawRotate = glm::rotate(glm::mat4(1.0), m_yaw, glm::vec3(0, 1, 0));

	return glm::inverse(transpose * pitchRotate * yawRotate);
}

CameraProjectionData Camera::fetchGPUData(float viewPortWidth, float viewPortHeight) {
	CameraProjectionData data{
		.viewMatrix = getViewMatrix(),
		.projectionMatrix = glm::perspectiveFov(m_FOV, viewPortWidth, viewPortHeight, m_near, m_far)
	};

	return data;
}