#pragma once

#include <random>
#include <vector>
#include <tuple>

#include "common.hpp"

#include "animation_system.hpp"
#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "turn_system.hpp"
#include "world_init.hpp"

#include "soloud_wav.h"

static void debug_log(std::string string) {
	// printf(string);
}

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
	void execute_dummy_sm(const Entity& entity);
	void execute_basic_sm(const Entity& entity);
	void execute_cowardly_sm(const Entity& entity);
	void execute_defensive_sm(const Entity& entity);
	void execute_aggressive_sm(const Entity& entity);

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

	// AOE attack.
	void release_aoe(const std::vector<Entity>& aoe);

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
	SoLoud::Wav king_mush_summon_wav;
	SoLoud::Wav king_mush_aoe_wav;

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
	enum class BTState {
		Running = 0,
		Success = 1,
		Failure = 2,
		Count = 3,
	};
	static constexpr std::array<const std::string_view, (size_t)BTState::Count> state_names = {
		"Running",
		"Success",
		"Failure",
	};

	// Base node: BTNode
	class BTNode {
	public:
		virtual ~BTNode() = default;

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

		void init(Entity /*e*/) override
		{
			debug_log("Debug: SummonEnemies.init\n");
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: SummonEnemies.process\n");

			ai->summon_enemies(e, m_type, m_num);

			ai->switch_enemy_state(e, EnemyState::Idle);
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
		explicit AOEAttack(std::vector<ivec2> area_pattern)
			: is_charged(false)
			, m_aoe_shape(std::move(area_pattern))
		{
		}

		void init(Entity /*e*/) override
		{
			debug_log("Debug: AOEAttack.init\n");
			is_charged = false;
			m_aoe.clear();
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: AOEAttack.process\n");

			if (!is_charged) {
				// Charging
				is_charged = true;

				// Compute AOE area using AOE shape and player position.
				Entity player = registry.view<Player>().front();
				const uvec2& player_map_pos = registry.get<MapPosition>(player).position;
				std::vector<uvec2> aoe_area;
				for (const ivec2& map_pos_offset : m_aoe_shape) {
					const ivec2& map_pos = map_pos_offset + static_cast<ivec2>(player_map_pos);
					if (map_pos.x >= 0 || map_pos.y >= 0) {
						aoe_area.emplace_back(map_pos);
					}
				}

				// Create AOE stats.
				Stats aoe_stats = registry.get<Stats>(e);
				aoe_stats.base_attack.damage_min *= 2;
				aoe_stats.base_attack.damage_max *= 2;
				aoe_stats.damage_bonus *= 2;

				Enemy enemy = registry.get<Enemy>(e);
				
				// Create AOE.
				m_aoe = create_aoe(aoe_area, aoe_stats, enemy.type);
				
				ai->switch_enemy_state(e, EnemyState::Charging);

				return handle_process_result(BTState::Running);
			}
			// Release AOE.
			ai->release_aoe(m_aoe);

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_event_animation(e, 4);
			ai->so_loud->play(ai->king_mush_aoe_wav);
			return handle_process_result(BTState::Success);
		}

	private:
		bool is_charged;
		std::vector<ivec2> m_aoe_shape;
		std::vector<Entity> m_aoe;
	};

	// Leaf action node: RegularAttack
	class RegularAttack : public BTNode {
	public:
		void init(Entity /*e*/) override { debug_log("Debug: RegularAttack.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: RegularAttack.process\n");

			ai->attack_player(e);

			ai->switch_enemy_state(e, EnemyState::Idle);

			// Gets information related to where the boss is attacking from a ranged. 
			// Can be moved into animation ssytem to make this portion clearer and free or registry accesses
			Entity player_entity = registry.view<Player>().front();
			uvec2 player_location = registry.get<MapPosition>(player_entity).position;
			EnemyType boss_enemy_type = registry.get<Enemy>(e).type;

			ai->animations->boss_ranged_attack(boss_enemy_type, player_location);

			return handle_process_result(BTState::Success);
		}
	};

	// Leaf action node: RecoverHealth
	class RecoverHealth : public BTNode {
	public:
		explicit RecoverHealth(float ratio)
			: m_ratio(ratio)
		{
		}

		void init(Entity /*e*/) override { debug_log("Debug: RecoverHealth.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: RecoverHealth.process\n");

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
		void init(Entity /*e*/) override { debug_log("Debug: DoNothing.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: DoNothing.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);

			return handle_process_result(BTState::Success);
		}
	};

	// Composite logic node: Selector
	class Selector : public BTNode {
	public:
		explicit Selector(std::unique_ptr<BTNode> default_child)
			: m_index(-1)
			, m_default_child(std::move(default_child))
		{
		}

		void add_precond_and_child(const std::function<bool(Entity)>& precond, std::unique_ptr<BTNode> child)
		{
			m_preconditions.push_back(precond);
			m_children.push_back(std::move(child));
		}

		void init(Entity e) override {
			debug_log("Debug: Selector.init\n");
			m_index = -1;
			for (const auto& child: m_children) {
				child->init(e);
			}
			m_default_child->init(e);
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: Selector.process\n");
			if (m_index == -1) {
				size_t i = 0;
				for (i = 0; i < m_preconditions.size(); ++i) {
					if (m_preconditions[i](e)) {
						m_index = static_cast<int>(i);
						return handle_process_result(m_children[i]->process(e, ai));
					}
				}
				m_index = static_cast<int>(i);
				return handle_process_result(m_default_child->process(e, ai));
			}

			if (m_index < m_preconditions.size()) {
				return handle_process_result(m_children[m_index]->process(e, ai));
			}

			return handle_process_result(m_default_child->process(e, ai));
		}

	private:
		int m_index;
		std::vector<std::function<bool(Entity)>> m_preconditions;
		std::vector<std::unique_ptr<BTNode>> m_children;
		std::unique_ptr<BTNode> m_default_child;
	};

	class SummonerTree : public BTNode {
	public:
		explicit SummonerTree(std::unique_ptr<BTNode> child)
			: m_child(std::move(child))
		{
		}

		void init(Entity e) override
		{
			debug_log("Debug: SummonerTree.init\n");
			m_child->init(e);
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("--------------------------------------------------\n");
			debug_log("Debug: SummonerTree.process\n");
			BTState state = m_child->process(e, ai);
			std::string message = "Debug: State after process = ";
			message += state_names.at((size_t)state);
			debug_log(message + "\n");
			return handle_process_result(state);
		}

		static std::unique_ptr<BTNode> summoner_tree_factory(AISystem* ai)
		{
			// Selector - active
			auto summon_enemies = std::make_unique<SummonEnemies>(EnemyType::Mushroom, 1);
			std::vector<ivec2> aoe_shape;
			aoe_shape.emplace_back(0, 0);
			aoe_shape.emplace_back(0, -1);
			aoe_shape.emplace_back(-1, 0);
			aoe_shape.emplace_back(1, 0);
			auto aoe_attack = std::make_unique<AOEAttack>(aoe_shape);
			auto regular_attack = std::make_unique<RegularAttack>();
			auto selector_active = std::make_unique<Selector>(std::move(regular_attack));
			Selector* p = selector_active.get();
			selector_active->add_precond_and_child(
				// Summoner summons enemies every fifth process during attack.
				[p](Entity /*e*/) { return (p->get_process_count() % 5 == 0) && (p->get_process_count() != 0); },
				std::move(summon_enemies));
			selector_active->add_precond_and_child(
				// Summoner has 20% chance to make an AOESquare attack with the following AOE shape.
				// ┌───┐
				// │ x │
				// │xPx|
				// |   |
				// └───┘
				[ai](Entity /*e*/) { return ai->chance_to_happen(0.20f); },
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

	class DragonTree : public BTNode {
	public:
		explicit DragonTree(std::unique_ptr<BTNode> child)
			: m_child(std::move(child))
		{
		}

		void init(Entity e) override
		{
			debug_log("Debug: DragonTree.init\n");
			m_child->init(e);
			ripple_attacks = {};
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("--------------------------------------------------\n");
			debug_log("Debug: DragonTree.process\n");
			BTState state = m_child->process(e, ai);
			std::string message = "Debug: State after process = ";
			message += state_names.at((size_t)state);
			debug_log(message + "\n");
			return handle_process_result(state);
		}

		static std::unique_ptr<BTNode> dragon_tree_factory(AISystem* ai)
		{
			// Selector - active
			auto summon_enemies = std::make_unique<SummonEnemies>(EnemyType::Mushroom, 1);
			std::vector<ivec2> aoe_shape;

			// Algorithm for shape
			// Why isn't this working??
			//for (auto& attack : ripple_attacks) {
				/* ivec2 center = attack.first;
				int dist = attack.second;

				for (int i = dist; i > -dist; i--) {
					aoe_shape.emplace_back(center + ivec2(dist, i));
					aoe_shape.emplace_back(center + ivec2(-dist, i));
				}
				for (int i = dist - 1; i > -dist + 1; i--) {
					aoe_shape.emplace_back(center + ivec2(i, dist));
					aoe_shape.emplace_back(center + ivec2(i, -dist));
				}*/
			//}

			auto aoe_attack = std::make_unique<AOEAttack>(aoe_shape);
			auto regular_attack = std::make_unique<RegularAttack>();
			auto selector_active = std::make_unique<Selector>(std::move(regular_attack));
			Selector* p = selector_active.get();
			selector_active->add_precond_and_child(
				[ai](Entity /*e*/) { return ai->chance_to_happen(1.0f); },
				std::move(aoe_attack));

			// Selector - idle
			auto recover_health = std::make_unique<RecoverHealth>(0.20f);
			auto do_nothing = std::make_unique<DoNothing>();
			auto selector_idle = std::make_unique<Selector>(std::move(do_nothing));
			selector_idle->add_precond_and_child(
				// Dragon recover 20% HP if its HP is not full during idle.
				[ai](Entity e) { return ai->is_health_below(e, 1.00f); },
				std::move(recover_health));

			// Selector - alive
			auto selector_alive = std::make_unique<Selector>(std::move(selector_idle));
			selector_alive->add_precond_and_child(
				// Summoner switch to attack if it spots the player during active.
				[ai](Entity e) { return ai->is_player_spotted(e); },
				std::move(selector_active));

			// Increase ripple size and clean up large ripples
			//for (std::pair)

			return std::make_unique<SummonerTree>(std::move(selector_alive));
		}

	private:
		std::unique_ptr<BTNode> m_child;
		static std::vector<std::pair<ivec2, int>> ripple_attacks;
	};

	// Boss entities and its corresponding behaviour trees.
	std::unordered_map<Entity, std::unique_ptr<BTNode>> bosses;
};
