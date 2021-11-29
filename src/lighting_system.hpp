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

private:

	void scan_row(uvec2 origin, int dy, const vec2& player_world_pos, vec2 left_bound, vec2 right_bound);
};
