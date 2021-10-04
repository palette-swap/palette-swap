#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:

	PhysicsSystem(const Debug& debugging);

	void step(float elapsed_ms, float /*window_width_px*/, float /*window_height_px*/);

private:
	const Debug& debugging;
};