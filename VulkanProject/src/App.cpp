#include "App.h"

void windowCloseCallback(GLFWwindow* pWin) {
	glfwSetWindowShouldClose(pWin, GLFW_TRUE);
}

VkPhysicalDevice choosePhysicalDevice(VkInstance instance) {
	uint32_t count;
	vkEnumeratePhysicalDevices(instance, &count, nullptr);
	std::vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(instance, &count, devices.data());

	for (VkPhysicalDevice d : devices) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(d, &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			return d;
		}
	}

	return devices.at(0);
}

void setQueueFamilyIndices(VkPhysicalDevice physDevice, QueueFamilyIndeces* indices, VkSurfaceKHR surface){
	uint32_t count;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, nullptr);
	std::vector<VkQueueFamilyProperties> props(count);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, props.data());


	for (size_t i = 0; i < props.size();i++) {
		VkBool32 surfaceSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, static_cast<uint32_t>(i), surface, &surfaceSupport);
		if (props.at(i).queueFlags && VK_QUEUE_GRAPHICS_BIT && (surfaceSupport == VK_TRUE)) {
			indices->graphicsIndex = static_cast<uint32_t>(i);
		}
	}

	for (size_t i = 0; i < props.size(); i++) {
		if (props.at(i).queueFlags && VK_QUEUE_COMPUTE_BIT) {
			if (i != indices->graphicsIndex) { indices->computeIndex = static_cast<uint32_t>(i); }
		}
	}

	for (size_t i = 0; i < props.size(); i++) {
		if (props.at(i).queueFlags && VK_QUEUE_TRANSFER_BIT) {
			if (i != indices->graphicsIndex && i!=indices->computeIndex) { indices->transferIndex = static_cast<uint32_t>(i); }
		}
	}

	if (indices->graphicsIndex == UINT32_MAX) {
		throw std::runtime_error("A valid graphics queue family is required, but was not found.");
	}
}

void App::run() {
	init();
	loop();
	cleanup();
}

void App::createDevice(){

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	VkDeviceQueueCreateInfo graphicsQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_indices.graphicsIndex,
		.queueCount = 1,
		.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY
	};
	queueCreateInfos.push_back(graphicsQueueCreateInfo);

	if (m_indices.computeIndex != UINT32_MAX) {
		VkDeviceQueueCreateInfo computeQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = m_indices.computeIndex,
			.queueCount = 1,
			.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY
		};
		queueCreateInfos.push_back(computeQueueCreateInfo);
	}

	if (m_indices.transferIndex != UINT32_MAX) {
		VkDeviceQueueCreateInfo transferQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = m_indices.transferIndex,
			.queueCount = 1,
			.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY
		};
		queueCreateInfos.push_back(transferQueueCreateInfo);
	}


	VkDeviceCreateInfo deviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(REQ_DEVICE_EXTENSIONS.size()),
		.ppEnabledExtensionNames = REQ_DEVICE_EXTENSIONS.data(),
		.pEnabledFeatures = nullptr			// will probably be changed later (maybe i should add a features variable but idc)
	};

	if (vkCreateDevice(m_physDevice, &deviceCreateInfo, P_DEFAULT_ALLOCATOR, &m_device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create vulkan device.");
	}
}

