#include "App.h"

void windowCloseCallback(GLFWwindow* pWin) {
	glfwSetWindowShouldClose(pWin, GL_TRUE);
}

Renderer::~Renderer() {
	cleanup();
}

void Renderer::run() {
	init();
	loop();
	cleanup();
}

void Renderer::chooseMostSuitablePhysicalDevice() {
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

void Renderer::setQueueIndices() {
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

void Renderer::createRenderPass(){
	VkAttachmentDescription colorAttachmentDesc{
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_NONE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_NONE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_NONE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachmentDesc{
		.format = VK_FORMAT_R32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_NONE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_NONE,
		.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	std::vector<VkAttachmentDescription> attachDescs = { colorAttachmentDesc, depthAttachmentDesc };

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpassDesc{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
		.preserveAttachmentCount = 0
	};

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachDescs.size()),
		.pAttachments = attachDescs.data(),
		.subpassCount = 1,
		.pSubpasses = &subpassDesc
	};

	if (vkCreateRenderPass(m_device, &renderPassInfo, P_DEFAULT_ALLOC, &m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Render Pass");
	}

	VkImageCreateInfo colorAttachImageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.extent = {
			.width = m_surfaceExtent.width,
			.height = m_surfaceExtent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkImageCreateInfo depthAttachImageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R32_SFLOAT,
		.extent = VkExtent3D{
			.width = m_surfaceExtent.width,
			.height = m_surfaceExtent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	vkCreateImage(m_device, &colorAttachImageInfo, P_DEFAULT_ALLOC, &m_colorAttachImage);
	vkCreateImage(m_device, &depthAttachImageInfo, P_DEFAULT_ALLOC, &m_depthAttachImage);

	VkImageViewCreateInfo colorAttachViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = m_colorAttachImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.components = VkComponentMapping{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = VkImageSubresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	VkImageViewCreateInfo depthAttachViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = m_colorAttachImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R32_SFLOAT,
		.components = VkComponentMapping{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = VkImageSubresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vkCreateImageView(m_device, &colorAttachViewInfo, P_DEFAULT_ALLOC, &m_colorAttachView);
	vkCreateImageView(m_device, &depthAttachViewInfo, P_DEFAULT_ALLOC, &m_depthAttachView);

	std::vector<VkImageView> attachments{ m_colorAttachView, m_depthAttachView };

	VkFramebufferCreateInfo framebufferInfo{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = m_renderPass,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.width = m_surfaceExtent.width,
		.height = m_surfaceExtent.height,
		.layers = 1
	};

	vkCreateFramebuffer(m_device, &framebufferInfo, P_DEFAULT_ALLOC, &m_framebuffer);
}

void Renderer::preparePipelineData() {
	VkBufferCreateInfo projectionDataBufferInfo{
	.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	.size = sizeof(CameraProjectionData),
	.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	.sharingMode = VK_SHARING_MODE_EXCLUSIVE //MAKE SURE TO CHANGE THIS IF CAMERA DATA IS USED FOR NON GRAPHICS GPU STUFF
	};

	vkCreateBuffer(m_device, &projectionDataBufferInfo, P_DEFAULT_ALLOC, &m_projectionDataBuf);

	VkDescriptorSetLayoutBinding lowFreqDescSetLayoutBinding{
		.binding = BINDING_LOW_FREQ,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	};

	VkDescriptorSetLayoutCreateInfo lowFreqDescSetLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &lowFreqDescSetLayoutBinding
	};

	vkCreateDescriptorSetLayout(m_device, &lowFreqDescSetLayoutInfo, P_DEFAULT_ALLOC, &m_lowFreqDescSetLayout);
}

void Renderer::createProjectionPipeline() {
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
		.binding = BINDING_VERTEX_BUFFER,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBindingInfo
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineTessellationStateCreateInfo tessellationStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.patchControlPoints = 1
	};

	VkViewport viewport{
		.x=0.0f,
		.y=0.0f,
		.width = static_cast<float>(m_surfaceExtent.width),
		.height = static_cast<float>(m_surfaceExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor{
		.offset = {
			.x=0,
			.y=0
		},
		.extent = m_surfaceExtent
	};

	VkPipelineViewportStateCreateInfo viewportStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_TRUE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE, //           *******CHANGE THIS LATER!!!!!!!!*******
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE
	};

	VkPipelineMultisampleStateCreateInfo multisampleStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.pSampleMask = nullptr
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 0
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &m_lowFreqDescSetLayout,
		.pushConstantRangeCount = 0
	};


	vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, P_DEFAULT_ALLOC, &m_pipelineLayout);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStageInfos.size()),
		.pStages = shaderStageInfos.data(),
		.pVertexInputState = &vertexInputStateInfo,
		.pInputAssemblyState = &inputAssemblyStateInfo,
		.pTessellationState = &tessellationStateInfo,
		.pViewportState = &viewportStateInfo,
		.pRasterizationState = &rasterizationStateInfo,
		.pMultisampleState = &multisampleStateInfo,
		.pDepthStencilState = &depthStencilStateInfo,
		.pColorBlendState = &colorBlendStateInfo,
		.pDynamicState = nullptr,
		.layout = m_pipelineLayout,
		.renderPass = m_renderPass,
		.subpass = 0
	};

	vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, P_DEFAULT_ALLOC, &m_pipeline);
}

void Renderer::init() {

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
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
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
	m_surfaceExtent = surfaceCaps.currentExtent;

	VkSwapchainCreateInfoKHR swapchainInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = m_surface,
		.minImageCount = 2,
		.imageFormat = VK_FORMAT_R8G8B8A8_SRGB,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = m_surfaceExtent,
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

	
	createRenderPass();
	preparePipelineData();
	createProjectionPipeline();


}

void Renderer::loop() {
	uint32_t renderImageIndex;
	vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSem, VK_NULL_HANDLE, &renderImageIndex);
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkBeginCommandBuffer(m_cmdBuffer, &beginInfo);

	VkRenderPassBeginInfo passBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_renderPass,
		.framebuffer = m_framebuffer,
		.renderArea = VkRect2D{
			.offset = VkOffset2D{.x = 0,.y=0},
			.extent = m_surfaceExtent
		},
		.clearValueCount = 0
	};

	vkCmdBeginRenderPass(m_cmdBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	vkCmdEndRenderPass(m_cmdBuffer);
	vkEndCommandBuffer(m_cmdBuffer);

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
	vkResetCommandBuffer(m_cmdBuffer, 0);
}

