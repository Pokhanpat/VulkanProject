#pragma once
#include "Vertex.h"
#include "ShaderCompile.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#undef UINT32_MAX

#ifndef NDEBUG
const std::vector<const char*> ENABLED_VALIDATION_LAYERS{
	"VK_LAYER_KHRONOS_validation"
};
#else
const std::vector<const char*> ENABLED_VALIDATION_LAYERS{};
#endif

const std::vector<const char*> REQ_INSTANCE_EXTENSIONS{
	"VK_KHR_surface"
};
const std::vector<const char*> REQ_DEVICE_EXTENSIONS{
	"VK_KHR_swapchain"
};

constexpr uint32_t UINT32_MAX = 0xffffffff;

const int WIDTH = 1600;
const int HEIGHT = 900;

const uint32_t REQUIRED_VULKAN_VER = VK_API_VERSION_1_3;
inline const VkAllocationCallbacks* P_DEFAULT_ALLOCATOR = nullptr;
const float DEFAULT_QUEUE_PRIORITY = 1.0f;

struct QueueFamilyIndeces {
	uint32_t graphicsIndex{UINT32_MAX}; //hopefully no ones computer has like 4 billion fucking queue families
	uint32_t computeIndex{ UINT32_MAX }; //bc thatd cause an error
	uint32_t transferIndex{ UINT32_MAX };
};

class App {
public:
	void run();

private:
	GLFWwindow* m_pWin;
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physDevice;
	QueueFamilyIndeces m_indices;
	VkDevice m_device;
	VkSwapchainKHR m_swapchain;
	VkRenderPass m_graphicsPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkShaderModule m_fragShaderModule;
	VkShaderModule m_vertShaderModule;

	void createDevice();
	void createPipeline();
	void init();

	void loop();
	void cleanup();
};