#include "Renderer.h"
#include "VkUtil.h"
#include <iostream>
#include <cassert>
#include <unordered_set>

#define LOG(msg) std::cout << msg;
#define LOG_ENDLINE(msg) std::cout << msg << std::endl;


Renderer::Renderer()
	:mCurrentFrame(0)
{
    createWindow();
    createInstance();
	mDebugMessenger = VkUtil::SetupDebugMessenger(mInstance);
    createSurface();

    uint32_t graphicsFamilyIndex;
    uint32_t presentFamilyIndex;
    pickPhysicalDevice(graphicsFamilyIndex, presentFamilyIndex);
	createLogicalDevice(graphicsFamilyIndex, presentFamilyIndex);
	createSwapchain(graphicsFamilyIndex, presentFamilyIndex);
    createRenderPass();
	createFramebuffers();
	createGraphicsPipeline();
    createCommandPool(graphicsFamilyIndex);
	createCommandBuffers();
    createSyncObjects();
}

void Renderer::Run()
{
    mainLoop();
    cleanup();
}

void Renderer::createWindow()
{
    glfwInit();
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

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCi{};

    const std::vector<const char*>& kValidationLayers = VkUtil::GetValidationLayers();
    createInfo.enabledLayerCount = (uint32_t)kValidationLayers.size();
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

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

void Renderer::pickPhysicalDevice(uint32_t& outGraphicsFamilyIndex, uint32_t& outPresentFamilyIndex)
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
        // 그래픽 지원 하는지, 표현 되는지 확인
        uint32_t graphicsFamilyIndex;
        uint32_t presentFamilyIndex;
        if (findQueueFamilies(device, graphicsFamilyIndex, presentFamilyIndex) == false)
        {
            continue;
		}

        VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        // GPU 고르기
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            selectedDevice = device;
            outGraphicsFamilyIndex = graphicsFamilyIndex;
            outPresentFamilyIndex = presentFamilyIndex;
            break;
	    }
    }

    if (selectedDevice == VK_NULL_HANDLE)
    {
        VkUtil::ExitIfFalse(false, "No suitable GPU");
    }

	mPhysicalDevice = selectedDevice;
}


bool Renderer::findQueueFamilies(VkPhysicalDevice device, uint32_t& outGraphicsFamilyIndex, uint32_t& outPresentFamilyIndex) const
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

    outGraphicsFamilyIndex = -1;
    outPresentFamilyIndex = -1;
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

        if (outGraphicsFamilyIndex != -1 && outPresentFamilyIndex != -1)
        {
            return true;
        }
    }
    return false;
}

void Renderer::createLogicalDevice(const uint32_t graphicsFamilyIndex, const uint32_t presentFamilyIndex)
{
    std::unordered_set<uint32_t> uniqueQueueFamilies;
	uniqueQueueFamilies.insert(graphicsFamilyIndex);
	uniqueQueueFamilies.insert(presentFamilyIndex);

    std::vector<VkDeviceQueueCreateInfo> queueCIs;
    queueCIs.reserve(uniqueQueueFamilies.size());

    float priority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) 
    {
        VkDeviceQueueCreateInfo queueCI{};
        queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCI.queueFamilyIndex = queueFamily;
        queueCI.queueCount = 1;
        queueCI.pQueuePriorities = &priority;
        queueCIs.push_back(queueCI);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCIs.size());
	createInfo.pQueueCreateInfos = queueCIs.data();
	createInfo.pEnabledFeatures = &deviceFeatures;


    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    VkResult result = vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mLogicalDevice);
	VkUtil::ExitIfFailed(result, "failed to create logical device!");

	mGraphicsQueue = VK_NULL_HANDLE;
	mPresentQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(mLogicalDevice, graphicsFamilyIndex, 0, &mGraphicsQueue);
	vkGetDeviceQueue(mLogicalDevice, presentFamilyIndex, 0, &mPresentQueue);

    if (mGraphicsQueue == VK_NULL_HANDLE || mPresentQueue == VK_NULL_HANDLE)
    {
        VkUtil::ExitIfFalse(false, "failed to get queue handles!");
    }
}

