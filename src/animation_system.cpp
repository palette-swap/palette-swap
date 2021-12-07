#include "animation_system.hpp"

/// static helpers
static void move_camera_to(const vec2& dest)
{
	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);
	camera_world_pos.position = dest;
}

static vec2 get_camera_pos_from_buffer(const vec2& camera_pos,
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

void AnimationSystem::init(RenderSystem* render_system) { renderer = render_system; }

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
	resolve_transient_event_animations();
	resolve_undisplay_event_animations();
	resolve_travel_event_animations(elapsed_ms);
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

void AnimationSystem::set_enemy_facing_player(const Entity& enemy)
{
	const Entity& player = registry.view<Player>().front();
	MapPosition& player_position = registry.get<MapPosition>(player);
	MapPosition& enemy_position = registry.get<MapPosition>(enemy);

	uint distance = player_position.position.x - player_position.position.x;

	Sprite_Direction direction = Sprite_Direction::SPRITE_LEFT;
	if (distance > 0) {
		direction = Sprite_Direction::SPRITE_RIGHT;
	} 

	set_sprite_direction(enemy, direction);

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

	printf("The enemy state was set back to idle");
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

void AnimationSystem::enemy_tile_transition(const Entity& enemy, uvec2 map_start_point, uvec2 map_end_point)
{
	EnemyType enemy_type = registry.get<Enemy>(enemy).type;
	Animation& enemy_animation = registry.get<Animation>(enemy);
	TravelEventAnimation& enemy_transition = registry.emplace<TravelEventAnimation>(enemy);

	enemy_transition.restore_speed = enemy_animation.speed_adjustment;
	enemy_transition.restore_state = enemy_animation.state;
	enemy_transition.start_point = MapUtility::map_position_to_world_position(map_start_point);
	enemy_transition.end_point = MapUtility::map_position_to_world_position(map_end_point);
	enemy_transition.middle_point = (enemy_transition.start_point + enemy_transition.end_point) * 0.5f;
	enemy_transition.max_time = enemy_tile_travel_time_ms;

	registry.emplace<WorldPosition>(enemy, enemy_transition.start_point);


	enemy_transition.middle_point -= vec2(0, MapUtility::tile_size * enemy_animation.travel_offset);

}

void AnimationSystem::set_enemy_death_animation(const Entity& enemy) 
{
	auto enemy_death_entity = registry.create();

	MapPosition &position = registry.get<MapPosition>(enemy);
	RenderRequest &enemy_render = registry.get<RenderRequest>(enemy);
	Animation &enemy_animation = registry.get<Animation>(enemy);

	registry.emplace<MapPosition>(enemy_death_entity, position);
	registry.emplace<TransientEventAnimation>(enemy_death_entity);
	registry.emplace<EffectRenderRequest>(
		enemy_death_entity, enemy_render.used_texture, EFFECT_ASSET_ID::DEATH, GEOMETRY_BUFFER_ID::DEATH, true);

	Animation& enemy_death_animation = registry.emplace<Animation>(enemy_death_entity);

	// Copies over enemy animation states from previous animation
	AnimationSystem::copy_animation_settings(enemy_animation, enemy_death_animation);
	// Changes death animation settings slightly
	enemy_death_animation.display_color = { 1, 1, 1, 0.8f };
	enemy_death_animation.speed_adjustment = enemy_death_animation_speed;
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

void AnimationSystem::player_toggle_spell(const Entity& player_arrow, int spell_type) 
{ 
	assert(registry.any_of<Animation>(player_arrow)); 
	Animation& player_spell_animation = registry.get<Animation>(player_arrow);
	player_spell_animation.state = spell_type;
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

void AnimationSystem::player_running_animation(const Entity& player, uvec2 map_start_point, uvec2 map_end_point)
{
	assert(registry.any_of<Player>(player));
	Animation& player_animation = registry.get<Animation>(player);

	if (!registry.any_of<TravelEventAnimation>(player)) {
		TravelEventAnimation& player_running = registry.emplace<TravelEventAnimation>(player);

		player_running.restore_speed = player_animation.speed_adjustment;
		player_running.restore_state = player_animation.state;
		player_running.start_point = MapUtility::map_position_to_world_position(map_start_point);
		player_running.end_point = MapUtility::map_position_to_world_position(map_end_point);
		player_running.middle_point = (player_running.start_point + player_running.end_point) * 0.5f;
		player_running.max_time = player_tile_travel_time_ms;

		registry.emplace<WorldPosition>(player, player_running.start_point);

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

void AnimationSystem::player_spell_impact_animation(const Entity& enemy, DamageType spelltype)
{
	auto spell_impact_entity = registry.create();
	MapPosition position = registry.get<MapPosition>(enemy);

	registry.emplace<MapPosition>(spell_impact_entity, position);
	registry.emplace<TransientEventAnimation>(spell_impact_entity);
	registry.emplace<EffectRenderRequest>(
		spell_impact_entity, TEXTURE_ASSET_ID::SPELLS, EFFECT_ASSET_ID::SPELL, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);
	Animation& spell_impact_animation = registry.emplace<Animation>(spell_impact_entity);
	spell_impact_animation.max_frames = spell_impact_total_frames;
	spell_impact_animation.state = damage_type_to_spell_impact.at((int)spelltype);
	spell_impact_animation.speed_adjustment = player_spell_impact_speed;
}

Entity AnimationSystem::create_boss_entry_entity(EnemyType boss_type, uvec2 map_position) {

	auto boss_entry_entity = registry.create();

	registry.emplace<RenderRequest>(boss_entry_entity, boss_type_entry_animation_map.at(boss_type).texture, 
									EFFECT_ASSET_ID::BOSS_INTRO_SHADER, 
									GEOMETRY_BUFFER_ID::ENTRY_ANIMATION_STRIP, true);

	registry.emplace<MapPosition>(boss_entry_entity, map_position);
	Animation& entry_animation = registry.emplace<Animation>(boss_entry_entity);
	entry_animation.max_frames = 8;
	entry_animation.state = 0;
	entry_animation.speed_adjustment = 0.5;

	registry.emplace<EntryAnimationEnemy>(boss_entry_entity, boss_type);

	return boss_entry_entity;
}

void AnimationSystem::trigger_full_boss_intro(const Entity& boss_entity) {
	Animation& boss_animation = registry.get<Animation>(boss_entity);
	EnemyType boss_type = registry.get<EntryAnimationEnemy>(boss_entity).intro_enemy_type;
	boss_animation.max_frames = boss_type_entry_animation_map.at(boss_type).max_frames;
	boss_animation.frame = 0;
	registry.emplace<UndisplayEventAnimation>(boss_entity);
}

bool AnimationSystem::boss_intro_complete(const Entity& boss_entity)
{
	return (!registry.any_of<UndisplayEventAnimation> (boss_entity));
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


void AnimationSystem::boss_special_attack_animation(Entity boss, int attack_state) {

	// Finds player's location for attack
	Entity player_entity = registry.view<Player>().front();
	uvec2 player_location = registry.get<MapPosition>(player_entity).position;

		// Sets boss's attack animation to be the regular one
	auto boss_type = registry.get<Enemy>(boss).type;
	auto& boss_animation = registry.get<Animation>(boss);

	if (!registry.any_of<EventAnimation>(boss)) {
		EventAnimation& boss_attack = registry.emplace<EventAnimation>(boss);

		// Stores restoration states for the player's animations, to be called after animation event is resolves
		this->animation_event_setup(boss_animation, boss_attack, boss_animation.display_color);

		// Sets animation state to be the beginning of the boss regular attack animation
		boss_animation.state = attack_state;
		boss_animation.frame = 0;
		boss_animation.speed_adjustment = boss_ranged_attack_speed;
	}

	// Creates a remote attack entity based on attack spritesheet
	auto boss_range_attack_entity = registry.create();
	registry.emplace<MapPosition>(boss_range_attack_entity, player_location);
	registry.emplace<TransientEventAnimation>(boss_range_attack_entity);
	registry.emplace<EffectRenderRequest>(boss_range_attack_entity,
										  boss_type_attack_spritesheet.at(boss_type),
										  EFFECT_ASSET_ID::ENEMY,
										  GEOMETRY_BUFFER_ID::SMALL_SPRITE,
										  true);
	Animation& spell_impact_animation = registry.emplace<Animation>(boss_range_attack_entity);
	spell_impact_animation.max_frames = boss_ranged_attack_total_frames;
	spell_impact_animation.state = attack_state;
	spell_impact_animation.speed_adjustment = boss_ranged_attack_speed;
}

void AnimationSystem::trigger_aoe_attack_animation(const Entity& aoe, int aoe_state) { 
	Animation& aoe_animation = registry.get<Animation>(aoe); 
	AOESquare& aoe_status = registry.get<AOESquare>(aoe);

	aoe_animation.state = aoe_state;
	aoe_animation.frame = 0;
	aoe_animation.max_frames = 8;
	aoe_animation.speed_adjustment = 0.8f;
	aoe_status.actual_attack_displayed = true;

	registry.emplace<UndisplayEventAnimation>(aoe);
}

bool AnimationSystem::animation_events_completed() { return (registry.empty<EventAnimation, TransientEventAnimation, TravelEventAnimation>()); }

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

void AnimationSystem::resolve_transient_event_animations()
{
	for (auto [entity, event_animation, actual_animation] : registry.view<TransientEventAnimation, Animation>().each()) {

		if (actual_animation.frame < event_animation.frame) {
			registry.destroy(entity);
		} else {
			event_animation.frame = actual_animation.frame;
		}
	}
}

void AnimationSystem::resolve_undisplay_event_animations()
{
	for (auto [entity, event_animation, actual_animation, effect] :
		 registry.view<UndisplayEventAnimation, Animation, EffectRenderRequest>().each()) {

		if (actual_animation.frame < event_animation.frame) {
			effect.visible = false;
			registry.remove<UndisplayEventAnimation>(entity);
		} else {
			event_animation.frame = actual_animation.frame;
		}
	}

	for (auto [entity, event_animation, actual_animation, render] :
		 registry.view<UndisplayEventAnimation, Animation, RenderRequest>().each()) {

		if (actual_animation.frame < event_animation.frame) {
			render.visible = false;
			registry.remove<UndisplayEventAnimation>(entity);
		} else {
			event_animation.frame = actual_animation.frame;
		}
	}
}

void AnimationSystem::resolve_travel_event_animations(float elapsed_ms) 
{
	for (auto [entity, travel_animation, actual_animation, world_position] :
		 registry.view<TravelEventAnimation, Animation, WorldPosition>().each()) {

		 travel_animation.total_time += elapsed_ms;

		 if (travel_animation.total_time >= travel_animation.max_time) {
			 actual_animation.state = travel_animation.restore_state;
			 actual_animation.speed_adjustment = travel_animation.restore_speed;

			 // Removes world position from entity, should return entity to map position
			 registry.remove<TravelEventAnimation, WorldPosition>(entity);
		 } else {
			 float time_percent = travel_animation.total_time / travel_animation.max_time;

			 if (registry.any_of<Player>(entity)) {
				 world_position.position = (travel_animation.end_point - travel_animation.start_point) * time_percent
					 + travel_animation.start_point;
			 } else {

					 if (time_percent <= 0.5f) {
						 float norm_time = time_percent / 0.5f;
						 float y_offset = travel_animation.middle_point.y - travel_animation.start_point.y;
						 world_position.position.y = travel_animation.start_point.y
							 + y_offset * (-norm_time * norm_time) * (2 * norm_time - 3);
					 } else {
						 float norm_time = (1.f - time_percent) / 0.5f;
						 float y_offset = travel_animation.middle_point.y - travel_animation.end_point.y;
						 world_position.position.y = travel_animation.middle_point.y
							 - y_offset * ((norm_time * norm_time) * (2 * norm_time - 3) + 1) ;
					 }

					 world_position.position.x
						 = (travel_animation.end_point.x - travel_animation.start_point.x) * time_percent
						 + travel_animation.start_point.x;
			 }
			 
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

void AnimationSystem::copy_animation_settings(Animation& original, Animation& copy)
{
	copy.max_frames = original.max_frames;
	copy.direction = original.direction;
	copy.state = original.state;
	copy.display_color = original.display_color;
}

void AnimationSystem::camera_update_position()
{
	// TODO: add checking to see if in the story/cutscene, if is then block the buffer tracking
	camera_track_buffer();
}

void AnimationSystem::camera_track_buffer()
{
	vec2 window_size = renderer->get_screen_size() * renderer->get_screen_scale();
	Entity player = registry.view<Player>().front();

	vec2 player_pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(player).position);

	if (registry.any_of<WorldPosition>(player)) {
		player_pos = registry.get<WorldPosition>(player).position;
	}

	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);

	vec2 buffer_top_left, buffer_down_right;

	std::tie(buffer_top_left, buffer_down_right)
		= CameraUtility::get_buffer_positions(camera_world_pos.position, window_size.x, window_size.y);

	vec2 final_camera_pos
		= get_camera_pos_from_buffer(camera_world_pos.position, player_pos, buffer_top_left, buffer_down_right);
	move_camera_to(final_camera_pos);
}
