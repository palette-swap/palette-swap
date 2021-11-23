// internal
#include "ai_system.hpp"
#include "components.hpp"
#include "world_init.hpp"

// AI logic
AISystem::AISystem(const Debug& debugging,
				   std::shared_ptr<CombatSystem> combat,
				   std::shared_ptr<MapGeneratorSystem> map_generator,
				   std::shared_ptr<TurnSystem> turns,
				   std::shared_ptr<AnimationSystem> animations,
				   std::shared_ptr<SoLoud::Soloud> so_loud)
	: debugging(debugging)
	, animations(std::move(animations))
	, combat(std::move(combat))
	, map_generator(std::move(map_generator))
	, turns(std::move(turns))
	, so_loud(std::move(so_loud))
	, enemy_team(registry.create())
	, rng(std::random_device()())
	, uniform_dist(0.00f, 1.00f)
{
	registry.emplace<DebugComponent>(enemy_team);

	this->turns->add_team_to_queue(enemy_team);

	enemy_attack1_wav.load(audio_path("enemy_attack1.wav").c_str());

	king_mush_summon_wav.load(audio_path("King Mush Shrooma.wav").c_str());
	king_mush_aoe_wav.load(audio_path("King Mush Fudun.wav").c_str());

	std::vector<std::function<void(const Entity& attacker, const Entity& target)>> callbacks;

	this->combat->on_attack(
		[this](const Entity& attacker, const Entity& target) { this->do_attack_callback(attacker, target); });

	this->combat->on_death([this](const Entity& entity) {
		// If entity is a boss, remove its behaviour tree and AOE squares.
		if (bosses.find(entity) != bosses.end()) {
			bosses.erase(entity);
			auto view = registry.view<AOESquare>();
			registry.destroy(view.begin(), view.end());
		}
	});
}

void AISystem::step(float /*elapsed_ms*/)
{
	if (turns->execute_team_action(enemy_team)) {

		// Released AOE squares are destroyed.
		for (auto [aoe_square_entity, aoe_square] : registry.view<AOESquare>().each()) {
			if (aoe_square.is_released) {
				registry.destroy(aoe_square_entity);
			}
		}

		for (auto [enemy_entity, enemy] : registry.view<Enemy>().each()) {

			ColorState active_world_color = turns->get_active_color();
			if (((uint8_t)active_world_color & (uint8_t)enemy.team) > 0) {

				if (Stunned* stunned = registry.try_get<Stunned>(enemy_entity)) {
					if (--(stunned->rounds) <= 0) {
						registry.erase<Stunned>(enemy_entity);
					}
					continue;
				}

				switch (enemy.behaviour) {

				// Small Enemy Behaviours (State Machines)
				case EnemyBehaviour::Dummy:
					execute_dummy_sm(enemy_entity);
					break;

				case EnemyBehaviour::Basic:
					execute_basic_sm(enemy_entity);
					break;

				case EnemyBehaviour::Cowardly:
					execute_cowardly_sm(enemy_entity);
					break;

				case EnemyBehaviour::Defensive:
					execute_defensive_sm(enemy_entity);
					break;

				case EnemyBehaviour::Aggressive:
					execute_aggressive_sm(enemy_entity);
					break;

				// Boss Enemy Behaviours (Behaviour Trees)
				case EnemyBehaviour::Summoner: {
					if ((bosses.find(enemy_entity) == bosses.end())) {
						// A boss entity occurs for the 1st time, create its corresponding behaviour tree & initialize.
						bosses[enemy_entity] = SummonerTree::summoner_tree_factory(this);
						bosses[enemy_entity]->init(enemy_entity);
					}
					if (bosses[enemy_entity]->process(enemy_entity, this) != BTState::Running) {
						bosses[enemy_entity]->init(enemy_entity);
					}
					break;
				}

				default:
					throw std::runtime_error("Invalid enemy behaviour.");
				}
			}
		}

		turns->complete_team_action(enemy_team);
	}

	// Debugging for the shortest paths of enemies.
	if (debugging.in_debug_mode) {
		draw_pathing_debug();
	}
}

