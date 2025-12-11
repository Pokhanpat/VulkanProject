#include "App.h"

bool QueueFamilyIndices::areIndicesSet() {
	return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
}

void windowCloseCallback(GLFWwindow* pWin) {
	glfwSetWindowShouldClose(pWin, GLFW_TRUE);
}

bool checkDeviceExtensionSupport(VkPhysicalDevice* device){
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> reqExtensions(ENABLED_DEVICE_EXTENSIONS.begin(), ENABLED_DEVICE_EXTENSIONS.end());
	for (const VkExtensionProperties& extension : availableExtensions) {
		reqExtensions.erase(extension.extensionName);
	}

	return reqExtensions.empty();
}

VkSurfaceFormatKHR getMostSuitableSurfaceFormat(std::vector<VkSurfaceFormatKHR> formats) {
	for (VkSurfaceFormatKHR& format : formats) {
		if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}
	return formats.at(0);
}

VkPresentModeKHR getMostSuitablePresentMode(std::vector<VkPresentModeKHR> presentModes) {
	for (VkPresentModeKHR& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D App::getDesiredSwapChainExtent(VkSurfaceCapabilitiesKHR capabilities) {
	if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(m_pWindow, &width, &height);

	VkExtent2D extent{
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height)
	};

	extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return extent;
}

void App::querySwapChainSupportDetails(SwapChainSupportDetails* pDetails) {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDevice, m_surface, &(pDetails->capabilities));

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDevice, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		pDetails->formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDevice, m_surface, &formatCount, pDetails->formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physDevice, m_surface, &presentModeCount, nullptr);

	if (formatCount != 0) {
		pDetails->formats.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physDevice, m_surface, &presentModeCount, pDetails->presentModes.data());
	}
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
		throw std::runtime_error("Failed to find Vulkan-enabled Graphics Device!");
	}
	std::vector<VkPhysicalDevice> physDevices(physDeviceCount);
	vkEnumeratePhysicalDevices(m_instance, &physDeviceCount, physDevices.data());

	*pDevice = physDevices.at(0);
	for (VkPhysicalDevice device : physDevices) {
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(device, &props);

		bool adequateSwapChain = false;
		SwapChainSupportDetails details;
		querySwapChainSupportDetails(&details);
		adequateSwapChain = !details.formats.empty() && !details.presentModes.empty();

		if ((props.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && checkDeviceExtensionSupport(&device) && adequateSwapChain) {
			*pDevice = device;
		}
	}
}

void App::run() {
	init();
	loop();
	cleanup();
}

void App::initGLFW() {
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Test App", nullptr, nullptr);
	glfwSetWindowCloseCallback(m_pWindow, windowCloseCallback);
}

void App::createInstance() {
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
		throw std::runtime_error("Failed to create Vulkan instance.");
	}
}

void App::createSurface() {
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{
	.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
	.pNext = nullptr,
	.flags = 0,
	.hinstance = GetModuleHandle(nullptr),
	.hwnd = glfwGetWin32Window(m_pWindow)
	};

	if (glfwCreateWindowSurface(m_instance, m_pWindow, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create GLFW Surface");
	}
}

void App::createDevice() {
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

	VkDeviceQueueCreateInfo queues[]{ graphicsQueueCreateInfo, computeQueueCreateInfo, transferQueueCreateInfo };
	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 3,
		.pQueueCreateInfos = queues,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(ENABLED_DEVICE_EXTENSIONS.size()),
		.ppEnabledExtensionNames = ENABLED_DEVICE_EXTENSIONS.data(),
		.pEnabledFeatures = &REQUIRED_DEVICE_FEATURES
	};

	if (vkCreateDevice(m_physDevice, &createInfo, nullptr, &m_logDevice) != VK_SUCCESS) {
		std::cout << vkCreateDevice(m_physDevice, &createInfo, nullptr, &m_logDevice) << std::endl;
		throw std::runtime_error("Failed to create Vulkan device.");
	}

	VkBool32 win32Support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_physDevice, m_queueFamilyIndices.graphicsFamily.value(), m_surface, &win32Support);
	if (!win32Support) {
		throw std::runtime_error("Graphics device doesn't support the Windows API");
	}
	vkGetDeviceQueue(m_logDevice, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
}

