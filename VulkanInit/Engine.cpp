#include "Engine.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <vector>
#include <set>
#include "glm/common.hpp"
#include <fstream>

void Engine::initVkInstance()
{
	VkApplicationInfo vkApplicationInfo = {};
	vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkApplicationInfo.pApplicationName = "Vulkan Init";
	vkApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vkApplicationInfo.pEngineName = "Engine";
	vkApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vkApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	unsigned int extensionCount;
	SDL_Vulkan_GetInstanceExtensions(m_sdlWindow, &extensionCount, nullptr);
	std::vector<const char*> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(m_sdlWindow, &extensionCount, extensions.data());

	VkInstanceCreateInfo vkInstanceCreateInfo = {};
	vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
	vkInstanceCreateInfo.enabledExtensionCount = extensionCount;
	vkInstanceCreateInfo.ppEnabledExtensionNames = extensions.data();

#ifdef _DEBUG
	std::vector<const char*> validationLayers;
	validationLayers.push_back("VK_LAYER_KHRONOS_validation");

	vkInstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	vkInstanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#endif

	VkResult result = vkCreateInstance(&vkInstanceCreateInfo, nullptr, &m_vkInstance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create VkInstance.");
	}
}

void Engine::createVkSurface()
{
	SDL_bool result = SDL_Vulkan_CreateSurface(m_sdlWindow, m_vkInstance, &m_vkSurface);

	if (result == SDL_FALSE)
	{
		throw std::runtime_error("Failed to create VkSurfaceKHR.");
	}
}

void Engine::pickPhysicalDevice()
{
	m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	unsigned int deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("None physical device is available.");
	}

	std::vector<VkPhysicalDevice> availableDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, availableDevices.data());

	m_vkPhysicalDevice = VK_NULL_HANDLE;
	for (VkPhysicalDevice availableDevice : availableDevices)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(availableDevice, &properties);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			checkDeviceExtensionSupport(availableDevice) &&
			checkSwapchainSupport(availableDevice) &&
			checkQueueFamiliesSupport(availableDevice))
		{
			m_vkPhysicalDevice = availableDevice;
			break;
		}
	}

	if (m_vkPhysicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("None discrete GPU is available.");
	}
}

void Engine::createDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(2);

	QueueFamilyIndices queueFamilyIndices = findQueueFamilyIndices(m_vkPhysicalDevice);
	const float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo& graphicsQueueCreateInfo = queueCreateInfos[0];
	graphicsQueueCreateInfo = {};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.queueFamilyIndex = *queueFamilyIndices.graphics;
	graphicsQueueCreateInfo.queueCount = 1;
	graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;

	VkDeviceQueueCreateInfo& presentationQueueCreateInfo = queueCreateInfos[1];
	presentationQueueCreateInfo = {};
	presentationQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	presentationQueueCreateInfo.queueFamilyIndex = *queueFamilyIndices.presentation;
	presentationQueueCreateInfo.queueCount = 1;
	presentationQueueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	VkResult result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create device.");
	}

	vkGetDeviceQueue(m_vkDevice, *queueFamilyIndices.graphics, 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, *queueFamilyIndices.presentation, 0, &m_vkPresentationQueue);
}

void Engine::createSwapChain()
{
	SwapChainSupportDetails supportDetails = querySwapChainSupport(m_vkPhysicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(supportDetails.presentModes);
	VkExtent2D extent = chooseSwapExtent(supportDetails.capabilities);

	uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
	
	if (supportDetails.capabilities.maxImageCount > 0 &&
		imageCount > supportDetails.capabilities.maxImageCount)
	{
		imageCount = supportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = m_vkSurface;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices queueFamilyIndices = findQueueFamilyIndices(m_vkPhysicalDevice);
	std::vector<uint32_t> indices;
	indices.push_back(queueFamilyIndices.graphics.value());
	indices.push_back(queueFamilyIndices.presentation.value());

	if (queueFamilyIndices.graphics != queueFamilyIndices.presentation)
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
		swapChainCreateInfo.pQueueFamilyIndices = indices.data();
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainCreateInfo.preTransform = supportDetails.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(m_vkDevice, &swapChainCreateInfo, nullptr, &m_vkSwapchain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain.");
	}

	uint32_t finalImageCount;
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &finalImageCount, nullptr);
	m_vkSwapchainImages.resize(finalImageCount);
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &finalImageCount, m_vkSwapchainImages.data());

	m_vkSwapchainImageFormat = surfaceFormat.format;
	m_vkSwapchainExtent = extent;
}

