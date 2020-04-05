#pragma once

#include <vulkan.h>
#include <optional>
#include <vector>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> presentation;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Engine
{
private:
	struct SDL_Window* m_sdlWindow;
	VkInstance m_vkInstance;
	VkPhysicalDevice m_vkPhysicalDevice;
	VkDevice m_vkDevice;
	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentationQueue;
	VkSurfaceKHR m_vkSurface;
	VkSwapchainKHR m_vkSwapchain;
	std::vector<VkImage> m_vkSwapchainImages;
	VkFormat m_vkSwapchainImageFormat;
	VkExtent2D m_vkSwapchainExtent;

	void initVkInstance();
	void createVkSurface();
	void pickPhysicalDevice();
	void createDevice();
	void createSwapChain();
	QueueFamilyIndices findQueueFamilyIndices();
	SwapChainSupportDetails querySwapChainSupport();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	
public:
	void init(struct SDL_Window* sdlWindow);
	void update();
	void render();
	void cleanUp();
};

