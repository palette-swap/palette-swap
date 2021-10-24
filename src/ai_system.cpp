// internal
#include "ai_system.hpp"
#include "components.hpp"

// AI logic
AISystem::AISystem(std::shared_ptr<CombatSystem> combat,
				   std::shared_ptr<MapGeneratorSystem> map_generator,
				   std::shared_ptr<TurnSystem> turns)
	: combat(std::move(combat))
	, map_generator(std::move(map_generator))
	, turns(std::move(turns))
{
	registry.debug_components.emplace(enemy_team);

	this->turns->add_team_to_queue(enemy_team);
}

void AISystem::step(float /*elapsed_ms*/)
{
	if (!turns->execute_team_action(enemy_team)) {
		return;
	}

	for (long long i = registry.enemies.entities.size() - 1; i >= 0; --i) {
		const Entity& enemy_entity = registry.enemies.entities[i];

		if (remove_dead_entity(enemy_entity)) {
			continue;
		}

		const Enemy enemy = registry.enemies.components[i];

		ColorState active_world_color = turns->get_active_color();
		if (((uint8_t)active_world_color & (uint8_t)enemy.team) > 0) {

			switch (enemy.type) {
			case EnemyType::Slime:
				execute_Slime(enemy_entity);
				break;

			case EnemyType::Raven:
				execute_Raven(enemy_entity);
				break;

			case EnemyType::LivingArmor:
				execute_LivingArmor(enemy_entity);
				break;

			case EnemyType::TreeAnt:
				execute_TreeAnt(enemy_entity);
				break;

			default:
				throw std::runtime_error("Invalid enemy type.");
			}
		}
	}

	turns->complete_team_action(enemy_team);
}

void AISystem::execute_Slime(const Entity& slime)
{
	const Enemy& enemy = registry.enemies.get(slime);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_in_radius(slime, enemy.radius)) {
			switch_enemy_state(slime, enemy.type, EnemyState::Active);
		}
		break;

	case EnemyState::Active:
		if (is_player_in_radius(slime, enemy.radius * 2)) {
			if (is_player_in_attack_range(slime, enemy.attack_range)) {
				attack_player(slime);
			} else {
				approach_player(slime, enemy.speed);
			}

			if (is_afraid(slime)) {
				switch_enemy_state(slime, enemy.type, EnemyState::Flinched);
			}
		} else {
			switch_enemy_state(slime, enemy.type, EnemyState::Idle);
		}
		break;

	case EnemyState::Flinched:
		if (is_at_nest(slime)) {
			switch_enemy_state(slime, enemy.type, EnemyState::Idle);
		} else {
			approach_nest(slime, enemy.speed);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state of Slime.");
	}
}

void AISystem::execute_Raven(const Entity& raven) {
	const Enemy& enemy = registry.enemies.get(raven);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_in_radius(raven, enemy.radius)) {
			switch_enemy_state(raven, enemy.type, EnemyState::Active);
		}
		break;

	case EnemyState::Active:
		if (is_player_in_radius(raven, enemy.radius * 2)) {
			if (is_player_in_attack_range(raven, enemy.attack_range)) {
				attack_player(raven);
			} else {
				approach_player(raven, enemy.speed);
			}
		} else {
			if (is_at_nest(raven)) {
				switch_enemy_state(raven, enemy.type, EnemyState::Idle);
			} else {
				approach_nest(raven, enemy.speed);
			}
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state of Raven.");
	}
}

// TODO
void AISystem::execute_LivingArmor(const Entity& living_armor) { }

// TODO
void AISystem::execute_TreeAnt(const Entity& tree_ant) { }

bool AISystem::remove_dead_entity(const Entity& entity)
{
	// TODO: Animate death.
	if (registry.stats.has(entity) && registry.stats.get(entity).health <= 0) {
		registry.remove_all_components_of(entity);
		return true;
	}
	return false;
}

void AISystem::switch_enemy_state(const Entity& enemy_entity, EnemyType /*enemy_type*/, EnemyState enemy_state)
{
	registry.enemies.get(enemy_entity).state = enemy_state;

	// TODO: Animate enemy state switch by a spreadsheet for enemy_type * enemy_state mapping.
	// Slime:		Idle, Active, Flinched.
	// Raven:		Idle, Active.
	// LivingArmor:	Idle, Active, Powerup.
	// TreeAnt:		Idle, Active, Immortal.
	TEXTURE_ASSET_ID& enemy_current_textrue = registry.render_requests.get(enemy_entity).used_texture;
	switch (enemy_state) {

	case EnemyState::Idle:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME;
		break;

	case EnemyState::Active:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_ALERT;
		break;

	case EnemyState::Flinched:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_FLINCHED;
		break;

	case EnemyState::Powerup:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_FLINCHED;
		break;

	case EnemyState::Immortal:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_FLINCHED;
		break;

	default:
		throw std::runtime_error("Invalid enemy state.");
	}
}

bool AISystem::is_player_in_radius(const Entity& entity, const uint radius)
{
	uvec2 player_map_pos = registry.map_positions.get(registry.players.top_entity()).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	ivec2 distance = entity_map_pos - player_map_pos;
	return uint(abs(distance.x)) <= radius && uint(abs(distance.y)) <= radius;
}

bool AISystem::is_player_in_attack_range(const Entity& entity, const uint attack_range)
{
	return is_player_in_radius(entity, attack_range);
}

bool AISystem::is_afraid(const Entity& entity)
{
	const Stats& states = registry.stats.get(entity);
	return states.health < (states.health_max / 4);
}

bool AISystem::is_at_nest(const Entity& entity)
{
	const uvec2& entity_map_pos = registry.map_positions.get(entity).position;
	const uvec2& nest_map_pos = registry.enemies.get(entity).nest_map_pos;

	return entity_map_pos == nest_map_pos;
}

void AISystem::attack_player(const Entity& entity)
{
	// TODO: Animate attack.
	char* enemy_type = enemy_type_to_string[registry.enemies.get(entity).type];
	printf("%s_%u attacks player!\n", enemy_type, (uint)entity);

	Stats& entity_stats = registry.stats.get(entity);
	Stats& player_stats = registry.stats.get(registry.players.top_entity());
	combat->do_attack(entity_stats, entity_stats.base_attack, player_stats);
}

bool AISystem::approach_player(const Entity& entity, uint speed)
{
	uvec2 player_map_pos = registry.map_positions.get(registry.players.top_entity()).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, player_map_pos);
	if (shortest_path.size() > 2) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 2))];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::approach_nest(const Entity& entity, uint speed)
{
	const uvec2& entity_map_pos = registry.map_positions.get(entity).position;
	const uvec2& nest_map_pos = registry.enemies.get(entity).nest_map_pos;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, nest_map_pos);
	if (shortest_path.size() > 1) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 1))];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::move(const Entity& entity, const uvec2& map_pos)
{
	// TODO: Animate move.
	MapPosition& entity_map_pos = registry.map_positions.get(entity);
	if (entity_map_pos.position != map_pos && map_generator->walkable(map_pos)) {
		entity_map_pos.position = map_pos;
		return true;
	}
	return false;
}