void Engine::createSwapChainImageViews()
{
	m_vkSwapchainImageViews.resize(m_vkSwapchainImages.size());

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = m_vkSwapchainImageFormat;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	for (size_t i = 0; i < m_vkSwapchainImageViews.size(); ++i)
	{
		createInfo.image = m_vkSwapchainImages[i];

		VkResult result = vkCreateImageView(m_vkDevice, &createInfo, nullptr, &m_vkSwapchainImageViews[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain image view.");
		}
	}
}

void Engine::createGraphicsPipeline()
{
	VkShaderModule vertexShader = loadShader("vertex.spv");
	VkShaderModule fragmentShader = loadShader("fragment.spv");

	VkPipelineShaderStageCreateInfo vertexStageCreateInfo = {};
	vertexStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexStageCreateInfo.module = vertexShader;
	vertexStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentStageCreateInfo = {};
	fragmentStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentStageCreateInfo.module = fragmentShader;
	fragmentStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStageInfos[] = { vertexStageCreateInfo, fragmentStageCreateInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_vkSwapchainExtent.width);
	viewport.height = static_cast<float>(m_vkSwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.extent = m_vkSwapchainExtent;
	scissor.offset = { 0, 0 };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo = {};
	multisamplingStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingStateCreateInfo.minSampleShading = 1.0f;
	multisamplingStateCreateInfo.pSampleMask = nullptr;
	multisamplingStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	VkResult result = vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout.");
	}

	vkDestroyShaderModule(m_vkDevice, vertexShader, nullptr);
	vkDestroyShaderModule(m_vkDevice, fragmentShader, nullptr);
}

VkShaderModule Engine::loadShader(const char * fileName)
{
	std::ifstream istr(fileName, std::ios::ate | std::ios::binary);

	if (!istr.is_open())
	{
		throw std::runtime_error("Failed to open shader file.");
	}

	size_t fileSize = static_cast<size_t>(istr.tellg());
	std::vector<char> buffer(fileSize);
	istr.seekg(0);
	istr.read(buffer.data(), fileSize);
	istr.close();

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = fileSize;
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(buffer.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(m_vkDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module.");
	}

	return shaderModule;
}

QueueFamilyIndices Engine::findQueueFamilyIndices(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices queueFamilyIndices;

	unsigned int familyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, queueFamilies.data());

	unsigned int index = 0;
	for (VkQueueFamilyProperties queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (!queueFamilyIndices.graphics.has_value())
			{
				queueFamilyIndices.graphics = index;
			}
		}
		else if (!queueFamilyIndices.presentation.has_value())
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, m_vkSurface, &presentSupport);

			if (presentSupport)
			{
				queueFamilyIndices.presentation = index;
			}
		}

		if (queueFamilyIndices.graphics.has_value() && queueFamilyIndices.presentation.has_value())
		{
			return queueFamilyIndices;
		}

		++index;
	}

	throw std::runtime_error("Graphics with presentation queue family not found.");
}

SwapChainSupportDetails Engine::querySwapChainSupport(VkPhysicalDevice physicalDevice)
{
	SwapChainSupportDetails supportDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_vkSurface, &supportDetails.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_vkSurface, &formatCount, nullptr);

	if (formatCount > 0)
	{
		supportDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_vkSurface, &formatCount, supportDetails.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_vkSurface, &presentModeCount, nullptr);

	if (presentModeCount > 0)
	{
		supportDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice, m_vkSurface, &presentModeCount, supportDetails.presentModes.data());
	}

	return supportDetails;
}

VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	return formats[0];
}

VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	VkExtent2D extent;

	if (capabilities.currentExtent.width == UINT32_MAX)
	{
		int width, height;
		SDL_Vulkan_GetDrawableSize(m_sdlWindow, &width, &height);

		glm::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		glm::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	}
	else
	{
		extent = capabilities.currentExtent;
	}

	return extent;
}

bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t availableExtensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionCount, availableExtensions.data());

	std::set<std::string> unavailableExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

	for (VkExtensionProperties& available : availableExtensions)
	{
		unavailableExtensions.erase(available.extensionName);
	}

	return unavailableExtensions.empty();
}

bool Engine::checkSwapchainSupport(VkPhysicalDevice physicalDevice)
{
	SwapChainSupportDetails swapchainSupportDetails = querySwapChainSupport(physicalDevice);
	return !swapchainSupportDetails.presentModes.empty() &&
		!swapchainSupportDetails.formats.empty();
}

bool Engine::checkQueueFamiliesSupport(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices indices = findQueueFamilyIndices(physicalDevice);
	return indices.graphics.has_value() && indices.presentation.has_value();
}

void Engine::init(SDL_Window* sdlWindow)
{
	m_sdlWindow = sdlWindow;

	initVkInstance();
	createVkSurface();
	pickPhysicalDevice();
	createDevice();
	createSwapChain();
	createSwapChainImageViews();
	createGraphicsPipeline();
}

void Engine::update()
{
}

void Engine::render()
{
}

void Engine::cleanUp()
{
	vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);

	for (VkImageView swapchainImageView : m_vkSwapchainImageViews)
	{
		vkDestroyImageView(m_vkDevice, swapchainImageView, nullptr);
	}

	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
}