void Renderer::createSwapchain(const uint32_t graphicsFamilyIndex, const uint32_t presentFamilyIndex)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &surfaceCapabilities);
    mSwapchainExtent = surfaceCapabilities.currentExtent;

    VkSurfaceFormatKHR bestFormat = pickBestFormat();
	VkPresentModeKHR bestPresentMode = pickBestPresentMode();

	VkSwapchainCreateInfoKHR swapchainCI{};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.surface = mSurface;
    swapchainCI.imageExtent = surfaceCapabilities.currentExtent;
	swapchainCI.minImageCount = surfaceCapabilities.minImageCount + 1;
    swapchainCI.preTransform = surfaceCapabilities.currentTransform;

    LOG("minImageCount : ");
	LOG_ENDLINE(swapchainCI.minImageCount);



	uint32_t queueFamilyIndices[] = { graphicsFamilyIndex, presentFamilyIndex };

    if (graphicsFamilyIndex != presentFamilyIndex)
    {
        swapchainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCI.queueFamilyIndexCount = 2;
        swapchainCI.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

	swapchainCI.imageFormat = bestFormat.format;
    swapchainCI.imageColorSpace = bestFormat.colorSpace;
    swapchainCI.presentMode = bestPresentMode;  // VSync

    swapchainCI.imageArrayLayers = 1;                    // VR이면 1
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCI.clipped = VK_TRUE;

    // 리사이즈 필요한 경우
	swapchainCI.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(mLogicalDevice, &swapchainCI, nullptr, &mSwapchain);
	VkUtil::ExitIfFailed(result, "fail vkCreateSwapchainKHR");

	createImageViews(bestFormat);
}

VkSurfaceFormatKHR Renderer::pickBestFormat() const
{
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, formats.data());
    for (const auto& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            return format;
        }
    }

    for (const auto& format : formats)
    {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB)
        {
            return format;
        }
    }
    return formats[0];
}

VkPresentModeKHR Renderer::pickBestPresentMode() const
{
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &modeCount, nullptr);

    if (modeCount == 0)
    {
        LOG_ENDLINE("No present modes found! default VK_PRESENT_MODE_IMMEDIATE_KHR");
	}

    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &modeCount, modes.data());

    for (const auto& mode : modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return mode;
		}
    }

    for (const auto& mode : modes)
    {
        if (mode == VK_PRESENT_MODE_FIFO_KHR)
        {
			return mode;
        }
    }

    for (const auto& mode : modes)
    {
        if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
        {
            return mode;
        }
    }

	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}


void Renderer::createImageViews(VkSurfaceFormatKHR format)
{
    // image 
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &imageCount, nullptr);
	mImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &imageCount, mImages.data());

    for (uint32_t i = 0; i < imageCount; ++i)
    {
        VkImageViewCreateInfo ivCI{};
        ivCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivCI.image = mImages[i];
        ivCI.format = format.format;

        ivCI.viewType = VK_IMAGE_VIEW_TYPE_2D;

		// swizzle
        ivCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Mipmap / Layer
        ivCI.subresourceRange.baseMipLevel = 0;
        ivCI.subresourceRange.levelCount = 1;
        ivCI.subresourceRange.baseArrayLayer = 0;
        ivCI.subresourceRange.layerCount = 1;

        VkImageView imageView;
        VkResult result = vkCreateImageView(mLogicalDevice, &ivCI, nullptr, &imageView);
		VkUtil::ExitIfFailed(result, "fail vkCreateImageView");
		mImageViews.push_back(imageView);
    }
}

void Renderer::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = pickBestFormat().format;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpass;
    renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &colorAttachment;

	VkResult result = vkCreateRenderPass(mLogicalDevice, &renderPassCI, nullptr, &mRenderPass);
    VkUtil::ExitIfFailed(result, "fail vkCreateRenderPass");
}

void Renderer::createFramebuffers()
{
    mFramebuffers.resize(mImages.size());
    for (uint32_t i = 0; i < mImages.size(); ++i)
    {
        VkImageView attachments[] = { mImageViews[i] };

		VkFramebufferCreateInfo framebufferCI{};
        framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCI.renderPass = mRenderPass;
        framebufferCI.attachmentCount = 1;
		framebufferCI.pAttachments = attachments;
		framebufferCI.width = mSwapchainExtent.width;
		framebufferCI.height = mSwapchainExtent.height;
		framebufferCI.layers = 1;

        VkResult result = vkCreateFramebuffer(mLogicalDevice, &framebufferCI, nullptr, &mFramebuffers[i]);
		VkUtil::ExitIfFailed(result, "fail vkCreateFramebuffer");
    }
    LOG("Framebuffers created: ");
    LOG_ENDLINE(mFramebuffers.size());
}