void Renderer::cleanup() {
	vkDestroyPipeline(m_device, m_pipeline, P_DEFAULT_ALLOC);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, P_DEFAULT_ALLOC);
	vkDestroyDescriptorSetLayout(m_device, m_lowFreqDescSetLayout, P_DEFAULT_ALLOC);
	vkDestroyBuffer(m_device, m_projectionDataBuf, P_DEFAULT_ALLOC);
	vkDestroyFramebuffer(m_device, m_framebuffer, P_DEFAULT_ALLOC);
	vkDestroyImageView(m_device, m_depthAttachView, P_DEFAULT_ALLOC);
	vkDestroyImageView(m_device, m_colorAttachView, P_DEFAULT_ALLOC);
	vkDestroyImage(m_device, m_depthAttachImage, P_DEFAULT_ALLOC);
	vkDestroyImage(m_device, m_colorAttachImage, P_DEFAULT_ALLOC);
	vkDestroyRenderPass(m_device, m_renderPass, P_DEFAULT_ALLOC);
	vkDestroySwapchainKHR(m_device, m_swapchain, P_DEFAULT_ALLOC);
	vkDestroyCommandPool(m_device, m_cmdPool, P_DEFAULT_ALLOC);
	vkDestroyDevice(m_device, P_DEFAULT_ALLOC);
	vkDestroySurfaceKHR(m_instance, m_surface, P_DEFAULT_ALLOC);
	vkDestroyInstance(m_instance, P_DEFAULT_ALLOC);
	glfwDestroyWindow(m_pWindow);
}