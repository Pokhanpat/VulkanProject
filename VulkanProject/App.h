#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <optional>
#include <string>

#ifndef NDEBUG
const bool ENABLE_VALIDATION = true;
#else
const bool ENABLE_VALIDATION = false;
#endif

const uint32_t REQ_VULKAN_VERSION = VK_API_VERSION_1_0;
const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 900;

inline const VkPhysicalDeviceFeatures REQUIRED_DEVICE_FEATURES{};
const std::vector<const char*> ENABLED_VALIDATION_LAYERS{
	"VK_LAYER_KHRONOS_validation"
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transferFamily;

	bool areIndicesSet();
};

class App {
	public:
		void run();
	private:
		GLFWwindow* m_pWindow;
		VkInstance m_instance;
		VkPhysicalDevice m_physDevice;
		VkDevice m_logDevice;
		QueueFamilyIndices m_queueFamilyIndices;
		VkSurfaceKHR m_surface;

		void init();
		void loop();
		void cleanup();
		void getQueueFamilyIndices(QueueFamilyIndices* pIndices);
		void getMostSuitablePhysicalDevice(VkPhysicalDevice* pDevice);
};