#pragma once
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH	800
#define WINDOW_HEIGHT	600

class CHelloTriangleApplication
{
public:
	void run();

private:
	GLFWwindow* m_pGLFWWindow = nullptr;
	VkInstance m_VkInstance;

	void __init();
	void __mainLoop();
	void __cleanup();

	void __initWindow();
	void __initVulkan();
	void __createVulkanInstance();
};