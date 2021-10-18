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
			animation.frame = ((animation.frame) + 1) % num_frames;
		}
		
	}
}