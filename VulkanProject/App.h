#include <vulkan/vulkan.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>

const uint32_t REQ_VULKAN_VERSION = VK_API_VERSION_1_0;

class App {
	public:
		void run();
	private:
		VkInstance m_instance;
		uint32_t m_physCount = 0;
		VkPhysicalDevice m_physDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;

		VkApplicationInfo m_appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "TestApp",
			.applicationVersion = VK_MAKE_VERSION(0, 0, 0),
			.pEngineName = "None",
			.engineVersion = VK_MAKE_VERSION(0, 0, 0),
			.apiVersion = REQ_VULKAN_VERSION
		};

		void init();
		void loop();
		void cleanup();
};