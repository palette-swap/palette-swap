#include "animation_system.hpp"

void AnimationSystem::init()
{
}

void AnimationSystem::update_animations(float elapsed_ms, ColorState inactive_color)
{
	for (auto [entity, animation]: registry.view<Animation>().each()) {
		if (animation.color != inactive_color) {
			animation.elapsed_time += elapsed_ms;
			if (animation.elapsed_time >= base_animation_speed / animation.speed_adjustment) {
				animation.elapsed_time = 0;
				animation.frame = ((animation.frame) + 1) % animation.max_frames;
			}
		}

	}
	resolve_event_animations();
}


void AnimationSystem::set_sprite_direction(const Entity& sprite, Sprite_Direction direction)
{
	Animation& sprite_animation = registry.get<Animation>(sprite);

	if (direction == Sprite_Direction::SPRITE_LEFT) {
		sprite_animation.direction = -1;
	} else if (direction == Sprite_Direction::SPRITE_RIGHT) {
		sprite_animation.direction = 1;
		// Sets direction to be facing right by default
	} else {
		sprite_animation.direction = 1;
	}
}

// TODO: Swap out damage animation with something more interesting
void AnimationSystem::damage_animation(const Entity& entity)
{
	if (!registry.any_of<Event_Animation>(entity)) {
		Animation& entity_animation = registry.get<Animation>(entity);
		vec3& entity_color = registry.get<Color>(entity).color;
		Event_Animation& damage_animation = registry.emplace<Event_Animation>(entity);
		this->animation_event_setup(entity_animation, damage_animation, entity_color);
		entity_color = damage_color;
		entity_animation.speed_adjustment = damage_animation_speed;
	}
}

void AnimationSystem::attack_animation(const Entity& entity) 
{ 
	if (registry.any_of<Player>(entity)) {
		this->player_attack_animation(entity);
	} else {
		this->enemy_attack_animation(entity);
	}
}

void AnimationSystem::set_enemy_animation(const Entity& enemy, TEXTURE_ASSET_ID enemy_type, ColorState color) 
{
	Animation& enemy_animation = registry.get<Animation>(enemy);
	RenderRequest& enemy_render = registry.get<RenderRequest>(enemy);
	vec3& enemy_color = registry.get<Color>(enemy).color;

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

void AnimationSystem::set_enemy_state(const Entity& enemy, int state)
{
	Animation& enemy_animation = registry.get<Animation>(enemy);

	if (registry.any_of<Event_Animation>(enemy)) {
		Event_Animation& enemy_event = registry.get<Event_Animation>(enemy);
		enemy_event.restore_state = state;
	} else {
		enemy_animation.state = state;
		enemy_animation.frame = 0;
	}
	
}

void AnimationSystem::enemy_attack_animation(const Entity& enemy)
{
	Animation& enemy_animation = registry.get<Animation>(enemy);
	vec3& enemy_color = registry.get<Color>(enemy).color;

	if (!registry.any_of<Event_Animation>(enemy)) {
		Event_Animation& enemy_attack = registry.emplace<Event_Animation>(enemy);

		// Stores restoration states for the player's animations, to be called after animation event is resolves
		this->animation_event_setup(enemy_animation, enemy_attack, enemy_color);

		// Sets animation state to be the beginning of the melee animation
		enemy_animation.state = static_cast<int>(enemy_animation_events::Attack);
		enemy_animation.frame = 0;
		enemy_animation.speed_adjustment = enemy_attack_speed;
	}
}

void AnimationSystem::set_player_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	vec3& player_color = registry.get<Color>(player).color;
	RenderRequest& player_render = registry.get<RenderRequest>(player);

	// Sets player rendering/animations to deafult values
	player_color = { 1, 1, 1 };
	player_animation.state = static_cast<int>(player_animation_states::Idle);
	player_animation.frame = 0;
	player_animation.max_frames = player_num_frames;
	player_animation.direction = 1;
	player_animation.elapsed_time = 0;
	player_animation.speed_adjustment = player_animation_speed;

	// Loads specific enemy spritesheet desired
	player_render = { TEXTURE_ASSET_ID::PALADIN, EFFECT_ASSET_ID::PLAYER, GEOMETRY_BUFFER_ID::PLAYER };
}

