#pragma once
#include "ShaderCompile.h"
#include "Vertex.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#undef UINT32_MAX
#undef UINT64_MAX

#include <stdexcept>
#include <vector>

constexpr uint32_t UINT32_MAX{ 0xffffffff };
constexpr uint64_t UINT64_MAX {0xffffffffffffffff};

#ifndef NDEBUG
const std::vector<const char*> VALIDATION_LAYERS{
	"VK_LAYER_KHRONOS_validation"
};
#else
const std::vector<const char*> ENABLED_VALIDATION_LAYERS{};
#endif

const std::vector<const char*> ENABLED_DEVICE_EXTENSIONS{
	"VK_KHR_swapchain"
};

constexpr int WIDTH = 1600;
constexpr int HEIGHT = 900;

constexpr uint32_t MIN_VULKAN_API_VERSION = VK_API_VERSION_1_0;
constexpr VkAllocationCallbacks* P_DEFAULT_ALLOC = nullptr;

struct QueueIndices {
	uint32_t graphicsIndex{ UINT32_MAX };
	uint32_t computeIndex{ UINT32_MAX };
	uint32_t transferIndex{ UINT32_MAX };
};

class App {
public:
	void run();

private:
	GLFWwindow* m_pWindow;
	VkInstance m_instance;
	VkSurfaceKHR m_surface;

	VkPhysicalDevice m_physDevice;
	QueueIndices m_queueIndices;
	VkQueue m_graphicsQueue;
	VkDevice m_device;

	VkCommandPool m_cmdPool;
	VkCommandBuffer m_cmdBuffer;
	
	VkShaderModule m_projVertModule;
	VkShaderModule m_projFragModule;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapchainImages;

	VkSemaphore m_imageAvailableSem;
	VkSemaphore m_renderCompleteSem;
	
	void setQueueIndices();
	void chooseMostSuitablePhysicalDevice();
	void createProjectionPipeline();
	void init();
	void loop();
	void cleanup();
};