void AISystem::do_attack_callback(const Entity& attacker, const Entity& target)
{
	if (registry.any_of<Player>(attacker)) {
		Enemy& enemy = registry.get<Enemy>(target);
		if (!is_player_spotted(target)) {
			enemy.radius = MapUtility::room_size * MapUtility::map_size;
		}
	}
}

void AISystem::execute_dummy_sm(const Entity& entity)
{
	Enemy& enemy = registry.get<Enemy>(entity);
	Stats& stats = registry.get<Stats>(entity);

	switch (enemy.state) {

	case EnemyState::Idle:
		for (size_t i = 0; i < static_cast<size_t>(DamageType::Count); ++i) {
			if (i != static_cast<size_t>(DamageType::Physical)) {
				stats.damage_modifiers[i] = INT_MIN;
			}
		}
		break;

	case EnemyState::Active:
		stats.damage_modifiers.at(static_cast<size_t>(DamageType::Physical)) = INT_MIN;
		break;

	default:
		throw std::runtime_error("Invalid enemy state for enemy behaviour Dummy.");
	}
}

void AISystem::execute_basic_sm(const Entity& entity)
{
	const Enemy& enemy = registry.get<Enemy>(entity);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(entity)) {
			switch_enemy_state(entity, EnemyState::Active);
			execute_basic_sm(entity);
			return;
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(entity)) {
			if (is_player_in_attack_range(entity)) {
				attack_player(entity);
			} else {
				approach_player(entity, enemy.speed);
			}
		} else {
			switch_enemy_state(entity, EnemyState::Idle);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state for enemy behaviour Basic.");
	}
}

void AISystem::execute_cowardly_sm(const Entity& entity)
{
	Enemy& enemy = registry.get<Enemy>(entity);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(entity)) {
			switch_enemy_state(entity, EnemyState::Active);
			execute_cowardly_sm(entity);
			return;
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(entity)) {
			if (is_player_in_attack_range(entity)) {
				attack_player(entity);
			} else {
				approach_player(entity, enemy.speed);
			}

			if (is_health_below(entity, 0.25f)) {
				switch_enemy_state(entity, EnemyState::Flinched);
			}
		} else {
			switch_enemy_state(entity, EnemyState::Idle);
		}
		break;

	case EnemyState::Flinched:
		if (is_at_nest(entity)) {
			enemy.radius = 3;
			if (!is_player_spotted(entity)) {
				recover_health(entity, 1.f);
				switch_enemy_state(entity, EnemyState::Idle);
			}
		} else {
			approach_nest(entity, enemy.speed);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state for enemy behaviour Cowardly.");
	}
}

void AISystem::execute_defensive_sm(const Entity& entity)
{
	const Enemy& enemy = registry.get<Enemy>(entity);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(entity)) {
			switch_enemy_state(entity, EnemyState::Active);
			execute_defensive_sm(entity);
			return;
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(entity)) {
			if (is_player_in_attack_range(entity)) {
				attack_player(entity);
			} else {
				approach_player(entity, enemy.speed);
			}

			if (chance_to_happen(0.2f)) {
				become_immortal(entity, true);
				switch_enemy_state(entity, EnemyState::Immortal);
			}
		} else {
			switch_enemy_state(entity, EnemyState::Idle);
		}
		break;

	case EnemyState::Immortal:
		become_immortal(entity, false);
		switch_enemy_state(entity, EnemyState::Active);
		break;

	default:
		throw std::runtime_error("Invalid enemy state for enemy behaviour Defensive.");
	}
}

void AISystem::execute_aggressive_sm(const Entity& entity)
{
	const Enemy& enemy = registry.get<Enemy>(entity);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(entity)) {
			switch_enemy_state(entity, EnemyState::Active);
			execute_aggressive_sm(entity);
			return;
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(entity)) {
			if (is_player_in_attack_range(entity)) {
				attack_player(entity);
			} else {
				approach_player(entity, enemy.speed);
			}

			if (is_health_below(entity, 0.20f)) {
				become_powerup(entity, true);
				switch_enemy_state(entity, EnemyState::Powerup);
			}
		} else {
			switch_enemy_state(entity, EnemyState::Idle);
		}
		break;

	case EnemyState::Powerup:
		if (is_player_spotted(entity)) {
			if (is_player_in_attack_range(entity)) {
				attack_player(entity);
			}
			// TreeAnt can't move when it's powered up.
		} else {
			become_powerup(entity, false);
			switch_enemy_state(entity, EnemyState::Active);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state for enemy behaviour Aggressive.");
	}
}

