#include "Renderer.h"
#include "VkUtil.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <cassert>



Renderer::Renderer()
{
    createWindow();
    createInstance();
	mDebugMessenger = VkUtil::SetupDebugMessenger(mInstance);
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


void Renderer::mainLoop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
    }
}

void Renderer::cleanup()
{

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

