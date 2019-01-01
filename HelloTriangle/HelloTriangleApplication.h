#pragma once
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct SQueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;

	bool isComplete() { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
};

struct SSwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};

class CHelloTriangleApplication
{
public:
	void run();

private:
	GLFWwindow* m_pGLFWWindow = nullptr;

	VkInstance					m_VkInstance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT	m_VkDebugCallback = VK_NULL_HANDLE;
	VkSurfaceKHR				m_VkSurface = VK_NULL_HANDLE;
	VkPhysicalDevice			m_VkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice					m_VkDevice = VK_NULL_HANDLE;
	VkQueue						m_VkGraphicsQueue = VK_NULL_HANDLE;
	VkQueue						m_VkPresentQueue = VK_NULL_HANDLE;
	VkSwapchainKHR				m_VkSwapChain = VK_NULL_HANDLE;
	VkPipelineLayout			m_VkPipelineLayout = VK_NULL_HANDLE;
	VkRenderPass				m_VkRenderPass = VK_NULL_HANDLE;
	VkPipeline					m_VkGraphicsPipeline = VK_NULL_HANDLE;
	VkCommandPool				m_VkCommandPool = VK_NULL_HANDLE;
	VkBuffer					m_VkVertexBuffer = VK_NULL_HANDLE;
	VkFormat					m_VkSwapChainImageFormat;
	VkExtent2D					m_VkSwapChainExtent;

	std::vector<VkCommandBuffer>	m_VkCommandBuffers;
	std::vector<VkImage>			m_VkSwapChainImages;
	std::vector<VkImageView>		m_VkSwapChainImageViews;
	std::vector<VkFramebuffer>		m_VkSwapChainFramebuffers;
	std::vector<VkSemaphore>		m_VkImageAvailableSemaphores;
	std::vector<VkSemaphore>		m_VkRenderFinishedSemaphores;
	std::vector<VkFence>			m_VkInFlightFences;

	size_t	m_CurrentFrame = 0;
	bool	m_EnableValidationLayers = false;

	void __init();
	void __mainLoop();
	void __cleanup();

	void __initWindow();
	void __initVulkan();

	void __drawFrame();

	void __createVulkanInstance();
	void __setupDebugCallback();
	void __createSurface();
	void __pickPhysicalDevice();
	void __createLogicalDevice();
	void __createSwapChain();
	void __createImageViews();
	void __createRenderPass();
	void __createGraphicsPipeline();
	void __createFrameBuffers();
	void __createCommandPool();
	void __createVertexBuffer();
	void __createCommandBuffers();
	void __createSyncObjects();

	VkShaderModule __createShaderModule(const std::vector<char>& vCode);

	bool __checkValidationLayerSupport() const;
	bool __checkDeviceExtensionSupport(VkPhysicalDevice vDevice) const;
	bool __isDeviceSuitable(VkPhysicalDevice vDevice) const;

	std::vector<const char*> __getRequiredExtensions() const;
	SQueueFamilyIndices __findQueueFamilies(VkPhysicalDevice vDevice) const;
	SSwapChainSupportDetails __querySwapChainSupport(VkPhysicalDevice vDevice) const;

	VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats) const;
	VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR> vAvailablePresentModes) const;
	VkExtent2D __chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities) const;

	VkResult __createDebugUtilsMessengerEXT(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*);
	void __destroyDebugUtilsMessengerEXT(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*);
};