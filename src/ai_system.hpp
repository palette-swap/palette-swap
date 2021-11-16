#pragma once

#include <random>
#include <vector>

#include "common.hpp"

#include "animation_system.hpp"
#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "turn_system.hpp"
#include "world_init.hpp"

#include "soloud_wav.h"

class AISystem {
public:
	AISystem(const Debug& debugging,
			 std::shared_ptr<CombatSystem> combat,
			 std::shared_ptr<MapGeneratorSystem> map_generator,
			 std::shared_ptr<TurnSystem> turns,
			 std::shared_ptr<AnimationSystem> animations,
			 std::shared_ptr<SoLoud::Soloud> so_loud);

	void step(float elapsed_ms);

	// Observer Pattern: a callback of CombatSystem::do_attack().
	// Maximize the target's radius if the player attacks but cannot be spotted.
	void do_attack_callback(const Entity& attacker, const Entity& target);

private:
	// Execute small enemy behaviours (state machines).
	void execute_basic_sm(const Entity& entity);
	void execute_cowardly_sm(const Entity& entity);
	void execute_defensive_sm(const Entity& entity);
	void execute_aggressive_sm(const Entity& entity);

	// Remove an entity if it is dead.
	bool remove_dead_entity(const Entity& entity);

	// Switch enemy state.
	void switch_enemy_state(const Entity& enemy_entity, EnemyState new_state);

	// Check if the player is spotted by an entity.
	bool is_player_spotted(const Entity& entity);

	// Check if the player is in the attack range of an entity.
	bool is_player_in_attack_range(const Entity& entity);

	// Check if an entity is at its nest.
	bool is_at_nest(const Entity& entity);

	// An entity attackes the player.
	void attack_player(const Entity& entity);

	// An entity approaches the player.
	bool approach_player(const Entity& entity, uint speed);

	// An entity approaches its nest.
	bool approach_nest(const Entity& entity, uint speed);

	// An entity moves to a targeted map position.
	bool move(const Entity& entity, const uvec2& map_pos);

	void recover_health(const Entity& entity, float ratio);

	// Check if an entity's health is below a ratio.
	bool is_health_below(const Entity& entity, float ratio);

	// Return true if a percent chance happens.
	bool chance_to_happen(float percent);

	// An entity become immortal if flag is true. Otherwise cancel immortality.
	void become_immortal(const Entity& entity, bool flag);

	// An entity become powerup if flag is true. Otherwise cancel powerup.
	void become_powerup(const Entity& entity, bool flag);

	// An entity summons new enemies with a certain type and number.
	void summon_enemies(const Entity& entity, EnemyType enemy_type, int num);

	// AOE attack on an area.
	void aoe_attack(const Entity& entity, const std::vector<uvec2>& area);

	// Debugging
	const Debug& debugging;
	void draw_pathing_debug();

	// Related Systems
	std::shared_ptr<AnimationSystem> animations;
	std::shared_ptr<CombatSystem> combat;
	std::shared_ptr<MapGeneratorSystem> map_generator;
	std::shared_ptr<TurnSystem> turns;

	// Sound stuff
	std::shared_ptr<SoLoud::Soloud> so_loud;
	SoLoud::Wav enemy_attack1_wav;

	// Entity representing the enemy team's turn.
	Entity enemy_team;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1



	//---------------------------------------------------------------------------
	//-------------------------		  Nested Classes     ------------------------
	//---------------------------------------------------------------------------
	// Declare behaviour tree classes as nested classes in AISystem.
	// This allows behaviour tree have access to the members of AISystem, while avoiding circuilar dependency.

	// The return type of behaviour tree processing.
	enum class BTState { Running, Success, Failure };

	// Base node: BTNode
	class BTNode {
	public:
		virtual void init(Entity e) {};

		virtual BTState process(Entity e, AISystem* ai) = 0;

		virtual BTState handle_process_result(BTState state)
		{
			process_count += state != BTState::Running ? 1 : 0;
			return state;
		}

		virtual size_t get_process_count() { return process_count; }

	private:
		// Each node counts its own processing times.
		size_t process_count = 0;
	};

	// Leaf action node: SummonEnemies
	class SummonEnemies : public BTNode {
	public:
		SummonEnemies(EnemyType type, int num)
			: m_type(type)
			, m_num(num)
		{
		}