void AISystem::switch_enemy_state(const Entity& enemy_entity, EnemyState new_state)
{
	Enemy& enemy = registry.get<Enemy>(enemy_entity);
	enemy.state = new_state;
	int new_state_id = enemy_state_to_animation_state.at(static_cast<size_t>(new_state));
	animations->set_enemy_state(enemy_entity, new_state_id);
}

bool AISystem::is_player_spotted(const Entity& entity)
{
	const uint radius = registry.get<Enemy>(entity).radius;

	uvec2 player_map_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	uvec2 entity_map_pos = registry.get<MapPosition>(entity).position;

	ivec2 distance = entity_map_pos - player_map_pos;
	return uint(abs(distance.x)) <= radius && uint(abs(distance.y)) <= radius;
}

bool AISystem::is_player_in_attack_range(const Entity& entity)
{
	const uint attack_range = registry.get<Enemy>(entity).attack_range;

	uvec2 player_map_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	uvec2 entity_map_pos = registry.get<MapPosition>(entity).position;

	ivec2 distance = entity_map_pos - player_map_pos;
	return uint(abs(distance.x)) <= attack_range && uint(abs(distance.y)) <= attack_range;
}

bool AISystem::is_at_nest(const Entity& entity)
{
	const uvec2& entity_map_pos = registry.get<MapPosition>(entity).position;
	const uvec2& nest_map_pos = registry.get<Enemy>(entity).nest_map_pos;

	return entity_map_pos == nest_map_pos;
}

void AISystem::attack_player(const Entity& entity)
{
	Entity player = registry.view<Player>().front();
	combat->do_attack(entity, registry.get<Stats>(entity).base_attack, player);
	so_loud->play(enemy_attack1_wav);
}

bool AISystem::approach_player(const Entity& entity, uint speed)
{
	const uvec2 player_map_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	const uvec2 entity_map_pos = registry.get<MapPosition>(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity, entity_map_pos, player_map_pos);
	if (shortest_path.size() > 2) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 2))];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::approach_nest(const Entity& entity, uint speed)
{
	const uvec2 player_map_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	const uvec2& entity_map_pos = registry.get<MapPosition>(entity).position;
	const uvec2& nest_map_pos = registry.get<Enemy>(entity).nest_map_pos;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity, entity_map_pos, nest_map_pos);
	if (shortest_path.size() > 1) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 1))];
		// A special case that the player occupies the nest, so the entity won't move in (overlap).
		if (next_map_pos == nest_map_pos && nest_map_pos == player_map_pos) {
			return false;
		}
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::move(const Entity& entity, const uvec2& map_pos)
{
	MapPosition& entity_map_pos = registry.get<MapPosition>(entity);

	if (Immobilized* immobilized = registry.try_get<Immobilized>(entity)) {
		if (--(immobilized->rounds) <= 0) {
			registry.erase<Immobilized>(entity);
		}
		return false;
	}

	if (entity_map_pos.position != map_pos && map_generator->walkable(map_pos)) {
		if (map_pos.x < entity_map_pos.position.x) {
			animations->set_sprite_direction(entity, Sprite_Direction::SPRITE_LEFT);
		} else if (map_pos.x > entity_map_pos.position.x) {
			animations->set_sprite_direction(entity, Sprite_Direction::SPRITE_RIGHT);
		}
		animations->enemy_tile_transition(entity, entity_map_pos.position, map_pos);
		entity_map_pos.position = map_pos;
		return true;
	}
	return false;
}

void AISystem::recover_health(const Entity& entity, float ratio)
{
	Stats& stats = registry.get<Stats>(entity);
	stats.health += static_cast<int>(static_cast<float>(stats.health_max) * ratio);
	stats.health = min(stats.health, stats.health_max);
}

