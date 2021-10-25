#pragma once
#include <memory>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "tiny_ecs_registry.hpp"

// Total number of frames in a spritesheet. Currently used for some test spritesheets.
// Number of actual frames can be adapted to vary from asset to asset
// TODO: change number of frames for an animation to vary based on the asset

// Currently this number is used for all enemies animation frames (a total of 4)
// Moving forward the animation frames by enemy type may chang
static constexpr int enemy_num_frames = 4;
static constexpr int base_animation_speed = 150;
static constexpr int player_frames = 6;

static constexpr vec3 default_enemy_red = { 2, 1, 1 };
static constexpr vec3 default_enemy_blue = { 1, 1, 2 };

class AnimationSystem {

public:
	void init();
	// Updates all animated entities based on elapsed time, changes their frame based on the time
	void update_animations(float elapsed_ms);	
	void set_enemy_animation(Entity enemy, TEXTURE_ASSET_ID enemy_type, ColorState color);
};