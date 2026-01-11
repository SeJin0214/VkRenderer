#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class VkUtil
{
public:
	static const char* ResultToString(VkResult result);
	static void ExitIfFailed(VkResult result, const char* what);
	static void ExitIfFalse(bool condition, const char* what);
	static const std::vector<const char*>& GetValidationLayers();
	static bool CheckValidationLayerSupport();

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pMessenger);

	static void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT messenger,
		const VkAllocationCallbacks* pAllocator);

	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& ci);
	static VkDebugUtilsMessengerEXT SetupDebugMessenger(VkInstance instance);

	static std::vector<char> ReadFile(const char* filename);

private:
	static const std::vector<const char*> kValidationLayers;


	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userData);

};