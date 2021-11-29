#pragma once

#include <array>
#include <utility>
#define SDL_MAIN_HANDLED
#include <SDL_ttf.h>

#include "common.hpp"
#include "components.hpp"

#include "map_generator_system.hpp"

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class LightingSystem {

	std::shared_ptr<MapGeneratorSystem> map_generator;

public:
	// Initialize the window
	void init(std::shared_ptr<MapGeneratorSystem> map);

	void step();
};
