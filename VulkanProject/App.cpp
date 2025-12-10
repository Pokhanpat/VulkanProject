#include "App.h"

bool QueueFamilyIndices::areIndicesSet() {
	return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
}

void windowCloseCallback(GLFWwindow* pWin) {
	glfwSetWindowShouldClose(pWin, GLFW_TRUE);
}

void App::getQueueFamilyIndices(QueueFamilyIndices* pIndices) {
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> props(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &queueFamilyCount, props.data());

	for (size_t i = 0; i < queueFamilyCount; i++) {
		if (props.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			pIndices->graphicsFamily = i;
			break;
		}
	}
	for (size_t i = 0; i < queueFamilyCount; i++) {
		if (props.at(i).queueFlags & VK_QUEUE_COMPUTE_BIT) {
			if (i != pIndices->graphicsFamily.value()) {
				pIndices->computeFamily = i;
				break;
			}
		}
	}
	for (size_t i = 0; i < queueFamilyCount; i++) {
		if (props.at(i).queueFlags & VK_QUEUE_TRANSFER_BIT) {
			if (i != pIndices->graphicsFamily.value() && i != pIndices->computeFamily.value()) {
				pIndices->transferFamily = i;
				break;
			}
		}
	}
}

void App::getMostSuitablePhysicalDevice(VkPhysicalDevice * pDevice) {
	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physDeviceCount, nullptr);
	if (physDeviceCount == 0) {
		std::runtime_error("Failed to find Vulkan-enabled Graphics Device!");
	}
	std::vector<VkPhysicalDevice> physDevices(physDeviceCount);
	vkEnumeratePhysicalDevices(m_instance, &physDeviceCount, physDevices.data());

	for (VkPhysicalDevice device : physDevices) {
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(device, &props);

		if (props.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			*pDevice = device;
			return;
		}
	}
	*pDevice = physDevices.at(0);
}

void App::run() {
	init();
	loop();
	cleanup();
}

void App::init() {
	if (glfwInit() != GLFW_TRUE) {
		std::runtime_error("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Test App", nullptr, nullptr);
	glfwSetWindowCloseCallback(m_pWindow, windowCloseCallback);

	VkApplicationInfo appInfo{
	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	.pApplicationName = "Test App",
	.applicationVersion = VK_MAKE_VERSION(1,0,0),
	.pEngineName = "None",
	.engineVersion = VK_MAKE_VERSION(1,0,0),
	.apiVersion = REQ_VULKAN_VERSION
	};

	uint32_t glfwExtCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

	VkInstanceCreateInfo instanceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = glfwExtCount,
		.ppEnabledExtensionNames = glfwExtensions
	};

	if (ENABLE_VALIDATION) {
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ENABLED_VALIDATION_LAYERS.size());
		instanceCreateInfo.ppEnabledLayerNames = ENABLED_VALIDATION_LAYERS.data();
	}

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS) {
		std::runtime_error("Failed to create Vulkan instance.");
	}
	
	getMostSuitablePhysicalDevice(&m_physDevice);
	getQueueFamilyIndices(&m_queueFamilyIndices);

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo graphicsQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value(),
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};
	VkDeviceQueueCreateInfo computeQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = m_queueFamilyIndices.computeFamily.value(),
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};
	VkDeviceQueueCreateInfo transferQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = m_queueFamilyIndices.transferFamily.value(),
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	VkDeviceQueueCreateInfo queues[]{graphicsQueueCreateInfo, computeQueueCreateInfo, transferQueueCreateInfo};
	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 3,
		.pQueueCreateInfos = queues,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = nullptr,
		.pEnabledFeatures = &REQUIRED_DEVICE_FEATURES
	};

		if (vkCreateDevice(m_physDevice, &createInfo, nullptr, &m_logDevice) != VK_SUCCESS) {
		std::runtime_error("Failed to create Vulkan device.");
	}
}

void App::loop() {
	while (!glfwWindowShouldClose(m_pWindow)) {
		glfwPollEvents();
	}
}

void App::cleanup() {
	vkDestroyDevice(m_logDevice, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}