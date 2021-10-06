// internal
#include "ai_system.hpp"
#include "components.hpp"

// AI logic
AISystem::AISystem(std::shared_ptr<MapGeneratorSystem> map_generator):
	map_generator(std::move(map_generator)) {
}

void AISystem::step(float /*elapsed_ms*/, bool& isPlayerTurn) {
	if (isPlayerTurn) {
		return;
	}

	for (const Entity& enemy_entity: registry.enemy_states.entities) {

		switch (registry.enemy_states.get(enemy_entity).current_state) {

		case ENEMY_STATE_ID::IDLE:
			if (is_player_spotted(enemy_entity, 3)) {
				become_alert(enemy_entity);
				switch_to_active(enemy_entity);
			}
			break;

		case ENEMY_STATE_ID::ACTIVE:
			if (is_player_spotted(enemy_entity, 6)) {
				if (is_player_reachable(enemy_entity, 1)) {
					attack_player(enemy_entity);
				} else {
					approach_player(enemy_entity);
				}

				if (is_afraid(enemy_entity)) {
					switch_to_flinched(enemy_entity);
				}
			} else {
				switch_to_idle(enemy_entity);
			}
			break;

		case ENEMY_STATE_ID::FLINCHED:
			if (is_at_nest(enemy_entity)) {
				switch_to_idle(enemy_entity);
			} else {
				approach_nest(enemy_entity);
			}
			break;

		default:
			throw std::runtime_error("Invalid Enemy State");
		}
	}

	isPlayerTurn = true;
}

void AISystem::switch_to_idle(const Entity& enemy_entity) {
	registry.enemy_states.get(enemy_entity).current_state = ENEMY_STATE_ID::IDLE;
	registry.render_requests.get(enemy_entity).used_texture = TEXTURE_ASSET_ID::SLUG;
	registry.colors.get(enemy_entity) = { 1, 4, 1 };
}

void AISystem::switch_to_active(const Entity& enemy_entity) {
	registry.enemy_states.get(enemy_entity).current_state = ENEMY_STATE_ID::ACTIVE;
	registry.render_requests.get(enemy_entity).used_texture = TEXTURE_ASSET_ID::SLUG_ALERT;
	registry.colors.get(enemy_entity) = { 1, 1, 1 };
}

void AISystem::switch_to_flinched(const Entity& enemy_entity) {
	registry.enemy_states.get(enemy_entity).current_state = ENEMY_STATE_ID::FLINCHED;
	registry.render_requests.get(enemy_entity).used_texture = TEXTURE_ASSET_ID::SLUG;
	registry.colors.get(enemy_entity) = { 0, 1, 8 };
}

// Depends on Turn System from Nathan.

bool AISystem::is_player_turn() {
	// TODO
	return true;
}

void AISystem::switch_to_player_turn() {
	// TODO
}

bool AISystem::is_player_spotted(const Entity& entity, const uint radius) {
	assert(registry.players.entities.size() == 1);
	uvec2 player_map_pos = registry.map_positions.get(registry.players.entities[0]).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	ivec2 distance = entity_map_pos - player_map_pos;
	return uint(abs(distance.x)) <= radius && uint(abs(distance.y)) <= radius;
}

bool AISystem::is_player_reachable(const Entity& entity, const uint attack_range) {
	return is_player_spotted(entity, attack_range);
}

bool AISystem::is_afraid(const Entity& /*entity*/) {
	// TODO: M2 whether enemies is afraid depends on their real HP.
	uint hp = uint(uniform_dist(rng) * 100.f);
	printf("Enemy HP: %d\n", hp);
	if (hp < 5) {
		printf("Enemy is afraid...\n");
		return true;
	}
	return false;
}

bool AISystem::is_at_nest(const Entity& entity) {
	assert(registry.enemy_nest_positions.has(entity));
	uvec2 nest_map_pos = registry.enemy_nest_positions.get(entity).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	return nest_map_pos == entity_map_pos;
}

void AISystem::become_alert(const Entity& /*entity*/) {
	// TODO: M2 add animation/sound.
	printf("Enemy becomes alert!\n");
}

void AISystem::attack_player(const Entity& /*entity*/) {
	// TODO: M2 add animation/sound and injure player.
	printf("Enemy attacks player!\n");
}

bool AISystem::approach_player(const Entity& entity) {
	assert(registry.players.entities.size() == 1);
	uvec2 player_map_pos = registry.map_positions.get(registry.players.entities[0]).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, player_map_pos);
	if (shortest_path.size() > 2) {
		uvec2 next_map_pos = shortest_path[1];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::approach_nest(const Entity& entity) {
	assert(registry.enemy_nest_positions.has(entity));
	uvec2 nest_map_pos = registry.enemy_nest_positions.get(entity).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, nest_map_pos);
	if (shortest_path.size() > 1) {
		uvec2 next_map_pos = shortest_path[1];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::move(const Entity& entity, const uvec2& map_pos) {
	MapPosition& entity_map_pos = registry.map_positions.get(entity);
	if (entity_map_pos.position != map_pos && map_generator->walkable(map_pos)) {
		entity_map_pos.position = map_pos;
		registry.motions.get(entity).position = map_position_to_screen_position(map_pos);
		return true;
	}
	return false;
}
