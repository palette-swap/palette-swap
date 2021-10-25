#include "animation_system.hpp"

void AnimationSystem::init() {}

void AnimationSystem::update_animations(float elapsed_ms)
{
	auto& animations = registry.animations;
	for (uint i = 0; i < animations.size(); i++) {
		Animation& animation = animations.components[i];

		animation.elapsed_time += elapsed_ms;
		if (animation.elapsed_time >= base_animation_speed/animation.speed_adjustment) {
			animation.elapsed_time = 0;
			animation.frame = ((animation.frame) + 1) % animation.max_frames;
		}
		
	}
}

// TODO: Change texture asset id to enemy enum that Franky has implemented
void AnimationSystem::create_enemy_animation(Entity enemy, TEXTURE_ASSET_ID enemy_type, ColorState color) {
	Animation& enemy_animation = registry.animations.get(enemy);
	RenderRequest& enemy_render = registry.render_requests.get(enemy);
	vec3& enemy_color = registry.colors.get(enemy);

	// Resets enemy animation status to default
	// TODO: specify desired animation speed by enemy type here (load from file)
	enemy_animation.state = 0;
	enemy_animation.frame = 0;
	enemy_animation.max_frames = enemy_num_frames;
	enemy_animation.direction = 1;
	enemy_animation.elapsed_time = 0;
	enemy_animation.speed_adjustment = 1;

	// Loads specific enemy spritesheet desired
	enemy_render = { enemy_type, EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::ENEMY };

	// Changes enemy color to a predefined default of red or blue (or nothing, if that's what you want)
	if (color == ColorState::Red) {
		enemy_color = default_enemy_red;
	} else if (color == ColorState::Blue) {
		enemy_color = default_enemy_blue;
	} else {
		enemy_color = { 1, 1, 1 };
	}
}