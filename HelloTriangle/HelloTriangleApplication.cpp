#include "HelloTriangleApplication.h"
#include <map>
#include <set>
#include <algorithm>
#include <fstream>

namespace
{
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	const int MAX_FRAMES_IN_FLIGHT = 2;
	const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_LUNARG_standard_validation" };
	const std::vector<const char*> DEVICE_EXTNESIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::run()
{
	__init();
	__mainLoop();
	__cleanup();
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__init()
{
#ifdef _DEBUG
	m_EnableValidationLayers = true;
#else
	m_EnableValidationLayers = false;
#endif

	__initWindow();
	__initVulkan();
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_pGLFWWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
}

//******************************************************************************************
//FUNCTION:
static VKAPI_ATTR VkBool32 VKAPI_CALL __debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT vMessageType,
	const VkDebugUtilsMessengerCallbackDataEXT* vCallbackData,
	void* vUserData)
{
	std::cerr << "validation layer: " << vCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

//******************************************************************************************
//FUNCTION:
static std::vector<char> __readFile(const std::string& vFilename) {
	std::ifstream File(vFilename, std::ios::ate | std::ios::binary);

	if (!File.is_open())
		throw std::runtime_error("failed to open file!");

	size_t FileSize = (size_t)File.tellg();
	std::vector<char> Buffer(FileSize);

	File.seekg(0);
	File.read(Buffer.data(), FileSize);

	File.close();

	return Buffer;
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__initVulkan()
{
	__createVulkanInstance();
	__setupDebugCallback();
	__createSurface();
	__pickPhysicalDevice();
	__createLogicalDevice();
	__createSwapChain();
	__createImageViews();
	__createRenderPass();
	__createGraphicsPipeline();
	__createFrameBuffers();
	__createCommandPool();
	__createCommandBuffers();
	__createSyncObjects();
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__drawFrame()
{
	vkWaitForFences(m_VkDevice, 1, &m_VkInFlightFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(m_VkDevice, 1, &m_VkInFlightFences[m_CurrentFrame]);

	uint32_t ImageIndex;
	vkAcquireNextImageKHR(m_VkDevice, m_VkSwapChain, std::numeric_limits<uint64_t>::max(), m_VkImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore WaitSemaphores[] = { m_VkImageAvailableSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores = WaitSemaphores;
	SubmitInfo.pWaitDstStageMask = WaitStages;

	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &m_VkCommandBuffers[ImageIndex];

	VkSemaphore SignalSemaphores[] = { m_VkRenderFinishedSemaphores[m_CurrentFrame] };
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores = SignalSemaphores;

	if (vkQueueSubmit(m_VkGraphicsQueue, 1, &SubmitInfo, m_VkInFlightFences[m_CurrentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");

	VkPresentInfoKHR PresentInfo = {};
	PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = SignalSemaphores;

	VkSwapchainKHR SwapChains[] = { m_VkSwapChain };
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = SwapChains;

	PresentInfo.pImageIndices = &ImageIndex;

	vkQueuePresentKHR(m_VkPresentQueue, &PresentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__pickPhysicalDevice()
{
	uint32_t DeviceCount = 0;
	vkEnumeratePhysicalDevices(m_VkInstance, &DeviceCount, nullptr);

	if (0 == DeviceCount) throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> Devices(DeviceCount);
	vkEnumeratePhysicalDevices(m_VkInstance, &DeviceCount, Devices.data());

	std::multimap<int, VkPhysicalDevice> Candidates;
	for (const auto& Device : Devices)
	{
		if (__isDeviceSuitable(Device))
		{
			m_VkPhysicalDevice = Device;
			break;
		}
	}

	if (m_VkPhysicalDevice == VK_NULL_HANDLE) throw std::runtime_error("failed to find a suitable GPU!");
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createLogicalDevice()
{
	SQueueFamilyIndices Indices = __findQueueFamilies(m_VkPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::set<uint32_t> UniqueQueueFamilies = { Indices.GraphicsFamily.value(), Indices.PresentFamily.value() };

	float QueuePriority = 1.0f;
	for (uint32_t QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = {};
		QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfo.queueFamilyIndex = QueueFamily;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};

	VkDeviceCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	CreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();

	CreateInfo.pEnabledFeatures = &DeviceFeatures;

	CreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTNESIONS.size());
	CreateInfo.ppEnabledExtensionNames = DEVICE_EXTNESIONS.data();

	if (m_EnableValidationLayers)
	{
		CreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		CreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else
	{
		CreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_VkPhysicalDevice, &CreateInfo, nullptr, &m_VkDevice) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(m_VkDevice, Indices.GraphicsFamily.value(), 0, &m_VkGraphicsQueue);
	vkGetDeviceQueue(m_VkDevice, Indices.PresentFamily.value(), 0, &m_VkPresentQueue);
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createSwapChain()
{
	SSwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(m_VkPhysicalDevice);

	VkSurfaceFormatKHR SurfaceFormat = __chooseSwapSurfaceFormat(SwapChainSupport.Formats);
	VkPresentModeKHR PresentMode = __chooseSwapPresentMode(SwapChainSupport.PresentModes);
	VkExtent2D Extent = __chooseSwapExtent(SwapChainSupport.Capabilities);

	uint32_t ImageCount = SwapChainSupport.Capabilities.minImageCount + 1;
	if (SwapChainSupport.Capabilities.maxImageCount > 0 && ImageCount > SwapChainSupport.Capabilities.maxImageCount)
	{
		ImageCount = SwapChainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	CreateInfo.surface = m_VkSurface;

	CreateInfo.minImageCount = ImageCount;
	CreateInfo.imageFormat = SurfaceFormat.format;
	CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
	CreateInfo.imageExtent = Extent;
	CreateInfo.imageArrayLayers = 1;
	CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	SQueueFamilyIndices Indices = __findQueueFamilies(m_VkPhysicalDevice);
	uint32_t QueueFamilyIndices[] = { Indices.GraphicsFamily.value(), Indices.PresentFamily.value() };

	if (Indices.GraphicsFamily != Indices.PresentFamily)
	{
		CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		CreateInfo.queueFamilyIndexCount = 2;
		CreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else
	{
		CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	CreateInfo.preTransform = SwapChainSupport.Capabilities.currentTransform;
	CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	CreateInfo.presentMode = PresentMode;
	CreateInfo.clipped = VK_TRUE;

	CreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_VkDevice, &CreateInfo, nullptr, &m_VkSwapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain!");

	vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &ImageCount, nullptr);
	m_VkSwapChainImages.resize(ImageCount);
	vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &ImageCount, m_VkSwapChainImages.data());

	m_VkSwapChainImageFormat = SurfaceFormat.format;
	m_VkSwapChainExtent = Extent;
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createImageViews()
{
	m_VkSwapChainImageViews.resize(m_VkSwapChainImages.size());

	for (size_t i = 0; i < m_VkSwapChainImages.size(); ++i)
	{
		VkImageViewCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		CreateInfo.image = m_VkSwapChainImages[i];
		CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		CreateInfo.format = m_VkSwapChainImageFormat;
		CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		CreateInfo.subresourceRange.baseMipLevel = 0;
		CreateInfo.subresourceRange.levelCount = 1;
		CreateInfo.subresourceRange.baseArrayLayer = 0;
		CreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_VkDevice, &CreateInfo, nullptr, &m_VkSwapChainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create image views!");
	}
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createRenderPass()
{
	VkAttachmentDescription ColorAttachment = {};
	ColorAttachment.format = m_VkSwapChainImageFormat;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference ColorAttachmentRef = {};
	ColorAttachmentRef.attachment = 0;
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachmentRef;

	VkRenderPassCreateInfo RenderPassInfo = {};
	RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	RenderPassInfo.attachmentCount = 1;
	RenderPassInfo.pAttachments = &ColorAttachment;
	RenderPassInfo.subpassCount = 1;
	RenderPassInfo.pSubpasses = &Subpass;

	if (vkCreateRenderPass(m_VkDevice, &RenderPassInfo, nullptr, &m_VkRenderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createGraphicsPipeline()
{
	auto VertShaderCode = __readFile("shaders/vert.spv");
	auto FragShaderCode = __readFile("shaders/frag.spv");

	VkShaderModule VertShaderModule = __createShaderModule(VertShaderCode);
	VkShaderModule FragShaderModule = __createShaderModule(FragShaderCode);

	VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
	VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertShaderStageInfo.module = VertShaderModule;
	VertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
	FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragShaderStageInfo.module = FragShaderModule;
	FragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = 0;
	VertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};
	InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport Viewport = {};
	Viewport.x = 0.0f;
	Viewport.y = 0.0f;
	Viewport.width = (float)m_VkSwapChainExtent.width;
	Viewport.height = (float)m_VkSwapChainExtent.height;
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;

	VkRect2D Scissor = {};
	Scissor.offset = { 0, 0 };
	Scissor.extent = m_VkSwapChainExtent;

	VkPipelineViewportStateCreateInfo ViewportState = {};
	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.viewportCount = 1;
	ViewportState.pViewports = &Viewport;
	ViewportState.scissorCount = 1;
	ViewportState.pScissors = &Scissor;

	VkPipelineRasterizationStateCreateInfo Rasterizer = {};
	Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	Rasterizer.depthClampEnable = VK_FALSE;
	Rasterizer.rasterizerDiscardEnable = VK_FALSE;
	Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	Rasterizer.lineWidth = 1.0f;
	Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	Rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo Multisampling = {};
	Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	Multisampling.sampleShadingEnable = VK_FALSE;
	Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo ColorBlending = {};
	ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = VK_LOGIC_OP_COPY;
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &ColorBlendAttachment;
	ColorBlending.blendConstants[0] = 0.0f;
	ColorBlending.blendConstants[1] = 0.0f;
	ColorBlending.blendConstants[2] = 0.0f;
	ColorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutInfo.setLayoutCount = 0;
	PipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_VkDevice, &PipelineLayoutInfo, nullptr, &m_VkPipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &VertexInputInfo;
	pipelineInfo.pInputAssemblyState = &InputAssembly;
	pipelineInfo.pViewportState = &ViewportState;
	pipelineInfo.pRasterizationState = &Rasterizer;
	pipelineInfo.pMultisampleState = &Multisampling;
	pipelineInfo.pColorBlendState = &ColorBlending;
	pipelineInfo.layout = m_VkPipelineLayout;
	pipelineInfo.renderPass = m_VkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_VkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkGraphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(m_VkDevice, FragShaderModule, nullptr);
	vkDestroyShaderModule(m_VkDevice, VertShaderModule, nullptr);
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createFrameBuffers()
{
	m_VkSwapChainFramebuffers.resize(m_VkSwapChainImageViews.size());

	for (size_t i = 0; i < m_VkSwapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { m_VkSwapChainImageViews[i] };

		VkFramebufferCreateInfo FramebufferInfo = {};
		FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferInfo.renderPass = m_VkRenderPass;
		FramebufferInfo.attachmentCount = 1;
		FramebufferInfo.pAttachments = attachments;
		FramebufferInfo.width = m_VkSwapChainExtent.width;
		FramebufferInfo.height = m_VkSwapChainExtent.height;
		FramebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_VkDevice, &FramebufferInfo, nullptr, &m_VkSwapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createCommandPool()
{
	SQueueFamilyIndices QueueFamilyIndices = __findQueueFamilies(m_VkPhysicalDevice);

	VkCommandPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	PoolInfo.queueFamilyIndex = QueueFamilyIndices.GraphicsFamily.value();

	if (vkCreateCommandPool(m_VkDevice, &PoolInfo, nullptr, &m_VkCommandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createCommandBuffers()
{
	m_VkCommandBuffers.resize(m_VkSwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.commandPool = m_VkCommandPool;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandBufferCount = (uint32_t)m_VkCommandBuffers.size();

	if (vkAllocateCommandBuffers(m_VkDevice, &AllocInfo, m_VkCommandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers!");

	for (size_t i = 0; i < m_VkCommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		if (vkBeginCommandBuffer(m_VkCommandBuffers[i], &BeginInfo) != VK_SUCCESS)
			throw std::runtime_error("failed to begin recording command buffer!");

		VkRenderPassBeginInfo RenderPassInfo = {};
		RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassInfo.renderPass = m_VkRenderPass;
		RenderPassInfo.framebuffer = m_VkSwapChainFramebuffers[i];
		RenderPassInfo.renderArea.offset = { 0, 0 };
		RenderPassInfo.renderArea.extent = m_VkSwapChainExtent;

		VkClearValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		RenderPassInfo.clearValueCount = 1;
		RenderPassInfo.pClearValues = &ClearColor;

		vkCmdBeginRenderPass(m_VkCommandBuffers[i], &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_VkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);

		vkCmdDraw(m_VkCommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_VkCommandBuffers[i]);

		if (vkEndCommandBuffer(m_VkCommandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to record command buffer!");
	}
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createSyncObjects()
{
	m_VkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_VkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_VkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo SemaphoreInfo = {};
	SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo FenceInfo = {};
	FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_VkDevice, &SemaphoreInfo, nullptr, &m_VkImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_VkDevice, &SemaphoreInfo, nullptr, &m_VkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_VkDevice, &FenceInfo, nullptr, &m_VkInFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

//******************************************************************************************
//FUNCTION:
VkShaderModule CHelloTriangleApplication::__createShaderModule(const std::vector<char>& vCode)
{
	VkShaderModuleCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = vCode.size();
	CreateInfo.pCode = reinterpret_cast<const uint32_t*>(vCode.data());

	VkShaderModule ShaderModule;
	if (vkCreateShaderModule(m_VkDevice, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return ShaderModule;
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__mainLoop()
{
	while (!glfwWindowShouldClose(m_pGLFWWindow))
	{
		glfwPollEvents();
		__drawFrame();
	}

	vkDeviceWaitIdle(m_VkDevice);
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__cleanup()
{
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_VkDevice, m_VkRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_VkDevice, m_VkImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_VkDevice, m_VkInFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_VkDevice, m_VkCommandPool, nullptr);

	for (auto Framebuffer : m_VkSwapChainFramebuffers) vkDestroyFramebuffer(m_VkDevice, Framebuffer, nullptr);

	vkDestroyPipeline(m_VkDevice, m_VkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_VkDevice, m_VkPipelineLayout, nullptr);
	vkDestroyRenderPass(m_VkDevice, m_VkRenderPass, nullptr);

	for (auto ImageView : m_VkSwapChainImageViews) vkDestroyImageView(m_VkDevice, ImageView, nullptr);

	vkDestroySwapchainKHR(m_VkDevice, m_VkSwapChain, nullptr);
	vkDestroyDevice(m_VkDevice, nullptr);
	vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr);

	if (m_EnableValidationLayers) __destroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugCallback, nullptr);

	vkDestroyInstance(m_VkInstance, nullptr);

	glfwDestroyWindow(m_pGLFWWindow);
	glfwTerminate();
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createVulkanInstance()
{
	if (m_EnableValidationLayers && !__checkValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available!");

	VkApplicationInfo AppInfo = {};
	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pApplicationName = "Hello Triangle";
	AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.pEngineName = "No Engine";
	AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	CreateInfo.pApplicationInfo = &AppInfo;

	auto Extensions = __getRequiredExtensions();
	CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
	CreateInfo.ppEnabledExtensionNames = Extensions.data();

	if (m_EnableValidationLayers)
	{
		CreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		CreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else
	{
		CreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&CreateInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
		throw std::runtime_error("failed to create instance!");
}

//******************************************************************************************
//FUNCTION:
bool CHelloTriangleApplication::__checkValidationLayerSupport() const
{
	uint32_t LayerCount;
	vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

	std::vector<VkLayerProperties> AvaliableLayers(LayerCount);
	vkEnumerateInstanceLayerProperties(&LayerCount, AvaliableLayers.data());

	for (auto LayerName : VALIDATION_LAYERS)
	{
		bool LayerFound = false;

		for (const auto& LayerProperties : AvaliableLayers)
			if (strcmp(LayerName, LayerProperties.layerName) == 0) { LayerFound = true; break; }

		if (!LayerFound) return false;
	}

	return true;
}

//******************************************************************************************
//FUNCTION:
bool CHelloTriangleApplication::__checkDeviceExtensionSupport(VkPhysicalDevice vDevice) const
{
	uint32_t ExtensionCount;
	vkEnumerateDeviceExtensionProperties(vDevice, nullptr, &ExtensionCount, nullptr);

	std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
	vkEnumerateDeviceExtensionProperties(vDevice, nullptr, &ExtensionCount, AvailableExtensions.data());

	std::set<std::string> RequiredExtensions(DEVICE_EXTNESIONS.begin(), DEVICE_EXTNESIONS.end());

	for (const auto& Extension : AvailableExtensions)
		RequiredExtensions.erase(Extension.extensionName);

	return RequiredExtensions.empty();
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__setupDebugCallback()
{
	if (!m_EnableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	CreateInfo.pfnUserCallback = __debugCallback;
	CreateInfo.pUserData = nullptr; // Optional

	if (__createDebugUtilsMessengerEXT(m_VkInstance, &CreateInfo, nullptr, &m_VkDebugCallback) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug callback!");
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__createSurface()
{
	if (glfwCreateWindowSurface(m_VkInstance, m_pGLFWWindow, nullptr, &m_VkSurface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
}

//******************************************************************************************
//FUNCTION:
bool CHelloTriangleApplication::__isDeviceSuitable(VkPhysicalDevice vDevice) const
{
	SQueueFamilyIndices Indices = __findQueueFamilies(vDevice);

	bool ExtensionsSupported = __checkDeviceExtensionSupport(vDevice);

	bool SwapChainAdequate = false;
	if (ExtensionsSupported)
	{
		SSwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(vDevice);
		SwapChainAdequate = !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();
	}

	return Indices.isComplete() && ExtensionsSupported && SwapChainAdequate;
}

//******************************************************************************************
//FUNCTION:
std::vector<const char*> CHelloTriangleApplication::__getRequiredExtensions() const
{
	uint32_t GLFWExtensionCount = 0;
	const char** pGLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);

	std::vector<const char*> Extensions(pGLFWExtensions, pGLFWExtensions + GLFWExtensionCount);

	if (m_EnableValidationLayers)
		Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return Extensions;
}

//******************************************************************************************
//FUNCTION:
SQueueFamilyIndices CHelloTriangleApplication::__findQueueFamilies(VkPhysicalDevice vDevice) const
{
	SQueueFamilyIndices QueueFamilyIndices;

	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, QueueFamilies.data());

	int Index = 0;
	for (const auto& QueueFamily : QueueFamilies)
	{
		if ((QueueFamily.queueCount > 0) && (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) QueueFamilyIndices.GraphicsFamily = Index;

		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vDevice, Index, m_VkSurface, &PresentSupport);

		if (QueueFamily.queueCount > 0 && PresentSupport) QueueFamilyIndices.PresentFamily = Index;

		if (QueueFamilyIndices.isComplete()) break;

		Index++;
	}

	return QueueFamilyIndices;
}

//******************************************************************************************
//FUNCTION:
SSwapChainSupportDetails CHelloTriangleApplication::__querySwapChainSupport(VkPhysicalDevice vDevice) const
{
	SSwapChainSupportDetails Details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vDevice, m_VkSurface, &Details.Capabilities);

	uint32_t FormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vDevice, m_VkSurface, &FormatCount, nullptr);

	if (FormatCount != 0)
	{
		Details.Formats.resize(FormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vDevice, m_VkSurface, &FormatCount, Details.Formats.data());
	}

	uint32_t PresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vDevice, m_VkSurface, &PresentModeCount, nullptr);

	if (PresentModeCount != 0)
	{
		Details.PresentModes.resize(PresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vDevice, m_VkSurface, &PresentModeCount, Details.PresentModes.data());
	}

	return Details;
}

//******************************************************************************************
//FUNCTION:
VkSurfaceFormatKHR CHelloTriangleApplication::__chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats) const
{
	if (vAvailableFormats.size() == 1 && vAvailableFormats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& AvailableFormat : vAvailableFormats)
	{
		if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return AvailableFormat;
	}

	return vAvailableFormats[0];
}

//******************************************************************************************
//FUNCTION:
VkPresentModeKHR CHelloTriangleApplication::__chooseSwapPresentMode(const std::vector<VkPresentModeKHR> vAvailablePresentModes) const
{
	VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& AvailablePresentMode : vAvailablePresentModes)
	{
		if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return AvailablePresentMode;
		else if (AvailablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			BestMode = AvailablePresentMode;
	}

	return BestMode;
}

//******************************************************************************************
//FUNCTION:
VkExtent2D CHelloTriangleApplication::__chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities) const
{
	if (vCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return vCapabilities.currentExtent;
	}
	else
	{
		VkExtent2D ActualExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };

		ActualExtent.width = std::max(vCapabilities.minImageExtent.width, std::min(vCapabilities.maxImageExtent.width, ActualExtent.width));
		ActualExtent.height = std::max(vCapabilities.minImageExtent.height, std::min(vCapabilities.maxImageExtent.height, ActualExtent.height));

		return ActualExtent;
	}
}

//******************************************************************************************
//FUNCTION:
VkResult CHelloTriangleApplication::__createDebugUtilsMessengerEXT(VkInstance vInstance, const VkDebugUtilsMessengerCreateInfoEXT* vCreateInfo, const VkAllocationCallbacks* vAllocator, VkDebugUtilsMessengerEXT* vCallback)
{
	auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkCreateDebugUtilsMessengerEXT");

	if (nullptr != Func)
		return Func(vInstance, vCreateInfo, vAllocator, vCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

//******************************************************************************************
//FUNCTION:
void CHelloTriangleApplication::__destroyDebugUtilsMessengerEXT(VkInstance vInstance, VkDebugUtilsMessengerEXT vCallback, const VkAllocationCallbacks* vAllocator)
{
	auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkDestroyDebugUtilsMessengerEXT");
	if (Func) Func(vInstance, vCallback, vAllocator);
}