bool AISystem::is_health_below(const Entity& entity, float ratio)
{
	const Stats& stats = registry.get<Stats>(entity);
	return static_cast<float>(stats.health) < static_cast<float>(stats.health_max) * ratio;
}

bool AISystem::chance_to_happen(float percent) {
	float chance = uniform_dist(rng);
	bool result = chance < percent;
	return result;
}

void AISystem::become_immortal(const Entity& entity, bool flag)
{
	Stats& stats = registry.get<Stats>(entity);
	for (int& damage_modifier : stats.damage_modifiers) {
		damage_modifier = flag ? INT_MIN : 0;
	}
}

void AISystem::become_powerup(const Entity& entity, bool flag)
{
	Enemy& enemy = registry.get<Enemy>(entity);
	enemy.radius *= 2;
	enemy.attack_range *= 2;

	Stats& stats = registry.get<Stats>(entity);
	if (flag) {
		stats.to_hit_bonus *= 2;
		stats.base_attack.to_hit_min *= 2;
		stats.base_attack.to_hit_max *= 2;
		stats.damage_bonus *= 2;
		stats.base_attack.damage_min *= 2;
		stats.base_attack.damage_max *= 2;
	} else {
		stats.to_hit_bonus /= 2;
		stats.base_attack.to_hit_min /= 2;
		stats.base_attack.to_hit_max /= 2;
		stats.damage_bonus /= 2;
		stats.base_attack.damage_min /= 2;
		stats.base_attack.damage_max /= 2;
	}
}

void AISystem::summon_enemies(const Entity& entity, EnemyType enemy_type, int num)
{
	MapPosition& map_pos = registry.get<MapPosition>(entity);

	for (size_t i = 0; i < num; ++i) {
		uvec2 new_map_pos = { map_pos.position.x - 2 - i, map_pos.position.y};
		if (map_generator->walkable_and_free(entt::null, new_map_pos)) {
			create_enemy(turns->get_active_color(), enemy_type, new_map_pos);
		}
	}

	so_loud->play(king_mush_summon_wav);
}

void AISystem::release_aoe(const std::vector<Entity>& aoe)
{
	for (const Entity& aoe_square : aoe) {
		const vec2& aoe_square_world_pos = registry.get<WorldPosition>(aoe_square).position;
		const uvec2& aoe_square_map_pos = MapUtility::world_position_to_map_position(aoe_square_world_pos);
		Entity player = registry.view<Player>().front();
		const uvec2& player_map_pos = registry.get<MapPosition>(player).position;
		if (aoe_square_map_pos == player_map_pos) {
			attack_player(aoe_square);
		}

		animations->trigger_aoe_attack_animation(aoe_square);
		// Released AOE squares will be destroyed in the next turn.
		registry.get<AOESquare>(aoe_square).is_released = true;
	}
}

void AISystem::draw_pathing_debug()
{
	for (auto [enemy_entity, enemy, entity_map_position] : registry.view<Enemy, MapPosition>().each()) {

		ColorState active_world_color = turns->get_active_color();
		if (((uint8_t)active_world_color & (uint8_t)enemy.team) > 0) {
			const uvec2& entity_map_pos = entity_map_position.position;

			if (enemy.state == EnemyState::Flinched) {
				const uvec2& nest_map_pos = enemy.nest_map_pos;
				std::vector<uvec2> shortest_path
					= map_generator->shortest_path(enemy_entity, entity_map_pos, nest_map_pos);

				for (const uvec2& path_point : shortest_path) {
					create_path_point(MapUtility::map_position_to_world_position(path_point));
				}
			} else if (enemy.state == EnemyState::Active && is_player_spotted(enemy_entity)) {
				Entity player = registry.view<Player>().front();
				std::vector<uvec2> shortest_path = map_generator->shortest_path(
					enemy_entity, entity_map_pos, registry.get<MapPosition>(player).position);

				for (const uvec2& path_point : shortest_path) {
					create_path_point(MapUtility::map_position_to_world_position(path_point));
				}
			}
		}
	}
}
