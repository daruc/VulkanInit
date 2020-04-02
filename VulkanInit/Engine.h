#pragma once

class Engine
{
private:
	struct SDL_Window* m_sdlWindow;

public:
	void init(struct SDL_Window* sdlWindow);
	void update();
	void render();
};

