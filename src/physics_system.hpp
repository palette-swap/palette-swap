#pragma once

#include "common.hpp"
#include "components.hpp"
#include "map_generator_system.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem {
public:
	explicit PhysicsSystem(const Debug& debugging, std::shared_ptr<MapGeneratorSystem> map);

	void step(float elapsed_ms, float /*window_width*/, float /*window_height*/);

private:
	const Debug& debugging;
	const std::shared_ptr<MapGeneratorSystem> map_generator;
};