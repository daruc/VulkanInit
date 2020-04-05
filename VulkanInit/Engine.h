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
	std::vector<VkImageView> m_vkSwapchainImageViews;
	VkFormat m_vkSwapchainImageFormat;
	VkExtent2D m_vkSwapchainExtent;
	std::vector<const char*> m_deviceExtensions;
	VkRenderPass m_vkRenderPass;
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkPipeline;

	void initVkInstance();
	void createVkSurface();
	void pickPhysicalDevice();
	void createDevice();
	void createSwapChain();
	void createSwapChainImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	VkShaderModule loadShader(const char* fileName);
	QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice physicalDevice);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	bool checkSwapchainSupport(VkPhysicalDevice physicalDevice);
	bool checkQueueFamiliesSupport(VkPhysicalDevice physicalDevice);
	
public:
	void init(struct SDL_Window* sdlWindow);
	void update();
	void render();
	void cleanUp();
};

