#include "Engine.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <vector>

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

	vkInstanceCreateInfo.enabledLayerCount = validationLayers.size();
	vkInstanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#endif

	VkResult result = vkCreateInstance(&vkInstanceCreateInfo, nullptr, &m_vkInstance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create VkInstance.");
	}
}

void Engine::pickPhysicalDevice()
{
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

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = getGraphicsQueueFamilyIndex();
	queueCreateInfo.queueCount = 1;
	const float queuePriority = 0.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;

	VkResult result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create device.");
	}

	vkGetDeviceQueue(m_vkDevice, getGraphicsQueueFamilyIndex(), 0, &m_vkGraphicsQueue);
}

unsigned int Engine::getGraphicsQueueFamilyIndex()
{
	unsigned int familyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &familyCount, queueFamilies.data());

	unsigned int index = 0;
	for (VkQueueFamilyProperties queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			return index;
		}
		++index;
	}

	throw std::runtime_error("Graphics queue family not found.");
}

void Engine::init(SDL_Window* sdlWindow)
{
	m_sdlWindow = sdlWindow;

	initVkInstance();
	pickPhysicalDevice();
	createDevice();
}

void Engine::update()
{
}

void Engine::render()
{
}

void Engine::cleanUp()
{
	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
}