void App::init(){
	if (glfwInit() != GL_TRUE) {
		throw std::runtime_error("Failed to initialize GLFW");
	};

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_pWin = glfwCreateWindow(WIDTH, HEIGHT, "Test", nullptr, nullptr);
	uint32_t glfwExtensionCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	VkApplicationInfo applicationInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Test",
		.applicationVersion = 1,
		.apiVersion	= REQUIRED_VULKAN_VER
	};

	VkInstanceCreateInfo instanceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
		.enabledLayerCount = static_cast<uint32_t>(ENABLED_VALIDATION_LAYERS.size()),
		.ppEnabledLayerNames = ENABLED_VALIDATION_LAYERS.data(),
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions
	};

	if (vkCreateInstance(&instanceCreateInfo, P_DEFAULT_ALLOCATOR, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance");
	}

	if (glfwCreateWindowSurface(m_instance, m_pWin, P_DEFAULT_ALLOCATOR, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance");
	}

	m_physDevice = choosePhysicalDevice(m_instance);
	setQueueFamilyIndices(m_physDevice, &m_indices, m_surface);
	createDevice();

	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDevice, m_surface, &capabilities);

	VkSwapchainCreateInfoKHR swapchainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = m_surface,
		.minImageCount = 2,
		.imageFormat = VK_FORMAT_R8G8B8A8_SRGB,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = capabilities.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
	};

	if (vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, P_DEFAULT_ALLOCATOR, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance");
	}	

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkAttachmentDescription colorAttachmentDesc{
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_NONE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};



	VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	VkAttachmentDescription depthAttachmentDesc{
		.format = VK_FORMAT_D32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_NONE,
		.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription graphicsSubpassDesc{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef
	};

	std::vector<VkAttachmentDescription> attachments{ colorAttachmentDesc, depthAttachmentDesc };

	VkRenderPassCreateInfo graphicsPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &graphicsSubpassDesc
	};

	if (vkCreateRenderPass(m_device, &graphicsPassInfo, P_DEFAULT_ALLOCATOR, &m_graphicsPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Render Pass");
	}

	ShaderCompile::compileShader("testvert.vert");
	std::vector<char> vertShaderCode = ShaderCompile::readCompiledShader("testvert.spv");
	VkShaderModule vertShaderModule = ShaderCompile::createShaderModule(m_device, vertShaderCode);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(swapchainCreateInfo.imageExtent.width),
		.height = static_cast<float>(swapchainCreateInfo.imageExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor{
		.offset = {0, 0},
		.extent = swapchainCreateInfo.imageExtent
	};

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main"
	};

	ShaderCompile::compileShader("testfrag.frag");
	std::vector<char> fragShaderCode = ShaderCompile::readCompiledShader("testfrag.spv");
	VkShaderModule fragShaderModule = ShaderCompile::createShaderModule(m_device, fragShaderCode);

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main"
	};

	std::vector<VkPipelineShaderStageCreateInfo> stages{ vertShaderStageInfo, fragShaderStageInfo };

	//feeding in vertex data from a vertex buffer using Vertex struct objects

	VkVertexInputBindingDescription bindingDesc{
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription attribDesc{ //only need position from Vertexes, prob should make this a struct method 
		.location = 0,							  //but whatever (just trying to get it to work)
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(Vertex, pos)
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDesc,
		.vertexAttributeDescriptionCount = 1,
		.pVertexAttributeDescriptions = &attribDesc
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizationState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,				//disabling backface culling temporarily bc im dumb (CHANGE THIS LATER!!!!)
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampleState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachState{
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR,
		.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT & VK_COLOR_COMPONENT_G_BIT & VK_COLOR_COMPONENT_B_BIT & VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachState
	};

	std::vector<VkDynamicState> dynamicVars{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamicVars.size()),
		.pDynamicStates = dynamicVars.data()
	};
	
	VkPipelineLayoutCreateInfo layoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	vkCreatePipelineLayout(m_device, &layoutInfo, P_DEFAULT_ALLOCATOR, &m_pipelineLayout);

	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(stages.size()),
		.pStages = stages.data(),
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pTessellationState = nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = m_pipelineLayout,
		.renderPass = m_graphicsPass,
		.subpass = 0
	};

	if (vkCreateGraphicsPipelines(m_device, nullptr, 1, &graphicsPipelineInfo, P_DEFAULT_ALLOCATOR, &m_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}
}

void App::loop() {
	while (!glfwWindowShouldClose(m_pWin)) {
		glfwPollEvents();
	}
}

void App::cleanup() {
	vkDestroyDevice(m_device, P_DEFAULT_ALLOCATOR);
	vkDestroyInstance(m_instance, P_DEFAULT_ALLOCATOR);
	glfwDestroyWindow(m_pWin);
}