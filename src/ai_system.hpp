#pragma once

#include <random>
#include <vector>

#include "common.hpp"

#include "animation_system.hpp"
#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "turn_system.hpp"

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

	// Check if the player is spotted by an entity within its radius.
	bool is_player_spotted(const Entity& entity, uint radius);

	// Check if the player is reachable by an entity within its attack range.
	bool is_player_in_attack_range(const Entity& entity, uint attack_range);

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

	// An entity become immortal if flag is true. Otherwise cancel immortality.
	void become_immortal(const Entity& entity, bool flag);

	// An entity become powerup if flag is true. Otherwise cancel powerup.
	void become_powerup(const Entity& entity, bool flag);

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
	// Declare behaviour tree classes as nested classes in AISystem. This allows behaviour tree have access to the
	// members of AISystem, while avoiding circuilar dependency.

	// The return type of behaviour tree processing.
	enum class BTState { Running, Success, Failure };

	// Base node: BTNode - representing any node in our behaviour tree.
	class BTNode {
	public:
		virtual void init(Entity e) {};

		virtual BTState process(Entity e, AISystem* ai) = 0;
	};

	// Leaf action node: DoNothing
	class DoNothing : public BTNode {
	public:
		void init(Entity e) override { printf("Debug: DoNothing.init\n"); }

		BTState process(Entity /*e*/, AISystem* ai) override
		{
			printf("Debug: DoNothing.process\n");
			return BTState::Success;
		}
	};

	// Leaf action node: RecoverHealth
	class RecoverHealth : public BTNode {
	public:
		void init(Entity e) override { printf("Debug: RecoverHealth.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			ai->recover_health(e, 0.2f);
			printf("Debug: RecoverHealth.process\n");
			return BTState::Success;
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
			if (m_index == -1) {
				size_t i;
				for (i = 0; i < m_preconditions.size(); ++i) {
					if (m_preconditions[i](e)) {
						m_index = i;
						return m_children[i]->process(e, ai);
					}
				}
				m_index = i;
				return m_default_child->process(e, ai);
			} else {
				if (m_index < m_preconditions.size()) {
					return m_children[m_index]->process(e, ai);
				} else {
					return m_default_child->process(e, ai);
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
		explicit SummonerTree(std::unique_ptr<BTNode> child)
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
			return state;
		}

		static std::unique_ptr<BTNode> summoner_tree_factory(AISystem* ai)
		{
			// Selector - attack
			auto do_nothing1 = std::make_unique<DoNothing>();

			// Selector - idle
			auto recover_health = std::make_unique<RecoverHealth>();
			auto do_nothing = std::make_unique<DoNothing>();
			auto selector_idle = std::make_unique<Selector>(std::move(do_nothing));
			selector_idle->add_precond_and_child([ai](Entity e) { return ai->is_health_below(e, 1.0f); },
												  std::move(recover_health));

			// Selector - active
			auto selector_active = std::make_unique<Selector>(std::move(selector_idle));
			selector_active->add_precond_and_child([ai](Entity e) { return ai->is_player_spotted(e, 5); },
												   std::move(do_nothing1));

			return std::make_unique<SummonerTree>(std::move(selector_active));
		}

	private:
		std::unique_ptr<BTNode> m_child;
	};

	// Boss entities and its corresponding behaviour trees.
	std::unordered_map<Entity, std::unique_ptr<BTNode>> bosses;
};
