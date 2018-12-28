#include "HelloTriangleApplication.h"

void CHelloTriangleApplication::run()
{
	__init();
	__mainLoop();
	__cleanup();
}

void CHelloTriangleApplication::__init()
{
	__initWindow();
	__initVulkan();
}

void CHelloTriangleApplication::__initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_pGLFWWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
}

void CHelloTriangleApplication::__initVulkan()
{
	__createVulkanInstance();
}

void CHelloTriangleApplication::__mainLoop()
{
	while (!glfwWindowShouldClose(m_pGLFWWindow)) { glfwPollEvents(); }
}

void CHelloTriangleApplication::__cleanup()
{
	vkDestroyInstance(m_VkInstance, nullptr);
	glfwDestroyWindow(m_pGLFWWindow);
	glfwTerminate();
}

void CHelloTriangleApplication::__createVulkanInstance()
{
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

	uint32_t GLFWExtensionCount = 0;
	const char** pGLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);

	CreateInfo.enabledExtensionCount = GLFWExtensionCount;
	CreateInfo.ppEnabledExtensionNames = pGLFWExtensions;
	CreateInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&CreateInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
		throw std::runtime_error("failed to create instance!");
}