#include "HelloTriangleApplication.h"

int main()
{
	CHelloTriangleApplication HelloTriangleApp;

	try
	{
		HelloTriangleApp.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}