void Renderer::createGraphicsPipeline()
{
	std::vector<char> vsCode = VkUtil::ReadFile("vert.spv"); // 커맨드라인 인자로 받기? 
    VkShaderModule vertShaderModule = createShaderModule(vsCode);
    std::vector<char> fsCode = VkUtil::ReadFile("frag.spv");
    VkShaderModule fragShaderModule = createShaderModule(fsCode);

    LOG_ENDLINE("Shader modules created.");


    VkPipelineShaderStageCreateInfo vertexShaderCI{};
	vertexShaderCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCI.module = vertShaderModule;
    vertexShaderCI.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderCI{};
	fragmentShaderCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCI.module = fragShaderModule;
    fragmentShaderCI.pName = "main";

    VkPipelineShaderStageCreateInfo shaders[] = { vertexShaderCI, fragmentShaderCI };

    VkPipelineVertexInputStateCreateInfo vertexInputCI{};
    vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
	inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCI.primitiveRestartEnable = VK_FALSE;


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(mSwapchainExtent.width);
    viewport.height = static_cast<float>(mSwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = mSwapchainExtent;

    VkViewport viewports[] = { viewport };
    VkRect2D scissors[] = { scissor };

    VkPipelineViewportStateCreateInfo viewportCI{};
    viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCI.viewportCount = 1;
    viewportCI.scissorCount = 1;
    viewportCI.pViewports = viewports;
    viewportCI.pScissors = scissors;

    VkPipelineRasterizationStateCreateInfo rasterizationCI{};
	rasterizationCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCI.depthClampEnable = VK_FALSE;
    rasterizationCI.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCI.lineWidth = 1.0f;
    rasterizationCI.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCI.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampleCI{};
	multisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCI.sampleShadingEnable = VK_FALSE;
    multisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendCI{};
	colorBlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCI.logicOpEnable = VK_FALSE;
    colorBlendCI.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCI.attachmentCount = 1;
    colorBlendCI.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo layoutCI{};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.setLayoutCount = 0;         // ��ũ���ͼ� ����
    layoutCI.pushConstantRangeCount = 0; // Ǫ���ܽ�źƮ ����

    VkResult result = vkCreatePipelineLayout(mLogicalDevice, &layoutCI, nullptr, &mPipelineLayout);
    VkUtil::ExitIfFailed(result, "fail layout");
	
    VkGraphicsPipelineCreateInfo pipelineCI{};

	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount = 2;
    pipelineCI.pStages = shaders;
    pipelineCI.pVertexInputState = &vertexInputCI;
    pipelineCI.pInputAssemblyState = &inputAssemblyCI;
    pipelineCI.pViewportState = &viewportCI;
    pipelineCI.pRasterizationState = &rasterizationCI;
    pipelineCI.pMultisampleState = &multisampleCI;
    pipelineCI.pColorBlendState = &colorBlendCI;
    pipelineCI.layout = mPipelineLayout;
	pipelineCI.renderPass = mRenderPass;
	pipelineCI.subpass = 0;

    VkResult r = vkCreateGraphicsPipelines(mLogicalDevice, nullptr, 1, &pipelineCI, nullptr, &mGraphicsPipeline);
	VkUtil::ExitIfFailed(r, "fail vkCreateGraphicsPipelines");


	LOG_ENDLINE("Graphics pipeline created.");
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(mLogicalDevice, &createInfo, nullptr, &shaderModule);
	VkUtil::ExitIfFailed(result, "fail vkCreateShaderModule");
    return shaderModule;
}

void Renderer::createCommandPool(const uint32_t graphicsFamilyIndex)
{
	VkCommandPoolCreateInfo poolCI{};
	poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCI.queueFamilyIndex = graphicsFamilyIndex;
    poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkResult result = vkCreateCommandPool(mLogicalDevice, &poolCI, nullptr, &mCommandPool);
    VkUtil::ExitIfFailed(result, "fail createCommandPool");

}

void Renderer::createCommandBuffers()
{
	mCommandBuffers.resize(mImages.size());
    
    VkCommandBufferAllocateInfo allocCI{};
    allocCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCI.commandPool = mCommandPool;
    allocCI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCI.commandBufferCount = mCommandBuffers.size();

    VkResult result = vkAllocateCommandBuffers(mLogicalDevice, &allocCI, mCommandBuffers.data());
    VkUtil::ExitIfFailed(result, "fail createCommandBuffers");

}

void Renderer::createSyncObjects() 
{
    mImagesInFlight.assign(mFramebuffers.size(), VK_NULL_HANDLE);
    mFences.resize(mFramebuffers.size());
	imageAvailableSemaphores.resize(mFramebuffers.size());
	renderFinishedSemaphores.resize(mFramebuffers.size());

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < mFences.size(); i++)
    {
        vkCreateFence(mLogicalDevice, &fenceInfo, nullptr, &mFences[i]);
		vkCreateSemaphore(mLogicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
		vkCreateSemaphore(mLogicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
    }
}

void Renderer::mainLoop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
        drawFrame();
    }
}

