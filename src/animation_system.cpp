#include "animation_system.hpp"

/// static helpers
static void move_camera_to(const vec2& dest)
{
	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);
	camera_world_pos.position = dest;
}

static const vec2 get_camera_pos_from_buffer(const vec2& camera_pos,
											 const vec2& player_pos,
											 const vec2& buffer_top_left,
											 const vec2& buffer_down_right)
{
	vec2 offset_top_left = player_pos - buffer_top_left;
	vec2 offset_down_right = player_pos - buffer_down_right;
	vec2 map_top_left = MapUtility::map_position_to_world_position(MapUtility::map_top_left);
	vec2 map_bottom_right = MapUtility::map_position_to_world_position(MapUtility::map_down_right);

	vec2 final_pos;
	final_pos = max(min(camera_pos, camera_pos + offset_top_left), map_top_left);
	final_pos = min(max(final_pos, final_pos + offset_down_right), map_bottom_right);

	return final_pos;
}

void AnimationSystem::init() {}

void AnimationSystem::update_animations(float elapsed_ms, ColorState inactive_color)
{
	camera_update_position();
	auto animation_step_lambda = [&](Entity /*entity*/, Animation& animation) {
		animation.elapsed_time += elapsed_ms;
		if (animation.elapsed_time >= base_animation_speed / animation.speed_adjustment) {
			animation.elapsed_time = 0;
			animation.frame = ((animation.frame) + 1) % animation.max_frames;
		}
	};

	switch (inactive_color) {
	case ColorState::Red:
		registry.view<Animation>(entt::exclude<RedExclusive>).each(animation_step_lambda);

	case ColorState::Blue:
		registry.view<Animation>(entt::exclude<BlueExclusive>).each(animation_step_lambda);

	default:
		registry.view<Animation>().each(animation_step_lambda);
	}
	resolve_event_animations();
}

void AnimationSystem::set_sprite_direction(const Entity& sprite, Sprite_Direction direction)
{
	Animation& sprite_animation = registry.get<Animation>(sprite);

	if (direction == Sprite_Direction::SPRITE_LEFT) {
		sprite_animation.direction = -1;
	} else {
		// Sets direction to be facing right by default
		sprite_animation.direction = 1;
	}
}

void AnimationSystem::damage_animation(const Entity& entity)
{
	if (!registry.any_of<EventAnimation>(entity)) {
		Animation& entity_animation = registry.get<Animation>(entity);
		EventAnimation& damage_animation = registry.emplace<EventAnimation>(entity);
		this->animation_event_setup(entity_animation, damage_animation, entity_animation.display_color);
		entity_animation.display_color = damage_color;
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
	enemy_render = { enemy_type, EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::SMALL_SPRITE };

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

	if (registry.any_of<EventAnimation>(enemy)) {
		EventAnimation& enemy_event = registry.get<EventAnimation>(enemy);
		enemy_event.restore_state = state;
	} else {
		enemy_animation.state = state;
		enemy_animation.frame = 0;
	}
}

void AnimationSystem::enemy_attack_animation(const Entity& enemy)
{
	Animation& enemy_animation = registry.get<Animation>(enemy);

	if (!registry.any_of<EventAnimation>(enemy)) {
		EventAnimation& enemy_attack = registry.emplace<EventAnimation>(enemy);

		// Stores restoration states for the player's animations, to be called after animation event is resolves
		this->animation_event_setup(enemy_animation, enemy_attack, enemy_animation.display_color);

		// Sets animation state to be the beginning of the melee animation
		enemy_animation.state = static_cast<int>(EnemyAnimationEvents::Attack);
		enemy_animation.frame = 0;
		enemy_animation.speed_adjustment = enemy_attack_speed;
	}
}

void AnimationSystem::set_all_inactive_colours(ColorState inactive_color)
{
	Entity player = registry.view<Player>().front();
	PlayerInactivePerception& player_perception = registry.get<PlayerInactivePerception>(player);
	player_perception.inactive = inactive_color;
}

void AnimationSystem::set_player_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	vec3& player_color = registry.get<Color>(player).color;
	RenderRequest& player_render = registry.get<RenderRequest>(player);

	// Sets player rendering/animations to deafult values
	player_color = { 1, 1, 1 };
	player_animation.state = static_cast<int>(PlayerAnimationStates::Idle);
	player_animation.frame = 0;
	player_animation.max_frames = player_num_frames;
	player_animation.direction = 1;
	player_animation.elapsed_time = 0;
	player_animation.speed_adjustment = player_animation_speed;

	// Loads specific enemy spritesheet desired
	player_render = { TEXTURE_ASSET_ID::PALADIN, EFFECT_ASSET_ID::PLAYER, GEOMETRY_BUFFER_ID::SMALL_SPRITE };
}

void AnimationSystem::player_idle_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	player_animation.state = static_cast<int>(PlayerAnimationStates::Idle);
	player_animation.frame = 0;
}

void AnimationSystem::player_spellcast_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	player_animation.state = static_cast<int>(PlayerAnimationStates::Spellcast);
	player_animation.frame = 0;
}

void AnimationSystem::player_toggle_weapon(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	int next_state = (player_animation.state + 1) % player_weapon_states;

	if (registry.any_of<EventAnimation>(player)) {
		EventAnimation& player_event = registry.get<EventAnimation>(player);
		player_event.restore_state = next_state;
	} else {
		player_animation.state = next_state;
	}
}