		void init(Entity e) override { printf("Debug: SummonEnemies.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			printf("Debug: SummonEnemies.process\n");
			ai->summon_enemies(e, m_type, m_num);

			// TODO (Evan): active(1) + temp-summon.
			ai->switch_enemy_state(e, EnemyState::Idle);
			// TODO (Evan): Change to not be hardcoded, currently aware that 2 is the specific boss's summoning state
			ai->animations->boss_event_animation(e, 2);

			return handle_process_result(BTState::Success);
		}

	private:
		EnemyType m_type;
		int m_num;
	};

	
	// Leaf action node: AOEAttack
	class AOEAttack : public BTNode {
	public:
		AOEAttack(const std::vector<ivec2>& area_pattern)
			: m_area_pattern(area_pattern)
			, m_isCharged(false)
		{
		}

		void init(Entity e) override
		{
			printf("Debug: AOEAttack.init\n");
			m_isCharged = false;
			m_aiming_area.clear();
		}

		BTState process(Entity e, AISystem* ai) override
		{
			printf("Debug: AOEAttack.process\n");
			if (!m_isCharged) {
				// Charging
				m_isCharged = true;
				// Aiming area
				Entity player = registry.view<Player>().front();
				const uvec2& player_map_pos = registry.get<MapPosition>(player).position;
				for (const ivec2& pattern_pos : m_area_pattern) {
					ivec2 map_pos = pattern_pos + static_cast<ivec2>(player_map_pos);
					if (map_pos.x >= 0 || map_pos.y >= 0) {
						m_aiming_area.push_back(map_pos);
					}
				}

				// Debug
				for (const uvec2& aiming_point : m_aiming_area) {
					create_path_point(MapUtility::map_position_to_world_position(aiming_point));
				}
				Entity aoe_attack = create_aoe_target_squares();
				/*registry.emplace<AOEAttackActive>(e, aoe_attack);*/
				// TODO (Evan): charge(2).
				ai->switch_enemy_state(e, EnemyState::Charging);
				// Need animation to visualize m_aiming_area, like the Debug above.

				return handle_process_result(BTState::Running);
			} else {
				// AOE attack.
				ai->aoe_attack(e, m_aiming_area);

				// TODO (Evan): active(1) + temp-aoeAttacks.
				ai->switch_enemy_state(e, EnemyState::Idle);
				ai->animations->boss_event_animation(e, 4);
				// ai->animations->aoe_animation(e, m_aiming_area);

				return handle_process_result(BTState::Success);
			}
		}

	private:
		std::vector<ivec2> m_area_pattern;
		bool m_isCharged;
		std::vector<uvec2> m_aiming_area;
	};

	// Leaf action node: RegularAttack
	class RegularAttack : public BTNode {
	public:
		void init(Entity e) override { printf("Debug: RegularAttack.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			printf("Debug: RegularAttack.process\n");
			ai->attack_player(e);

			// TODO (Evan): active(1) + temp-attack.
			ai->switch_enemy_state(e, EnemyState::Idle);

			return handle_process_result(BTState::Success);
		}
	};

	// Leaf action node: RecoverHealth
	class RecoverHealth : public BTNode {
	public:
		RecoverHealth(float ratio)
			: m_ratio(ratio)
		{
		}

		void init(Entity e) override { printf("Debug: RecoverHealth.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			printf("Debug: RecoverHealth.process\n");
			ai->recover_health(e, m_ratio);
			ai->switch_enemy_state(e, EnemyState::Idle);
			return handle_process_result(BTState::Success);
		}

	private:
		float m_ratio;
	};

	// Leaf action node: DoNothing
	class DoNothing : public BTNode {
	public:
		void init(Entity e) override { printf("Debug: DoNothing.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			printf("Debug: DoNothing.process\n");
			ai->switch_enemy_state(e, EnemyState::Idle);
			return handle_process_result(BTState::Success);
		}
	};

	// Composite logic node: Selector
	class Selector : public BTNode {
	public:
		Selector(std::unique_ptr<BTNode> default_child)
			: m_index(-1)
			, m_default_child(std::move(default_child))
		{
		}

