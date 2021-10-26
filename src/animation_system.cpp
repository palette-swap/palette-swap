#include "animation_system.hpp"

AnimationSystem::AnimationSystem(std::shared_ptr<TurnSystem> turns)
	: turns(std::move(turns))
{
}

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

	resolve_event_animations();
}

void AnimationSystem::resolve_event_animations()
{
	for (Entity animation_entity : registry.event_animations.entities) {
		Event_Animation& event_animation = registry.event_animations.get(animation_entity);
		Animation& actual_animation = registry.animations.get(animation_entity);

		// Checks if the animation frame had been reset to 0. If true, this means event animation has completed
		// TODO: Change to be a different check, this current one is a bit iffy, and is reliant on there being at least two
		// frames in all event animations (which is technically correct since it's an "animation", but a bit iffy)
		if (actual_animation.frame < event_animation.frame) {
			// Restores animation back to default before event animation was called
			actual_animation.speed_adjustment = event_animation.restore_speed;
			actual_animation.state = event_animation.restore_state;
			bool turn_trigger = event_animation.turn_trigger;
			// Removes event animation from registry
			registry.event_animations.remove(animation_entity);
			// If event animation should trigger turn over for the entity, triggers it here
			if (turn_trigger == true) {

			}
		} else {
			event_animation.frame = actual_animation.frame;
		}
	}
}

// TODO: Change texture asset id to enemy enum that Franky has implemented
void AnimationSystem::set_enemy_animation(Entity enemy, TEXTURE_ASSET_ID enemy_type, ColorState color) 
{
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

void AnimationSystem:: initialize_player_animation(Entity player) {
	Animation& player_animation = registry.animations.get(player);
	vec3& player_color = registry.colors.get(player);
	RenderRequest& player_render = registry.render_requests.get(player);

	// Sets player rendering/animations to deafult values
	player_color = { 1, 1, 1 };
	player_animation.state = (int)player_animation_states::Idle;
	player_animation.frame = 0;
	player_animation.max_frames = player_num_frames;
	player_animation.direction = 1;
	player_animation.elapsed_time = 0;
	player_animation.speed_adjustment = player_animation_speed;

	// Loads specific enemy spritesheet desired
	player_render = { TEXTURE_ASSET_ID::PALADIN, EFFECT_ASSET_ID::PLAYER, GEOMETRY_BUFFER_ID::PLAYER };
}

void AnimationSystem::set_sprite_direction(Entity sprite, Sprite_Direction direction) { 
	Animation& sprite_animation = registry.animations.get(sprite);

	if (direction == Sprite_Direction::SPRITE_LEFT) {
		sprite_animation.direction = -1;
	} else if (direction == Sprite_Direction::SPRITE_RIGHT) {
		sprite_animation.direction = 1;	
	// Sets direction to be facing right by default
	} else {
		sprite_animation.direction = 1;	
	}
}

void AnimationSystem::player_attack_animation(Entity player) {
	Animation& player_animation = registry.animations.get(player);

	if (!registry.event_animations.has(player)) {
		Event_Animation player_melee = registry.event_animations.emplace(player);

		// Stores restoration states for the player's animations, to be called after animation event is resolved
		player_melee.restore_speed = player_animation.speed_adjustment;
		player_melee.restore_state = player_animation.state;
		// Tells the turn system that the player entity's turn should end after this animation plays out
		player_melee.turn_trigger = true;

		// Sets animation state to be the beginning of the melee animation
		player_animation.state = (int)player_animation_states::Melee;
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_melee_speed;
	}

	// TODO: Add callback after player attack completes
}

void AnimationSystem::player_running_animation(Entity player)
{
	Animation& player_animation = registry.animations.get(player);

	if (!registry.event_animations.has(player)) {
		Event_Animation player_melee = registry.event_animations.emplace(player);

		// Stores restoration states for the player's animations, to be called after animation event is resolved
		player_melee.restore_speed = player_animation.speed_adjustment;
		player_melee.restore_state = player_animation.state;

		// Sets animation state to be the beginning of the melee animation
		player_animation.state = (int)player_animation_states::Running;
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_running_speed;
	}

	// TODO: Add call after player's attack animation completes
}

void AnimationSystem::damage_animation(Entity entity) 
{ 
	Animation& entity_animation = registry.animations.get(entity); 
	vec3& entity_color = registry.colors.get(entity);
}