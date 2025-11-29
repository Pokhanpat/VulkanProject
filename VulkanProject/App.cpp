#include "App.h"

#ifndef NDEBUG
const char* ACTIVE_LAYERS{
	"VK_LAYER_KHRONOS_validation"
};
const uint32_t LAYER_COUNT = 1;
#elif
const char* ACTIVE_LAYERS{};
const uint32_t LAYER_COUNT = 0;
#endif

VkPhysicalDevice getValidPhysicalDevice(std::vector<VkPhysicalDevice>* dList) {
	for (VkPhysicalDevice device : *dList) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && properties.apiVersion >= REQ_VULKAN_VERSION){
			std::cout << properties.deviceName << std::endl;
			return device;
		}
	}
	return (dList->front());
}

uint32_t getValidQueueFamily(std::vector<VkQueueFamilyProperties>* props) {
	for (uint32_t i = 0; i < props->size(); i++) {
		if ((props->at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)!=0) {
			return i;
		}
	}
	throw std::runtime_error("No Valid Queue Family Found!");
}

void App::run() {
	init();
	loop();
	cleanup();
}

void App::init() {
	VkInstanceCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &m_appInfo,
		.enabledLayerCount = LAYER_COUNT,
		.ppEnabledLayerNames = &ACTIVE_LAYERS,
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = nullptr
	};

	if (vkCreateInstance(&info, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Vulkan instance!");
	}
	
	vkEnumeratePhysicalDevices(m_instance, &m_physCount, nullptr);
	std::vector<VkPhysicalDevice> devices(m_physCount);
	vkEnumeratePhysicalDevices(m_instance, &m_physCount, devices.data());

	m_physDevice = getValidPhysicalDevice(&devices);
	if (m_physDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find valid GPU!");
	}

	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &queueFamilyPropertyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());
	
	float priorityVal = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = getValidQueueFamily(&queueFamilyProperties),
		.queueCount = 1,
		.pQueuePriorities = &priorityVal
	};

	VkDeviceCreateInfo logInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = nullptr,
		.pEnabledFeatures = nullptr
	};
	
	vkCreateDevice(m_physDevice, &logInfo, nullptr, &m_device);
	if (m_device == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to create logical device!");
	}
}

void App::loop() {}
void App::cleanup() {
	vkDeviceWaitIdle(m_device);
	vkDestroyDevice(m_device, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}