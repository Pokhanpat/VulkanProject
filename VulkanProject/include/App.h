#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <string>
#include <limits>
#include <algorithm>

#undef max
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
const std::vector<const char*> ENABLED_DEVICE_EXTENSIONS{
	"VK_KHR_swapchain"
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transferFamily;

	bool areIndicesSet();
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
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
		VkQueue m_graphicsQueue;
		VkSwapchainKHR m_swapChain;
		std::vector<VkImage> m_swapChainImages;
		VkFormat m_swapChainImageFormat;
		VkExtent2D m_swapChainExtent;
		std::vector<VkImageView> m_swapChainImageViews;

		void init();
		void loop();
		void cleanup();
		void querySwapChainSupportDetails(SwapChainSupportDetails* pDetails);
		void getQueueFamilyIndices(QueueFamilyIndices* pIndices);
		void getMostSuitablePhysicalDevice(VkPhysicalDevice* pDevice);

		VkExtent2D getDesiredSwapChainExtent(VkSurfaceCapabilitiesKHR capabilities);
};