		void add_precond_and_child(std::function<bool(Entity)> precond, std::unique_ptr<BTNode> child)
		{
			m_preconditions.push_back(precond);
			m_children.push_back(std::move(child));
		}

		void init(Entity e) override {
			printf("Debug: Selector.init\n");
			m_index = -1;
			for (const auto& child: m_children) {
				child->init(e);
			}
			m_default_child->init(e);
		}

		BTState process(Entity e, AISystem* ai) override
		{
			printf("Debug: Selector.process\n");
			BTState state;
			if (m_index == -1) {
				size_t i;
				for (i = 0; i < m_preconditions.size(); ++i) {
					if (m_preconditions[i](e)) {
						m_index = i;
						return handle_process_result(m_children[i]->process(e, ai));
					}
				}
				m_index = i;
				return handle_process_result(m_default_child->process(e, ai));
			} else {
				if (m_index < m_preconditions.size()) {
					return handle_process_result(m_children[m_index]->process(e, ai));
				} else {
					return handle_process_result(m_default_child->process(e, ai));
				}
			}
		}

	private:
		int m_index;
		std::vector<std::function<bool(Entity)>> m_preconditions;
		std::vector<std::unique_ptr<BTNode>> m_children;
		std::unique_ptr<BTNode> m_default_child;
	};

	class SummonerTree : public BTNode {
	public:
		SummonerTree(std::unique_ptr<BTNode> child)
			: m_child(std::move(child))
		{
		}

		void init(Entity e) override
		{
			printf("Debug: SummonerTree.init\n");
			m_child->init(e);
		}

		BTState process(Entity e, AISystem* ai) override
		{
			printf("--------------------------------------------------\n");
			printf("Debug: SummonerTree.process\n");
			BTState state = m_child->process(e, ai);
			printf("Debug: State after process = %s\n",
				   state == BTState::Running	   ? "Running"
					   : state == BTState::Success ? "Success"
												   : "Failure");
			return handle_process_result(state);
		}

		static std::unique_ptr<BTNode> summoner_tree_factory(AISystem* ai)
		{
			// Selector - active
			auto summon_enemies = std::make_unique<SummonEnemies>(EnemyType::Mushroom, 1);
			std::vector<ivec2> area_pattern;
			area_pattern.push_back(ivec2(0, 0));
			area_pattern.push_back(ivec2(0, -1));
			area_pattern.push_back(ivec2(-1, 0));
			area_pattern.push_back(ivec2(1, 0));
			auto aoe_attack = std::make_unique<AOEAttack>(area_pattern);
			auto regular_attack = std::make_unique<RegularAttack>();
			auto selector_active = std::make_unique<Selector>(std::move(regular_attack));
			Selector* p = selector_active.get();
			selector_active->add_precond_and_child(
				// Summoner summons enemies every fifth process during attack.
				[p](Entity e) { return (p->get_process_count() % 5 == 0) && (p->get_process_count() != 0); },
				std::move(summon_enemies));
			selector_active->add_precond_and_child(
				// Summoner has 20% chance to make an AOE attack with the following area pattern.
				// ┌───┐
				// │ x │
				// │xPx|
				// |   |
				// └───┘
				[ai](Entity e) { return ai->chance_to_happen(0.20f); },
				std::move(aoe_attack));

			// Selector - idle
			auto recover_health = std::make_unique<RecoverHealth>(0.20f);
			auto do_nothing = std::make_unique<DoNothing>();
			auto selector_idle = std::make_unique<Selector>(std::move(do_nothing));
			selector_idle->add_precond_and_child(
				// Summoner recover 20% HP if its HP is not full during idle.
				[ai](Entity e) { return ai->is_health_below(e, 1.00f); },
				std::move(recover_health));

			// Selector - alive
			auto selector_alive = std::make_unique<Selector>(std::move(selector_idle));
			selector_alive->add_precond_and_child(
				// Summoner switch to attack if it spots the player during active.
				[ai](Entity e) { return ai->is_player_spotted(e); },
				std::move(selector_active));

			return std::make_unique<SummonerTree>(std::move(selector_alive));
		}

	private:
		std::unique_ptr<BTNode> m_child;
	};

	// Boss entities and its corresponding behaviour trees.
	std::unordered_map<Entity, std::unique_ptr<BTNode>> bosses;
};
