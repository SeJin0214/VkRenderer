
#include "VkUtil.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <GLFW/glfw3.h>

const std::vector<const char*> VkUtil::kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*>& VkUtil::GetValidationLayers()
{
    return kValidationLayers;
}

// -------------------- Layer support check --------------------
bool VkUtil::CheckValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available.data());

    for (const char* want : kValidationLayers) 
    {
        bool found = false;
        for (const auto& lp : available) 
        {
            if (std::strcmp(want, lp.layerName) == 0) 
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
			return false;
        }
    }
    return true;
}

// vkCreateDebugUtilsMessengerEXT는 확장 함수라서 GetProcAddr로 가져와야 함
VkResult VkUtil::CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (fn == nullptr)
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return fn(instance, pCreateInfo, pAllocator, pMessenger);
}

void VkUtil::DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");

    if (fn != nullptr)
    {
        fn(instance, messenger, pAllocator);
    }
}

VkDebugUtilsMessengerEXT VkUtil::SetupDebugMessenger(VkInstance instance) 
{
#ifdef NDEBUG
    return VK_NULL_HANDLE;
#endif

    VkDebugUtilsMessengerCreateInfoEXT ci{};
    PopulateDebugMessengerCreateInfo(ci);

    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    VkResult result = CreateDebugUtilsMessengerEXT(instance, &ci, nullptr, &messenger);
    ExitIfFailed(result,"vkCreateDebugUtilsMessengerEXT");
    return messenger;
}

void VkUtil::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& ci)
{
    ci = {};
    ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = DebugCallback;
    ci.pUserData = nullptr;
}

// -------------------- Debug callback --------------------
VKAPI_ATTR VkBool32 VKAPI_CALL VkUtil::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData)
{
    // 필요하면 severity로 필터링 가능
    const char* msg = data->pMessage ? data->pMessage : "";
    if (std::strstr(msg, "forced disabled because name matches filter of env var 'VK_LOADER_LAYERS_DISABLE'")) {
        return VK_FALSE;
    }

    std::cerr << "[Validation] " << data->pMessage << "\n";
    return VK_FALSE;
}

const char* VkUtil::ResultToString(VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";

    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
        return "VK_ERROR_UNKNOWN";

    default:
        return "VK_RESULT_<unknown>";
    }
}

void VkUtil::ExitIfFalse(bool condition, const char* what)
{
    if (condition)
    {
        return;
    }
    std::cerr << what << std::endl;
    std::exit(EXIT_FAILURE);
}


void VkUtil::ExitIfFailed(VkResult result, const char* what)
{
    if (result == VK_SUCCESS)
    {
        return;
	}
	std::cerr << what << " : " << ResultToString(result) << std::endl;
	std::exit(EXIT_FAILURE);
}


std::vector<char> VkUtil::ReadFile(const char* filename)
{
    uintmax_t fileSize = std::filesystem::file_size(filename);
    std::vector<char> buffer(fileSize);

    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.read(buffer.data(), fileSize);
		file.close();
	}
    else
    {
		std::cerr << "Failed to open file: " << filename << std::endl;
    }
    return buffer;
}