void Renderer::drawFrame()
{
	LOG("Drawing frame start");
    vkWaitForFences(mLogicalDevice, 1, &mFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(
        mLogicalDevice,
        mSwapchain,
        UINT64_MAX,
        imageAvailableSemaphores[mCurrentFrame],
        VK_NULL_HANDLE,
        &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        return;
    }
    VkUtil::ExitIfFailed(acquireResult, "vkAcquireNextImageKHR");
	
    if (mImagesInFlight[imageIndex] != VK_NULL_HANDLE) 
    {
        vkWaitForFences(mLogicalDevice, 1, &mImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    vkResetFences(mLogicalDevice, 1, &mFences[mCurrentFrame]);
    mImagesInFlight[imageIndex] = mFences[mCurrentFrame];
    VkResult reulst = vkResetCommandBuffer(mCommandBuffers[imageIndex], 0);
	VkUtil::ExitIfFailed(reulst, "fail vkResetCommandBuffer");

    recordCommandBuffer(mCommandBuffers[imageIndex], imageIndex);

    VkSemaphore signalSem[] = { renderFinishedSemaphores[imageIndex] };
    VkSemaphore waitSem[] = { imageAvailableSemaphores[mCurrentFrame] };
    VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSem;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSem;
    submitInfo.pWaitDstStageMask = waitStage;

    VkResult result1 = vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mFences[mCurrentFrame]);
    VkUtil::ExitIfFailed(result1, "fail vkQueueSubmit");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSem;

    VkResult rP = vkQueuePresentKHR(mPresentQueue, &presentInfo);
    if (rP == VK_ERROR_OUT_OF_DATE_KHR || rP == VK_SUBOPTIMAL_KHR)
    {
        return;
    }
    VkUtil::ExitIfFailed(rP, "vkQueuePresentKHR");

	mCurrentFrame = (mCurrentFrame + 1) % mFramebuffers.size();
}

void Renderer::recordCommandBuffer(VkCommandBuffer currentBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkResult result = vkBeginCommandBuffer(currentBuffer, &beginInfo);
	VkUtil::ExitIfFailed(result, "fail vkBeginCommandBuffer");

    LOG("Recording command buffer for image index: ");
	LOG_ENDLINE(imageIndex);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mRenderPass;
    renderPassInfo.framebuffer = mFramebuffers[imageIndex];
    renderPassInfo.renderArea.extent = mSwapchainExtent;
    renderPassInfo.renderArea.offset = { 0, 0 };

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(currentBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(currentBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

    vkCmdDraw(currentBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(currentBuffer);
    VkResult endResult = vkEndCommandBuffer(currentBuffer);
    VkUtil::ExitIfFailed(endResult, "vkEndCommandBuffer");
}

void Renderer::cleanup()
{
    for (uint32_t i = 0; i < mFences.size(); i++)
    {
        vkDestroyFence(mLogicalDevice, mFences[i], nullptr);
		vkDestroySemaphore(mLogicalDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(mLogicalDevice, renderFinishedSemaphores[i], nullptr);
    }
	vkFreeCommandBuffers(mLogicalDevice, mCommandPool, static_cast<uint32_t>(mCommandBuffers.size()), mCommandBuffers.data());
	vkDestroyCommandPool(mLogicalDevice, mCommandPool, nullptr);
	vkDestroyPipeline(mLogicalDevice, mGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mLogicalDevice, mPipelineLayout, nullptr);
    for (uint32_t i = 0; i < mFramebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(mLogicalDevice, mFramebuffers[i], nullptr);
    }
	vkDestroyRenderPass(mLogicalDevice, mRenderPass, nullptr);
    for (uint32_t i = 0; i < mImageViews.size(); ++i)
    {
        vkDestroyImageView(mLogicalDevice, mImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(mLogicalDevice, mSwapchain, nullptr);
    vkDestroyDevice(mLogicalDevice, nullptr);
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	VkUtil::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	vkDestroyInstance(mInstance, nullptr);
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

