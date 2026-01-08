#include "Renderer.h"
#include "VkUtil.h"
#include <vector>
#include <iostream>
#include <cassert>

Renderer::Renderer()
{
    createWindow();
    createInstance();
	mDebugMessenger = VkUtil::SetupDebugMessenger(mInstance);
    createSurface();
    pickPhysicalDevice();
}

void Renderer::Run()
{
    mainLoop();
    cleanup();
}

void Renderer::createWindow()
{
    glfwInit(); // 기본 init이겠고
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

}

void Renderer::createInstance()
{

#ifndef NDEBUG
    assert(VkUtil::CheckValidationLayerSupport());
#endif
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

    appInfo.pEngineName = "Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;


    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    createInfo.pNext = nullptr;

#ifndef NDEBUG
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    // 디버그 쓸 거면 이것도 추가
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCi{};

    const std::vector<const char*>& kValidationLayers = VkUtil::GetValidationLayers();
    createInfo.enabledLayerCount = (uint32_t)kValidationLayers.size();
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

    // instance 생성 “순간”부터 메시지 받고 싶으면 pNext로 연결
    VkUtil::PopulateDebugMessengerCreateInfo(debugCi);
    createInfo.pNext = &debugCi;
#endif
    VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
	VkUtil::ExitIfFailed(result, "failed to create instance!");
}

void Renderer::createSurface()
{
    VkResult result = glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface);
    VkUtil::ExitIfFailed(result, "glfwCreateWindowSurface");
}

void Renderer::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
		VkUtil::ExitIfFalse(false, "failed to find GPUs with Vulkan support!");
    }

	std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
    for (const auto& device : devices)
    {
        // 그래픽 작업을 할 수 있고 출력할 수 있는 지 확인
        std::optional<uint32_t> graphicsFamilyIndex;
        std::optional<uint32_t> presentFamilyIndex;
		FindQueueFamilies(device, graphicsFamilyIndex, presentFamilyIndex);
        if (!graphicsFamilyIndex.has_value() || !presentFamilyIndex.has_value())
        {
            continue;
		}

        VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        // 외장 GPU를 사용하자.
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            selectedDevice = device;
			mGraphicsFamilyIndex = graphicsFamilyIndex;
			mPresentFamilyIndex = presentFamilyIndex;
            break;
	    }

    }

    if (selectedDevice == VK_NULL_HANDLE)
    {
        VkUtil::ExitIfFalse(false, "No suitable GPU");
    }

	mPhysicalDevice = selectedDevice;
}


void Renderer::FindQueueFamilies(VkPhysicalDevice device, std::optional<uint32_t>& outGraphicsFamilyIndex, std::optional<uint32_t>& outPresentFamilyIndex) const
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

    for (uint32_t i = 0; i < count; ++i) 
    {
        if (props[i].queueCount > 0 && (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) 
        {
            outGraphicsFamilyIndex = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);
        if (props[i].queueCount > 0 && presentSupport == VK_TRUE) 
        {
            outPresentFamilyIndex = i;
        }

        if (outGraphicsFamilyIndex.has_value() && outPresentFamilyIndex.has_value())
        {
            break;
        }
    }
}



void Renderer::mainLoop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
    }
}

void Renderer::cleanup()
{


    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	VkUtil::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	vkDestroyInstance(mInstance, nullptr);
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

