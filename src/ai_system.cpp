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
			if (is_player_spotted(enemy_entity, 5)) {
				become_alert(enemy_entity);
				switch_to_active(enemy_entity);
			}
			break;

		case ENEMY_STATE_ID::ACTIVE:
			if (is_player_spotted(enemy_entity, 7)) {
				if (is_player_reachable(enemy_entity, 1)) {
					attack_player();
				} else {
					approach_player();
				}

				if (is_afraid()) {
					switch_to_flinched(enemy_entity);
				}
			} else {
				switch_to_idle(enemy_entity);
			}
			break;

		case ENEMY_STATE_ID::FLINCHED:
			if (is_player_spotted(enemy_entity, 5)) {
				flee_player();
			} else {
				switch_to_idle(enemy_entity);
			}
			break;

		default:
			printf("Should never happen.\n");
			throw std::runtime_error("Invalid Enemy State");
		}
	}

	isPlayerTurn = true;
}

void AISystem::switch_to_idle(const Entity& enemy_entity) {
	registry.enemy_states.get(enemy_entity).current_state = ENEMY_STATE_ID::IDLE;
	registry.render_requests.get(enemy_entity).used_texture = TEXTURE_ASSET_ID::SLUG;
	registry.colors.get(enemy_entity) = { 1, 1, 1 };
}

void AISystem::switch_to_active(const Entity& enemy_entity) {
	registry.enemy_states.get(enemy_entity).current_state = ENEMY_STATE_ID::ACTIVE;
	registry.render_requests.get(enemy_entity).used_texture = TEXTURE_ASSET_ID::SLUG_ALERT;
	registry.colors.get(enemy_entity) = { 1, 1, 4 };
}

void AISystem::switch_to_flinched(const Entity& enemy_entity) {
	registry.enemy_states.get(enemy_entity).current_state = ENEMY_STATE_ID::FLINCHED;
	registry.render_requests.get(enemy_entity).used_texture = TEXTURE_ASSET_ID::SLUG;
	registry.colors.get(enemy_entity) = { 1, 4, 1 };
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
	uvec2 entity_map_position = registry.map_positions.get(entity).position;
	uvec2 player_map_position = registry.map_positions.get(registry.players.entities[0]).position;

	ivec2 distance = entity_map_position - player_map_position;
	return uint(abs(distance.x)) <= radius && uint(abs(distance.y)) <= radius;
}

bool AISystem::is_player_reachable(const Entity& entity, const uint attack_range) {
	return is_player_spotted(entity, attack_range);
}

bool AISystem::is_afraid() {
	// TODO: M2 whether enemies is afraid depends on their real HP.
	uint hp = uint(uniform_dist(rng) * 100.f);
	printf("HP: %d\n", hp);
	if (hp < 20) {
		printf("Enemy is afraid...\n");
		return true;
	}
	return false;
}

void AISystem::become_alert(const Entity& /*entity*/) {
	// TODO: M2 add animation/sound.
	printf("Enemy becomes alert!\n");
}

void AISystem::attack_player() {
	// TODO: M2 add animation/sound and injure player.
	printf("Enemy attacks player!\n");
}

// Depends on BFS from Dylan.

void AISystem::approach_player() {
	// TODO
}


void AISystem::flee_player() {
	// TODO
}
