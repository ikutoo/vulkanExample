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

	VkInstance m_VkInstance;
	VkDebugUtilsMessengerEXT m_VkDebugCallback;
	VkSurfaceKHR m_VkSurface;

	VkPhysicalDevice m_VkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_VkDevice;

	VkQueue m_VkGraphicsQueue;
	VkQueue m_VkPresentQueue;

	VkSwapchainKHR m_VkSwapChain;
	VkFormat m_VkSwapChainImageFormat;
	VkExtent2D m_VkSwapChainExtent;
	VkPipelineLayout m_VkPipelineLayout;
	VkRenderPass m_VkRenderPass;
	VkPipeline m_VkGraphicsPipeline;
	VkCommandPool m_VkCommandPool;

	std::vector<VkCommandBuffer> m_VkCommandBuffers;
	std::vector<VkImage> m_VkSwapChainImages;
	std::vector<VkImageView> m_VkSwapChainImageViews;
	std::vector<VkFramebuffer> m_VkSwapChainFramebuffers;
	std::vector<VkSemaphore> m_VkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_VkRenderFinishedSemaphores;
	std::vector<VkFence> m_VkInFlightFences;
	size_t m_CurrentFrame = 0;

	bool m_EnableValidationLayers = false;

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
	void __createCommandBuffers();
	void __createSyncObjects();

	VkShaderModule  __createShaderModule(const std::vector<char>& vCode);

	bool __checkValidationLayerSupport();
	bool __checkDeviceExtensionSupport(VkPhysicalDevice vDevice);
	bool __isDeviceSuitable(VkPhysicalDevice vDevice);

	std::vector<const char*> __getRequiredExtensions();
	SQueueFamilyIndices __findQueueFamilies(VkPhysicalDevice vDevice);
	SSwapChainSupportDetails __querySwapChainSupport(VkPhysicalDevice vDevice);

	VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats);
	VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR> vAvailablePresentModes);
	VkExtent2D __chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities);

	VkResult __createDebugUtilsMessengerEXT(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*);
	void __destroyDebugUtilsMessengerEXT(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*);
};