void App::createSwapChain() {
	SwapChainSupportDetails swapChainDetails;
	querySwapChainSupportDetails(&swapChainDetails);

	VkSurfaceFormatKHR surfaceFormat = getMostSuitableSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode = getMostSuitablePresentMode(swapChainDetails.presentModes);
	VkExtent2D extent = getDesiredSwapChainExtent(swapChainDetails.capabilities);

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;

	uint32_t imgCount = swapChainDetails.capabilities.minImageCount + 1;
	if (swapChainDetails.capabilities.maxImageCount > 0 && imgCount > swapChainDetails.capabilities.maxImageCount) { imgCount--; }

	VkSwapchainCreateInfoKHR swapChainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = m_surface,
		.minImageCount = imgCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.preTransform = swapChainDetails.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	if (vkCreateSwapchainKHR(m_logDevice, &swapChainCreateInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan swapchain");
	}

	vkGetSwapchainImagesKHR(m_logDevice, m_swapChain, &imgCount, nullptr);
	m_swapChainImages.resize(imgCount);
	vkGetSwapchainImagesKHR(m_logDevice, m_swapChain, &imgCount, m_swapChainImages.data());

	m_swapChainImageViews.resize(m_swapChainImages.size());
	for (size_t i = 0; i < m_swapChainImages.size(); i++) {
		VkImageViewCreateInfo IVCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = m_swapChainImages.at(i),
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_swapChainImageFormat,
			.components{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(m_logDevice, &IVCreateInfo, nullptr, &m_swapChainImageViews.at(i)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Image Views.");
		}
	}
}

void App::createRenderPass() {
	VkAttachmentDescription colorAttachment{
		.flags = 0,
		.format = m_swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentReference{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass{
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentReference,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass
	};

	if (vkCreateRenderPass(m_logDevice, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Render Pass.");
	}
}

void App::createRenderPipeline() {

	std::vector<char> vertShader = ShaderCompile::readShader("testvert.spv");
	std::vector<char> fragShader = ShaderCompile::readShader("testfrag.spv");

	VkShaderModule vertShaderModule = ShaderCompile::createShaderModule(m_logDevice, vertShader);
	VkShaderModule fragShaderModule = ShaderCompile::createShaderModule(m_logDevice, fragShader);

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main",
		.pSpecializationInfo = nullptr
	};

	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	.pNext = nullptr,
	.flags = 0,
	.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	.module = fragShaderModule,
	.pName = "main",
	.pSpecializationInfo = nullptr
	};

	VkPipelineShaderStageCreateInfo shaderStages[]{ vertexShaderStageInfo, fragmentShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(m_swapChainExtent.width),
		.height = static_cast<float>(m_swapChainExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissorRect{
		.offset = {0, 0},
		.extent = m_swapChainExtent
	};

	VkPipelineViewportStateCreateInfo viewportStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissorRect
	};

	VkPipelineRasterizationStateCreateInfo rasterizationInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampleInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	VkPipelineColorBlendAttachmentState colorBlendAtt{
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAtt
	};

	if (vkCreatePipelineLayout(m_logDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Graphics Pipeline Layout");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = nullptr,
		.pViewportState = &viewportStateInfo,
		.pRasterizationState = &rasterizationInfo,
		.pMultisampleState = &multisampleInfo,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = nullptr,
		.layout = m_pipelineLayout,
		.renderPass = m_renderPass,
		.subpass = 0,
	};

	if (vkCreateGraphicsPipelines(m_logDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline.");
	}

	vkDestroyShaderModule(m_logDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logDevice, fragShaderModule, nullptr);
}

void App::init() {
	initGLFW();
	createInstance();
	createSurface();
	
	getMostSuitablePhysicalDevice(&m_physDevice);
	getQueueFamilyIndices(&m_queueFamilyIndices);

	createDevice();
	createSwapChain();

	createRenderPass();
	createRenderPipeline();
}

void App::loop() {
	while (!glfwWindowShouldClose(m_pWindow)) {
		glfwPollEvents();
	}
}

void App::cleanup() {
	vkDestroyPipeline(m_logDevice, m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_logDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_logDevice, m_renderPass, nullptr);
	for (VkImageView IV : m_swapChainImageViews) {vkDestroyImageView(m_logDevice, IV, nullptr);}
	vkDestroySwapchainKHR(m_logDevice, m_swapChain, nullptr);
	vkDestroyDevice(m_logDevice, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}