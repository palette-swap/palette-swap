#pragma once
#include "common.hpp"
#include "components.hpp"

#include "map_generator_system.hpp"

#include <glm/gtx/hash.hpp>
#include <unordered_set>

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class LightingSystem {

	std::shared_ptr<MapGeneratorSystem> map_generator;

public:
	// Initialize the window
	void init(std::shared_ptr<MapGeneratorSystem> map);

	void step(float elapsed_ms);

	bool is_visible(uvec2 tile);

private:
	enum class AngleResult {
		Redundant = 0,
		Overlap = 1,
		New = 2,
	};

	void spin(uvec2 player_map_pos, vec2 player_world_pos);
	void process_tile(vec2 player_world_pos, uvec2 tile);

	// Wall case
	AngleResult try_add_angle(dvec2& angle);
	vec2 project_onto_tile(uvec2 tile, vec2 player_world_pos, double angle);
	void draw_tile(AngleResult result, const dvec2& angle, uvec2 tile, vec2 player_world_pos);

	// Non-wall case
	AngleResult check_visible(dvec2& angle);

	// Exploration / hidden monsters stuff
	void mark_as_visible(uvec2 tile);
	void update_visible();

	inline int rad_to_int(double angle)
	{
		return static_cast<int>(round(angle * half_pseudo_degrees / glm::pi<double>()));
	}

	std::vector<dvec2> visited_angles;
	std::unordered_set<uvec2> visible_tiles;
	std::unordered_map<uint8_t, uvec2> visible_rooms;
	const int light_radius = 10000;
	const double half_pseudo_degrees = 2 << 14;
	const double tol = 4.0 / half_pseudo_degrees;

	static constexpr float center_offset = MapUtility::tile_size / 2.f + .25f;
	static constexpr std::array<ivec2, 4> offsets = {
		ivec2(-1, -1),
		ivec2(-1, 1),
		ivec2(1, 1),
		ivec2(1, -1),
	};
};
