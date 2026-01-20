#include "App.h"

void windowCloseCallback(GLFWwindow* pWin) {
	glfwSetWindowShouldClose(pWin, GL_TRUE);
}

void App::run() {
	init();
	loop();
	cleanup();
}

void App::chooseMostSuitablePhysicalDevice() {
	uint32_t physCount;
	vkEnumeratePhysicalDevices(m_instance, &physCount, nullptr);
	std::vector<VkPhysicalDevice> physDevices(physCount);
	vkEnumeratePhysicalDevices(m_instance, &physCount, physDevices.data());
	m_physDevice = physDevices.at(0);

	for (VkPhysicalDevice device : physDevices) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && props.apiVersion >= MIN_VULKAN_API_VERSION) {
			m_physDevice = device;
		}
	}
}

void App::setQueueIndices() {
	uint32_t count;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &count, nullptr);
	std::vector<VkQueueFamilyProperties> props(count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &count, props.data());

	for (uint32_t i = 0; i < count; i++) {
		if (props.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			m_queueIndices.graphicsIndex = i;
			break;
		}
	}
	for (uint32_t i = 0; i < count; i++) {
		if (props.at(i).queueFlags & VK_QUEUE_COMPUTE_BIT) {
			if (m_queueIndices.graphicsIndex != i) {
				m_queueIndices.computeIndex = i;
				break;
			}
		}
	}
	for (uint32_t i = 0; i < count; i++) {
		if (props.at(i).queueFlags & VK_QUEUE_TRANSFER_BIT) {
			if (m_queueIndices.graphicsIndex != i && m_queueIndices.computeIndex != i) {
				m_queueIndices.transferIndex = i;
				break;
			}
		}
	}
};

void App::createProjectionPipeline() {
	ShaderCompile::compileShader("projectionVert.vert");
	ShaderCompile::compileShader("projectionFrag.frag");
	std::vector<char> vertCode{ ShaderCompile::readCompiledShader("projectionVert.spv") };
	std::vector<char> fragCode{ ShaderCompile::readCompiledShader("projectionFrag.spv") };

	m_projVertModule = ShaderCompile::createShaderModule(m_device, vertCode);
	m_projFragModule = ShaderCompile::createShaderModule(m_device, fragCode);

	VkPipelineShaderStageCreateInfo vertInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = m_projVertModule,
		.pName = "main"
	};
	VkPipelineShaderStageCreateInfo fragInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = m_projFragModule,
		.pName = "main"
	};

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos{ vertInfo, fragInfo };

	VkVertexInputBindingDescription vertexInputBindingInfo{
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBindingInfo
	};
}

void App::init() {

	if (glfwInit() != GL_TRUE) {
		throw std::runtime_error("Failed to Initialize GLFW");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Test", nullptr, nullptr);
	glfwSetWindowCloseCallback(m_pWindow, windowCloseCallback);

	uint32_t glfwReqInstanceExtensionCount;
	const char** glfwReqExtensions{ glfwGetRequiredInstanceExtensions(&glfwReqInstanceExtensionCount) };

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Vulkan Engine",
		.applicationVersion = 1,
		.apiVersion = MIN_VULKAN_API_VERSION
	};

	VkInstanceCreateInfo instanceInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(ENABLED_VALIDATION_LAYERS.size()),
		.ppEnabledLayerNames = ENABLED_VALIDATION_LAYERS.data(),
		.enabledExtensionCount = glfwReqInstanceExtensionCount,
		.ppEnabledExtensionNames = glfwReqExtensions
	};

	if (vkCreateInstance(&instanceInfo, P_DEFAULT_ALLOC, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance");
	}

	if (glfwCreateWindowSurface(m_instance, m_pWindow, P_DEFAULT_ALLOC, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Surface");
	}
	
	chooseMostSuitablePhysicalDevice();
	setQueueIndices();

	float priority = 1.0f;

	VkDeviceQueueCreateInfo graphicsQueueInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_queueIndices.graphicsIndex,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	VkDeviceQueueCreateInfo computeQueueInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_queueIndices.computeIndex,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	VkDeviceQueueCreateInfo transferQueueInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_queueIndices.transferIndex,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	
	std::vector<VkDeviceQueueCreateInfo> queueInfos{ graphicsQueueInfo, computeQueueInfo, transferQueueInfo };

	VkDeviceCreateInfo deviceInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
		.pQueueCreateInfos = queueInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(ENABLED_DEVICE_EXTENSIONS.size()),
		.ppEnabledExtensionNames = ENABLED_DEVICE_EXTENSIONS.data()
	};

	if (vkCreateDevice(m_physDevice, &deviceInfo, P_DEFAULT_ALLOC, &m_device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Device");
	}

	vkGetDeviceQueue(m_device, m_queueIndices.graphicsIndex, 0, &m_graphicsQueue);

	VkCommandPoolCreateInfo cmdPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_queueIndices.graphicsIndex
	};

	if (vkCreateCommandPool(m_device, &cmdPoolInfo, P_DEFAULT_ALLOC, &m_cmdPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Command Pool");
	}

	VkCommandBufferAllocateInfo cmdBufferInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_cmdPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
	};
	
	if (vkAllocateCommandBuffers(m_device, &cmdBufferInfo, &m_cmdBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffer");
	}

	VkSurfaceCapabilitiesKHR surfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDevice, m_surface, &surfaceCaps);

	VkSwapchainCreateInfoKHR swapchainInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = m_surface,
		.minImageCount = 2,
		.imageFormat = VK_FORMAT_R8G8B8A8_SRGB,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = surfaceCaps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = surfaceCaps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE
	};

	if (vkCreateSwapchainKHR(m_device, &swapchainInfo, P_DEFAULT_ALLOC, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Swapchain");
	}

	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages = std::vector<VkImage>(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

	createProjectionPipeline();
}

void App::loop() {
	uint32_t renderImageIndex;
	vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSem, VK_NULL_HANDLE, &renderImageIndex);

	VkSubmitInfo renderSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_imageAvailableSem,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_cmdBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &m_renderCompleteSem
	};

	vkQueueSubmit(m_graphicsQueue, 1, &renderSubmitInfo, VK_NULL_HANDLE);
	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_renderCompleteSem,
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain,
		.pImageIndices = &renderImageIndex
	};

	vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
}

void App::cleanup() {
	vkDestroySwapchainKHR(m_device, m_swapchain, P_DEFAULT_ALLOC);
	vkDestroyCommandPool(m_device, m_cmdPool, P_DEFAULT_ALLOC);
	vkDestroyDevice(m_device, P_DEFAULT_ALLOC);
	vkDestroySurfaceKHR(m_instance, m_surface, P_DEFAULT_ALLOC);
	vkDestroyInstance(m_instance, P_DEFAULT_ALLOC);
	glfwDestroyWindow(m_pWindow);
}