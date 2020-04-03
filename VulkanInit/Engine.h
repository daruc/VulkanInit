#pragma once

#include <vulkan.h>

class Engine
{
private:
	struct SDL_Window* m_sdlWindow;
	VkInstance m_vkInstance;
	VkPhysicalDevice m_vkPhysicalDevice;
	VkDevice m_vkDevice;
	VkQueue m_vkGraphicsQueue;

	void initVkInstance();
	void pickPhysicalDevice();
	void createDevice();
	unsigned int getGraphicsQueueFamilyIndex();
	
public:
	void init(struct SDL_Window* sdlWindow);
	void update();
	void render();
	void cleanUp();
};