void AnimationSystem::player_attack_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);
	if (player_animation.state == static_cast<int>(PlayerAnimationStates::Spellcast)) {
		return;
	}
	if (!registry.any_of<EventAnimation>(player)) {
		EventAnimation& player_melee = registry.emplace<EventAnimation>(player);

		this->animation_event_setup(player_animation, player_melee, player_animation.display_color);

		// Sets animation state to be the beginning of the melee animation
		player_animation.state = static_cast<int>(PlayerAnimationStates::Melee);
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_melee_speed;
	}
}

void AnimationSystem::player_running_animation(const Entity& player)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);

	if (!registry.any_of<EventAnimation>(player)) {
		EventAnimation& player_running = registry.emplace<EventAnimation>(player);

		this->animation_event_setup(player_animation, player_running, player_animation.display_color);

		// Sets animation state to be the beginning of the melee animation
		player_animation.state = static_cast<int>(PlayerAnimationStates::Running);
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_running_speed;
	}
}

void AnimationSystem::player_red_blue_animation(const Entity& player, ColorState color)
{
	Animation& player_animation = registry.get<Animation>(player);

	if (!registry.any_of<EventAnimation>(player)) {
		EventAnimation& player_melee = registry.emplace<EventAnimation>(player);

		//// Stores restoration states for the player's animations, to be called after animation event is resolved
		// player_melee.restore_speed = player_animation.speed_adjustment;
		// player_melee.restore_state = player_animation.state;
		// player_melee.restore_color = player_color;

		this->animation_event_setup(player_animation, player_melee, player_animation.display_color);

		// Sets animation state to be the beginning of the melee animation
		player_animation.frame = 0;
		player_animation.speed_adjustment = player_blue_red_switch_speed;

		switch (color) {
		case ColorState::Red:
			player_animation.display_color = player_red_transition_colour;
			break;
		case ColorState::Blue:
			player_animation.display_color = player_blue_transition_colour;
			break;
		default:
			player_animation.display_color = original_colours;
			break;
		}
	}
}

Entity AnimationSystem::create_boss_entry_entity(EnemyType boss_type, uvec2 map_position) {

	auto boss_entry_entity = registry.create();

	registry.emplace<RenderRequest>(boss_entry_entity, boss_type_entry_animation_map.at(boss_type), 
									EFFECT_ASSET_ID::BOSS_INTRO_SHADER, 
									GEOMETRY_BUFFER_ID::ENTRY_ANIMATION_STRIP, true);

	registry.emplace<MapPosition>(boss_entry_entity, map_position);
	Animation& entry_animation = registry.emplace<Animation>(boss_entry_entity);
	entry_animation.max_frames = 8;
	entry_animation.state = 0;
	entry_animation.speed_adjustment = 0.5;

	return boss_entry_entity;
}

void AnimationSystem::boss_event_animation(const Entity& boss, int event_state) {
	Animation& enemy_animation = registry.get<Animation>(boss);

	if (!registry.any_of<EventAnimation>(boss)) {
		EventAnimation& enemy_attack = registry.emplace<EventAnimation>(boss);

		// Stores restoration states for the player's animations, to be called after animation event is resolves
		this->animation_event_setup(enemy_animation, enemy_attack, enemy_animation.display_color);

		// Sets animation state to be the beginning of the melee animation
		enemy_animation.state = event_state;
		enemy_animation.frame = 0;
		enemy_animation.speed_adjustment = boss_action_speed;
	}
}

bool AnimationSystem::animation_events_completed() { return (registry.empty<EventAnimation>()); }

void AnimationSystem::resolve_event_animations()
{
	for (auto [entity, event_animation, actual_animation] : registry.view<EventAnimation, Animation>().each()) {

		// Checks if the animation frame had been reset to 0. If true, this means event animation has completed
		// TODO: Change to be a different check, this current one is a bit iffy, and is reliant on there being at least
		// two frames in all event animations (which is technically correct since it's an "animation", but a bit iffy)
		if (actual_animation.frame < event_animation.frame) {
			// Restores animation back to default before event animation was called
			actual_animation.speed_adjustment = event_animation.restore_speed;
			actual_animation.state = event_animation.restore_state;
			actual_animation.display_color = event_animation.restore_color;
			// Removes event animation from registry
			registry.remove<EventAnimation>(entity);
		} else {
			event_animation.frame = actual_animation.frame;
		}
	}
}

void AnimationSystem::animation_event_setup(Animation& animation, EventAnimation& EventAnimation, vec4& color)
{
	// Stores restoration states for the player's animations, to be called after animation event is resolved
	EventAnimation.restore_speed = animation.speed_adjustment;
	EventAnimation.restore_state = animation.state;
	EventAnimation.restore_color = color;
}

void AnimationSystem::camera_update_position()
{
	// TODO: add checking to see if in the story/cutscene, if is then block the buffer tracking
	camera_track_buffer();
}

void AnimationSystem::camera_track_buffer()
{
	vec2 window_size = screen_size * screen_scale;
	Entity player = registry.view<Player>().front();
	vec2 player_pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(player).position);

	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);

	vec2 buffer_top_left, buffer_down_right;

	std::tie(buffer_top_left, buffer_down_right)
		= CameraUtility::get_buffer_positions(camera_world_pos.position, window_size.x, window_size.y);

	vec2 final_camera_pos
		= get_camera_pos_from_buffer(camera_world_pos.position, player_pos, buffer_top_left, buffer_down_right);
	move_camera_to(final_camera_pos);
}
