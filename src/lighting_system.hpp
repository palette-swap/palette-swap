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
	enum class AngleResult {
		New,
		Redundant,
		Overlap,
	};

	void spin(uvec2 player_map_pos, vec2 player_world_pos);
	void process_tile(vec2 player_world_pos, uvec2 tile);
	AngleResult try_add_angle(dvec2& angle);
	vec2 project_onto_tile(uvec2 tile, vec2 player_world_pos, double angle);

	inline int rad_to_int(double angle)
	{
		return static_cast<int>(round(angle * half_pseudo_degrees / glm::pi<double>()));
	}

	std::vector<dvec2> visited_angles;
	const int light_radius = 14;
	const double half_pseudo_degrees = 2 << 14;
	const double tol = 4.0 / half_pseudo_degrees;
};