void AnimationSystem::player_idle_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	player_animation.state = static_cast<int>(player_animation_states::Idle);
	player_animation.frame = 0;
}

void AnimationSystem::player_spellcast_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	player_animation.state = static_cast<int>(player_animation_states::Spellcast);
	player_animation.frame = 0;
}

void AnimationSystem::player_toggle_weapon(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	int next_state = (player_animation.state + 1) % player_weapon_states;
	

	if (registry.any_of<Event_Animation>(player)) {
		Event_Animation& player_event = registry.get<Event_Animation>(player);
		player_event.restore_state = next_state;
	} else {
		player_animation.state = next_state;
	}
}


void AnimationSystem::player_attack_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	vec3& player_color = registry.get<Color>(player).color;
	if (player_animation.state == static_cast<int>(player_animation_states::Spellcast)) {
		return;
	}
	if (!registry.any_of<Event_Animation>(player)) {
		Event_Animation& player_melee = registry.emplace<Event_Animation>(player);

		this->animation_event_setup(player_animation, player_melee, player_color);

		// Sets animation state to be the beginning of the melee animation
		player_animation.state = static_cast<int>(player_animation_states::Melee);
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_melee_speed;
	}
}

void AnimationSystem::player_running_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	vec3& player_color = registry.get<Color>(player).color;

	if (!registry.any_of<Event_Animation>(player)) {
		Event_Animation& player_running = registry.emplace<Event_Animation>(player);

		this->animation_event_setup(player_animation, player_running, player_color);

		// Sets animation state to be the beginning of the melee animation
		player_animation.state = static_cast<int>(player_animation_states::Running);
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_running_speed;
	}
}

void AnimationSystem::player_red_blue_animation(const Entity& player, ColorState color)
{
	Animation& player_animation = registry.get<Animation>(player);
	vec3& player_color = registry.get<Color>(player).color;

	if (!registry.any_of<Event_Animation>(player)) {
		Event_Animation& player_melee = registry.emplace<Event_Animation>(player);

		//// Stores restoration states for the player's animations, to be called after animation event is resolved
		//player_melee.restore_speed = player_animation.speed_adjustment;
		//player_melee.restore_state = player_animation.state;
		//player_melee.restore_color = player_color;

		this->animation_event_setup(player_animation, player_melee, player_color);

		// Sets animation state to be the beginning of the melee animation
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_blue_red_switch_speed;

		switch (color) {
		case ColorState::Red:
			player_color = { 2, 0.8, 0.8 };
			break;
		case ColorState::Blue:
			player_color = { 0.5, 0.5, 3 };
			break;
		default:
			player_color = { 1, 1, 1 };
			break;
		}
		
	}
}


bool AnimationSystem::animation_events_completed() { return (registry.empty<Event_Animation>()); }

void AnimationSystem::resolve_event_animations()
{
	for (auto [entity, event_animation, actual_animation, entity_color] :
		 registry.view<Event_Animation, Animation, Color>().each()) {

		// Checks if the animation frame had been reset to 0. If true, this means event animation has completed
		// TODO: Change to be a different check, this current one is a bit iffy, and is reliant on there being at least
		// two frames in all event animations (which is technically correct since it's an "animation", but a bit iffy)
		if (actual_animation.frame < event_animation.frame) {
			// Restores animation back to default before event animation was called
			actual_animation.speed_adjustment = event_animation.restore_speed;
			actual_animation.state = event_animation.restore_state;
			entity_color.color = event_animation.restore_color;
			// Removes event animation from registry
			registry.remove<Event_Animation>(entity);
		} else {
			event_animation.frame = actual_animation.frame;
		}
	}
}

void AnimationSystem::animation_event_setup(Animation& animation, Event_Animation& event_animation, vec3& color) 
{
	// Stores restoration states for the player's animations, to be called after animation event is resolved
	event_animation.restore_speed = animation.speed_adjustment;
	event_animation.restore_state = animation.state;
	event_animation.restore_color = color;
}