#pragma once
#include <memory>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "tiny_ecs_registry.hpp"

// Total number of frames in a spritesheet. Currently used for some test spritesheets.
// Number of actual frames can be adapted to vary from asset to asset
// TODO: change number of frames for an animation to vary based on the asset

static constexpr int num_frames = 4;
static constexpr int base_animation_speed = 300;

class AnimationSystem {

public:
	void init();
	// Updates all animated entities based on elapsed time, changes their frame based on the time
	void update_animations(float elapsed_ms);	
};