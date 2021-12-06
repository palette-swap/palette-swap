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

static void debug_log(std::string str) {
	 printf(str.c_str());
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
	void execute_sacrificed_sm(const Entity& entity);

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

	// An entity is added with an attack effect.
	void add_attack_effect(const Entity& entity, Effect effect, float chance, int magnitude);

	// An entity is cleared with all attack effects.
	void clear_attack_effects(const Entity& entity);

	// An entity summons new enemies with a certain type and number.
	void summon_enemies(const Entity& entity, EnemyType enemy_type, int num);

	// An entity summons victims for sacrifice later.
	std::vector<Entity> summon_victims(const Entity& entity, EnemyType enemy_type, int num);

	// AOE attack.
	void release_aoe(const std::vector<Entity>& aoe, int attack_state);

	// Return line of tiles from a to b
	static std::vector<ivec2> draw_tile_line(ivec2 a, ivec2 b, int offset = 0);

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
			process_count += (state != BTState::Running) ? 1 : 0;
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
		SummonEnemies(int animation, std::string summon_sound, EnemyType type, int num)
			: m_type(type)
			, m_num(num)
			, m_animation(animation)

		{
			summon_effect.load(audio_path(summon_sound).c_str());
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
			ai->so_loud->play(summon_effect);
			ai->animations->boss_event_animation(e, m_animation);
			return handle_process_result(BTState::Success);
		}

	private:
		EnemyType m_type;
		int m_num;
		int m_animation;
		SoLoud::Wav summon_effect;
	};

	// Leaf action node: SummonVictims
	class SummonVictims : public BTNode {
	public:
		SummonVictims(int animation, std::string summon_sound, EnemyType type, int num, std::vector<uvec2> altars)
			: m_type(type)
			, m_num(num)
			, m_animation(animation)
			, m_altars(altars)

		{
			summon_effect.load(audio_path(summon_sound).c_str());
		}

		void init(Entity /*e*/) override { debug_log("Debug: SummonVictims.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: SummonVictims.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_event_animation(e, m_animation);
			ai->so_loud->play(summon_effect);

			Dragon& dragon = registry.get<Dragon>(e);
			dragon.is_sacrifice_used = true;
			dragon.victims = ai->summon_victims(e, m_type, m_num);
			assert(dragon.victims.size() <= m_altars.size());
			for (size_t i = 0; i < dragon.victims.size(); ++i) {
				Enemy& victim_enemy = registry.get<Enemy>(dragon.victims[i]);
				victim_enemy.behaviour = EnemyBehaviour::Sacrificed;
				victim_enemy.nest_map_pos = m_altars[i];
				registry.emplace<Victim>(dragon.victims[i], e);
			}

			return handle_process_result(BTState::Success);
		}

	private:
		EnemyType m_type;
		int m_num;
		std::vector<uvec2> m_altars;
		int m_animation;
		SoLoud::Wav summon_effect;
	};

	// Leaf action node: SacrificeVictims
	class SacrificeVictims : public BTNode {
	public:
		SacrificeVictims(int animation, std::string sound, float recover_ratio)
			: m_animation(animation)
			, m_recover_ratio(recover_ratio)

		{
			m_sound.load(audio_path(sound).c_str());
		}

		void init(Entity /*e*/) override { debug_log("Debug: SacrificeVictims.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: SacrificeVictims.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_event_animation(e, m_animation);
			ai->so_loud->play(m_sound);

			// Sacrifice all victims of dragon to recover HP.
			Dragon& dragon = registry.get<Dragon>(e);
			for (Entity victim : dragon.victims) {
				registry.destroy(victim);
				m_recover_ratio += 0.25f;
			}
			dragon.victims.clear();
			ai->recover_health(e, m_recover_ratio);

			return handle_process_result(BTState::Success);
		}

	private:
		int m_animation;
		SoLoud::Wav m_sound;
		float m_recover_ratio;
	};

	// Leaf action node: AOEAttack
	class AOEAttack : public BTNode {
	public:
		explicit AOEAttack(std::vector<ivec2> area_pattern, std::string aoe_sound, int aoe_attack_state, Entity target)
			: is_charged(false)
			, aoe_shape(std::move(area_pattern))
			, aoe_attack_state(aoe_attack_state)
			, target(target)
		{
			aoe_effect.load(audio_path(aoe_sound).c_str());
		}

		void init(Entity /*e*/) override
		{
			debug_log("Debug: AOEAttack.init\n");
			is_charged = false;
			aoe_entities.clear();
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: AOEAttack.process\n");

			if (!is_charged) {
				// Charging
				is_charged = true;

				// Compute AOE area using AOE shape and player position.
				MapPosition* mp = registry.try_get<MapPosition>(target);
				const uvec2& target_map_pos = (mp != nullptr ? mp->position : uvec2(0));

				std::vector<uvec2> aoe_area;
				for (const ivec2& map_pos_offset : aoe_shape) {
					const ivec2& map_pos = map_pos_offset + ivec2(target_map_pos);
					if (map_pos.x >= 0 || map_pos.y >= 0) {
						aoe_area.emplace_back(map_pos);
					}
				}

				// Create AOE stats.
				return handle_process_result(prepare_aoe(e, ai, aoe_area));
			}
			// Release AOE.
			return handle_process_result(release_aoe(e, ai));
		}

		BTState prepare_aoe(Entity e, AISystem* ai, std::vector<uvec2> aoe_area) {
			Stats& aoe_stats = registry.get<Stats>(e);
			aoe_stats.base_attack.damage_min *= 2;
			aoe_stats.base_attack.damage_max *= 2;
			aoe_stats.damage_bonus *= 2;

			Enemy& enemy = registry.get<Enemy>(e);

			// Create AOE.
			aoe_entities = create_aoe(aoe_area, aoe_stats, enemy.type, e);

			ai->switch_enemy_state(e, EnemyState::Charging);

			return BTState::Running;
		}

		BTState release_aoe(Entity e, AISystem* ai)
		{
			// Release AOE.
			ai->release_aoe(aoe_entities, aoe_attack_state);

			ai->switch_enemy_state(e, EnemyState::Idle);
			if (registry.try_get<Animation>(e)) {
				ai->animations->boss_event_animation(e, aoe_attack_state);
			}
			ai->so_loud->play(aoe_effect);
			return BTState::Success;
		}

	protected:
		bool is_charged;
		std::vector<ivec2> aoe_shape;
		std::vector<Entity> aoe_entities;
		int aoe_attack_state;
		SoLoud::Wav aoe_effect;
		Entity target;
	};

	// Leaf action node: AOEAttack
	class AOERandomAttack : public AOEAttack {
	public:
		explicit AOERandomAttack(
			std::string aoe_sound, int aoe_attack_state, Entity target, int num_attacks, int radius)
			: AOEAttack({}, aoe_sound, aoe_attack_state, target)
			, num_attacks(num_attacks)
			, radius(radius)
			, attack_state(aoe_attack_state)
		{
			dist = std::uniform_int_distribution<int>(-radius, radius);
		}

		void init(Entity /*e*/) override
		{
			debug_log("Debug: AOERandomAttack.init\n");
			is_charged = false;
			aoe_entities.clear();
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: AOEAttack.process\n");

			if (!is_charged) {
				// Charging
				is_charged = true;

				// Compute AOE area using AOE shape and player position.
				MapPosition* mp = registry.try_get<MapPosition>(target);
				const uvec2& target_map_pos = (mp != nullptr ? mp->position : uvec2(0, 0));

				std::vector<ivec2> attack_points;
				int retries = 3; 
				vec2 dragon_pos = vec2(registry.get<MapPosition>(e).position);

				for (int i = 0; i < num_attacks; i++) {
					ivec2 attack_i;
					attack_i.x = dist(ai->rng);
					attack_i.y = dist(ai->rng);

					// Check if point is more than 3 tiles from other points and the dragon
					bool too_close = false;
					if (distance2(vec2(attack_i), dragon_pos) < 9.0f) {
						if (retries > 0) {
							i--;
							retries--;
						}
						continue;
					}
					for (vec2 point : attack_points) {
						if (distance2(vec2(attack_i), point) < 9.0f) {
							too_close = true;
							break;
						}
					}
					if (too_close) {
						if (retries > 0) {
							i--;
							retries--;
						}
						continue;
					}
					attack_points.emplace_back(attack_i);
				}

				std::vector<uvec2> aoe_area;
				for (ivec2 point : attack_points) {
					for (const ivec2& map_pos_offset : aoe_shape) {
						const ivec2& map_pos = point + map_pos_offset + ivec2(target_map_pos);
						if (map_pos.x >= 0 || map_pos.y >= 0) {
							aoe_area.emplace_back(map_pos);
						}
					}
				}
				// Create AOE stats.
				return handle_process_result(prepare_aoe(e, ai, aoe_area));
			}
			// Release AOE.
			return handle_process_result(release_aoe(e, ai));
		}

	private:
		int num_attacks;
		int radius;
		std::uniform_int_distribution<int> dist;
		const std::vector<ivec2> aoe_shape = { { 0, 0 }, { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
		int attack_state;
		// All attacks are in a + pattern
		// ┌───┐
		// │ x │
		// │xxx|
		// | x |
		// └───┘
	};

	// Leaf action node: AOEAttack
	class AOEConeAttack : public AOEAttack {
	public:
		explicit AOEConeAttack(std::string aoe_sound, int aoe_attack_state, Entity attacker, Entity target)
			: AOEAttack({}, aoe_sound, aoe_attack_state, target)
			, attacker(attacker)
			, min_length(15)
		{
		}

		void init(Entity /*e*/) override
		{
			debug_log("Debug: AOEAttack.init\n");
			is_charged = false;
			aoe_entities.clear();
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: AOEConeAttack.process\n");

			if (!is_charged) {
				// Charging
				is_charged = true;

				MapPosition* ap = registry.try_get<MapPosition>(attacker);
				MapPosition* mp = registry.try_get<MapPosition>(target);
				const uvec2& target_map_pos = (mp != nullptr ? mp->position : uvec2(0, 0));
				const uvec2& attacker_map_pos = (ap != nullptr ? ap->position : uvec2(0, 0));

				// Transforming the beam
				ivec2 diff = static_cast<ivec2>(target_map_pos) - static_cast<ivec2>(attacker_map_pos);
				float length = distance(vec2(attacker_map_pos), vec2(target_map_pos));
				int multiplier = ceil(min_length / length);

				auto rotate_vector = [](ivec2 vector, float angle_r) { 
					float x = cos(angle_r) * vector.x - sin(angle_r) * vector.y;
					float y = sin(angle_r) * vector.x + cos(angle_r) * vector.y;
					return ivec2(round(x), round(y));
				};

				ivec2 attack_source_offset = -ivec2(diff.x * 4 / length, diff.y * 4 / length);
				ivec2 attack_source = static_cast<ivec2>(attacker_map_pos) + attack_source_offset;

				ivec2 overshot_target = attack_source + diff * multiplier;
				ivec2 overshot_target_L = attack_source + rotate_vector(diff * multiplier, glm::pi<float>() / 12);
				ivec2 overshot_target_R = attack_source + rotate_vector(diff * multiplier, -glm::pi<float>() / 12);

				int extra_squares = max(abs(attack_source_offset.x), abs(attack_source_offset.y)) + 1;
				std::vector<ivec2> main_line = draw_tile_line(attack_source, overshot_target, extra_squares);
				std::vector<ivec2> left_line = draw_tile_line(attack_source, overshot_target_L, extra_squares);
				std::vector<ivec2> right_line = draw_tile_line(attack_source, overshot_target_R, extra_squares);
				aoe_shape.insert(aoe_shape.end(), main_line.begin(), main_line.end());
				aoe_shape.insert(aoe_shape.end(), left_line.begin(), left_line.end());
				aoe_shape.insert(aoe_shape.end(), right_line.begin(), right_line.end());

				std::vector<uvec2> aoe_area;
				for (const ivec2& map_pos_offset : aoe_shape) {
					const ivec2& map_pos = map_pos_offset;
					if (map_pos.x >= 0 || map_pos.y >= 0) {
						aoe_area.emplace_back(map_pos);
					}
				}

				// Create AOE stats.
				return handle_process_result(prepare_aoe(e, ai, aoe_area));
			}
			aoe_shape.clear();
			return handle_process_result(release_aoe(e, ai));
		}

	private:
		Entity attacker;
		int min_length;
	};

	// Leaf action node: AOERingAttack
	class AOERingAttack : public AOEAttack {
	public:
		explicit AOERingAttack(std::string aoe_sound, int aoe_attack_state, int max_radius, Entity target)
			: AOEAttack({}, aoe_sound, aoe_attack_state, target)
			, max_radius(max_radius)
			, attack_state(aoe_attack_state)
		{
			radius = 3;
		}

		void init(Entity /*e*/) override
		{
			debug_log("Debug: AOEAttack.init\n");
			is_charged = false;
			aoe_entities.clear();
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: AOERingAttack.process\n");

			while (radius < max_radius) {
				if (!is_charged) {
					// Charging
					is_charged = true;

					// Compute AOE area using AOE shape and player position.
					MapPosition* mp = registry.try_get<MapPosition>(target);
					const uvec2& target_map_pos = (mp != nullptr ? mp->position : uvec2(0, 0));

					std::vector<uvec2> aoe_area;

					for (int i = -radius; i <= radius; i++) {
						const ivec2& map_pos_N = ivec2(i, -radius) + static_cast<ivec2>(target_map_pos);
						if (map_pos_N.x >= 0 || map_pos_N.y >= 0) {
							aoe_area.emplace_back(map_pos_N);
						}
						const ivec2& map_pos_S = ivec2(i, radius) + static_cast<ivec2>(target_map_pos);
						if (map_pos_S.x >= 0 || map_pos_S.y >= 0) {
							aoe_area.emplace_back(map_pos_S);
						}
					}
					for (int i = -radius + 1; i <= radius - 1; i++) {
						const ivec2& map_pos_W = ivec2(-radius, i) + static_cast<ivec2>(target_map_pos);
						if (map_pos_W.x >= 0 || map_pos_W.y >= 0) {
							aoe_area.emplace_back(map_pos_W);
						}
						const ivec2& map_pos_E = ivec2(radius, i) + static_cast<ivec2>(target_map_pos);
						if (map_pos_E.x >= 0 || map_pos_E.y >= 0) {
							aoe_area.emplace_back(map_pos_E);
						}
					}

					// Create AOE stats.
					return handle_process_result(prepare_aoe(e, ai, aoe_area));
				}
				// Release AOE.
				release_aoe(e, ai);

				is_charged = false;
				radius++;
			}

			return handle_process_result(BTState::Success);
		}

	protected:
		int radius;
		int max_radius;
		int attack_state;
	};

	// Leaf action node: RegularAttack
	class RegularAttack : public BTNode {
	public:
		RegularAttack(int animation)
			: m_animation(animation)
		{
		}
		void init(Entity /*e*/) override { debug_log("Debug: RegularAttack.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: RegularAttack.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_special_attack_animation(e, m_animation);

			ai->attack_player(e);

			return handle_process_result(BTState::Success);
		}

	private:
		int m_animation;
	};

	// Leaf action node: FireAttack (high damage)
	class FireAttack : public BTNode {
	public:
		FireAttack(int animation)
			: m_animation(animation)
		{
		}
		void init(Entity /*e*/) override { debug_log("Debug: FireAttack.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: FireAttack.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_special_attack_animation(e, m_animation);

			ai->become_powerup(e, true);
			ai->attack_player(e);
			ai->become_powerup(e, false);

			return handle_process_result(BTState::Success);
		}

	private:
		int m_animation;
	};

	// Leaf action node: IceAttack (stun effect)
	class IceAttack : public BTNode {
	public:
		IceAttack(int animation, float chance, int magnitude)
			: m_animation(animation)
			, m_chance(chance)
			, m_magnitude(magnitude)
		{
		}

		void init(Entity /*e*/) override { debug_log("Debug: IceAttack.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: IceAttack.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_special_attack_animation(e, m_animation);

			ai->add_attack_effect(e, Effect::Stun, m_chance, m_magnitude);
			ai->attack_player(e);
			ai->clear_attack_effects(e);

			return handle_process_result(BTState::Success);
		}

	private:
		int m_animation;
		float m_chance;
		int m_magnitude;
	};

	// Leaf action node: GaleAttack (shove effect)
	class GaleAttack : public BTNode {
	public:
		GaleAttack(int animation, float chance, int magnitude)
			: m_animation(animation)
			, m_chance(chance)
			, m_magnitude(magnitude)
		{
		}

		void init(Entity /*e*/) override { debug_log("Debug: GaleAttack.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: GaleAttack.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_special_attack_animation(e, m_animation);

			ai->add_attack_effect(e, Effect::Shove, m_chance, m_magnitude);
			ai->attack_player(e);
			ai->clear_attack_effects(e);

			return handle_process_result(BTState::Success);
		}

	private:
		int m_animation;
		float m_chance;
		int m_magnitude;
	};

	// Leaf action node: TarAttack (immobilize effect and evasion down effect)
	class TarAttack : public BTNode {
	public:
		TarAttack(int animation, float chance, int magnitude)
			: m_animation(animation) 
			, m_chance(chance)
			, m_magnitude(magnitude)
		{
		}

		void init(Entity /*e*/) override { debug_log("Debug: TarAttack.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: TarAttack.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_special_attack_animation(e, m_animation);

			ai->add_attack_effect(e, Effect::Immobilize, m_chance, m_magnitude);
			ai->add_attack_effect(e, Effect::EvasionDown, m_chance, m_magnitude);
			ai->attack_player(e);
			ai->clear_attack_effects(e);

			return handle_process_result(BTState::Success);
		}

	private:
		int m_animation;
		float m_chance;
		int m_magnitude;
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
		explicit DoNothing(int aoe_attack_state = 0) : aoe_attack_state(aoe_attack_state)
		{
		}

		void init(Entity /*e*/) override { debug_log("Debug: DoNothing.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: DoNothing.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			ai->animations->boss_event_animation(e, aoe_attack_state);

			return handle_process_result(BTState::Success);
		}

	protected:
		int aoe_attack_state;
	};

	// Leaf action node: SelfDestruct
	class SelfDestruct : public BTNode {
	public:
		void init(Entity /*e*/) override { debug_log("Debug: SelfDestruct.init\n"); }

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: SelfDestruct.process\n");

			ai->switch_enemy_state(e, EnemyState::Idle);
			registry.destroy(e);

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

		void init(Entity e) override
		{
			debug_log("Debug: Selector.init\n");
			m_index = -1;
			for (const auto& child : m_children) {
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

	// Composite logic node: Sequence
	class Sequence : public BTNode {

	public:
		Sequence()
			: m_index(0)
		{
		}

		void add_child(std::unique_ptr<BTNode> child)
		{
			m_children.push_back(std::move(child));
		}

		void init(Entity e) override
		{
			debug_log("Debug: Sequence.init\n");

			// Trivial case.
			if (m_children.empty()) {
				return;
			}

			m_index = 0;
			// Initialize the first child.
			const auto& child = m_children[m_index];
			child->init(e);
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("Debug: Sequence.process\n");

			// Trivial case.
			if (m_children.empty()) {
				return handle_process_result(BTState::Success);
			}

			// Process current child.
			BTState state = m_children[m_index]->process(e, ai);

			// Select a new active child and initialize its internal state.
			if (state == BTState::Success) {
				++m_index;
				if (m_index < m_children.size()) {
					m_children[m_index]->init(e);
					return handle_process_result(BTState::Running);
				} else {
					return handle_process_result(BTState::Success);
				}
			} else {
				return handle_process_result(state);
			}
		}
		
	private:
		int m_index;
		std::vector<std::unique_ptr<BTNode>> m_children;
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
			auto summon_enemies = std::make_unique<SummonEnemies>(2, "King Mush Shrooma.wav", EnemyType::Mushroom, 1);
			Entity player = registry.view<Player>().front();
			std::vector<ivec2> aoe_shape;
			aoe_shape.emplace_back(0, 0);
			aoe_shape.emplace_back(0, -1);
			aoe_shape.emplace_back(-1, 0);
			aoe_shape.emplace_back(1, 0);
			auto aoe_attack = std::make_unique<AOEAttack>(aoe_shape, "King Mush Fudun.wav ", 4, player);
			auto regular_attack = std::make_unique<RegularAttack>(1);
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
				// Summoner switch to active if it spots the player.
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

		static std::unique_ptr<BTNode> dragon_tree_factory(AISystem* ai, Entity e)
		{
			registry.emplace<Dragon>(e);

			// Selector - active
			auto do_nothing_1 = std::make_unique<DoNothing>();
			auto selector_active = std::make_unique<Selector>(std::move(do_nothing_1));
			Selector* p = selector_active.get();

			
			uvec2 dragon_map_pos = registry.get<MapPosition>(e).position;
			std::vector<uvec2> altars;
			altars.push_back({ dragon_map_pos.x - 2, dragon_map_pos.y - 2 }); // top left altar
			altars.push_back({ dragon_map_pos.x + 2, dragon_map_pos.y - 2 }); // top right altar
			altars.push_back({ dragon_map_pos.x - 2, dragon_map_pos.y + 2 }); // bottom left altar
			altars.push_back({ dragon_map_pos.x + 2, dragon_map_pos.y + 2 }); // bottom right altar
			auto summon_victims = std::make_unique<SummonVictims>(2, "Dragon Roar.wav", EnemyType::KoboldMage, altars.size(), altars);
			selector_active->add_precond_and_child(
				// Dragon summons victims when its HP is below 50%.
				[ai](Entity e) {
					return (!registry.get<Dragon>(e).is_sacrifice_used) && (ai->is_health_below(e, 0.50f));
				},
				std::move(summon_victims));

			auto sacrifice_victims = std::make_unique<SacrificeVictims>(4, "Dragon Long Roar.wav", 0.25f);
			selector_active->add_precond_and_child(
				// Dragon sacrifices victims when the victims are all active (ie. They are all at an altar).
				[ai](Entity e) {
					Dragon& dragon = registry.get<Dragon>(e);
					if (dragon.victims.size() == 0) {
						return false;
					}
					for (Entity victim : dragon.victims) {
						if (registry.get<Enemy>(victim).state != EnemyState::Active) {
							return false;
						}
					}
					return true;
				},
				std::move(sacrifice_victims));

			auto summon_aoe_emitter
				= std::make_unique<SummonEnemies>(3, "Dragon Attack Roar.wav", EnemyType::AOERingGen, 1);
			auto do_nothing_aoe = std::make_unique<DoNothing>(3/*TODO: Insert AOE animation here */);
			auto aoe_sequence = std::make_unique<Sequence>();
			aoe_sequence->add_child(std::move(summon_aoe_emitter));
			aoe_sequence->add_child(std::move(do_nothing_aoe));
			selector_active->add_precond_and_child(
				[p](Entity /*e*/) { return ((p->get_process_count()+2) % 5 == 0); }, std::move(aoe_sequence)
			);
			

			auto wild_surge = std::make_unique<AOERandomAttack>("Dragon Roar.wav", 5, e, 10, 8);
			selector_active->add_precond_and_child(
				[p](Entity /*e*/) { return p->get_process_count() % 5 == 0; }, 
				std::move(wild_surge)
			);

			Entity player = registry.view<Player>().front();
			auto cone_attack = std::make_unique<AOEConeAttack>("Dragon Attack Roar.wav", 6, e, player);
			selector_active->add_precond_and_child(
				[p](Entity /*e*/) { return p->get_process_count() % 2 == 0; }, 
				std::move(cone_attack)
			);

			// Selector - idle
			auto recover_health = std::make_unique<RecoverHealth>(0.20f);
			auto do_nothing_2 = std::make_unique<DoNothing>();
			auto selector_idle = std::make_unique<Selector>(std::move(do_nothing_2));
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

			return std::make_unique<DragonTree>(std::move(selector_alive));
		}

	private:
		std::unique_ptr<BTNode> m_child;
	};

	class AOEEmitterTree : public BTNode {
	public: 
		explicit AOEEmitterTree(std::unique_ptr<BTNode> child)
			: m_child(std::move(child))
		{
		}

		void init(Entity e) override 
		{ 
			debug_log("Debug: AOEEmitterTree.init\n");
			m_child->init(e);
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("--------------------------------------------------\n");
			debug_log("Debug: AOEEmitterTree.process\n");
			BTState state = m_child->process(e, ai);
			std::string message = "Debug: State after process = ";
			message += state_names.at((size_t)state);
			debug_log(message + "\n");
			return handle_process_result(state);
		}

		static std::unique_ptr<BTNode> aoe_emitter_tree_factory(AISystem* ai, Entity target)
		{
			// Selector - active
			auto aoe_attack = std::make_unique<AOERingAttack>("Dragon Roar.wav", 7, 10, target);
			auto self_destruct = std::make_unique<SelfDestruct>();
			//auto selector_active = std::make_unique<Selector>(std::move(aoe_attack));
			auto sequence = std::make_unique<Sequence>();
			sequence->add_child(std::move(aoe_attack));
			sequence->add_child(std::move(self_destruct));

			// Selector - alive
			//auto selector_alive = std::make_unique<Selector>(std::move(sequence));

			return std::make_unique<AOEEmitterTree>(std::move(sequence));
		}

	private:
		std::unique_ptr<BTNode> m_child;
	};

	class WeaponMasterTree : public BTNode {
	public:
		explicit WeaponMasterTree(std::unique_ptr<BTNode> child)
			: m_child(std::move(child))
		{
		}

		void init(Entity e) override
		{
			debug_log("Debug: WeaponMasterTree.init\n");
			m_child->init(e);
		}

		BTState process(Entity e, AISystem* ai) override
		{
			debug_log("--------------------------------------------------\n");
			debug_log("Debug: WeaponMasterTree.process\n");
			BTState state = m_child->process(e, ai);
			std::string message = "Debug: State after process = ";
			message += state_names.at((size_t)state);
			debug_log(message + "\n");
			return handle_process_result(state);
		}

		static std::unique_ptr<BTNode> weapon_master_tree_factory(AISystem* ai)
		{
			// Selector - special attack
			auto fire_attack = std::make_unique<FireAttack>(2);
			auto ice_attack = std::make_unique<IceAttack>(3, 1.0f, 1);
			auto gale_attack = std::make_unique<GaleAttack>(5, 1.0f, 1);
			auto tar_attack = std::make_unique<TarAttack>(4, 1.0f, 1);
			auto selector_special_attack = std::make_unique<Selector>(std::move(tar_attack));
			Selector* p = selector_special_attack.get();
			selector_special_attack->add_precond_and_child(
				// WeaponMaster has 25% chance to make a fire attack during special attack mode.
				[ai](Entity /*e*/) { return ai->chance_to_happen(0.25f); },
				std::move(fire_attack));
			selector_special_attack->add_precond_and_child(
				// WeaponMaster has 25% chance to make a ice attack during special attack mode.
				[ai](Entity /*e*/) { return ai->chance_to_happen(0.33f); },
				std::move(ice_attack));
			selector_special_attack->add_precond_and_child(
				// WeaponMaster has 25% chance to make a gale attack during special attack mode.
				[ai](Entity /*e*/) { return ai->chance_to_happen(0.50f); },
				std::move(gale_attack));
				// WeaponMaster has 25% chance to make a tar attack during special attack mode.

			// Selector - active
			auto regular_attack = std::make_unique<RegularAttack>(1);
			auto sequence_active = std::make_unique<Sequence>();
			sequence_active->add_child(std::move(regular_attack));
			sequence_active->add_child(std::move(selector_special_attack));

			// Selector - idle
			auto recover_health = std::make_unique<RecoverHealth>(0.20f);
			auto do_nothing = std::make_unique<DoNothing>();
			auto selector_idle = std::make_unique<Selector>(std::move(do_nothing));
			selector_idle->add_precond_and_child(
				// WeaponMaster recover 20% HP if its HP is not full during idle.
				[ai](Entity e) { return ai->is_health_below(e, 1.00f); },
				std::move(recover_health));

			// Selector - alive
			auto selector_alive = std::make_unique<Selector>(std::move(selector_idle));
			selector_alive->add_precond_and_child(
				// WeaponMaster switch to active if it spots the player.
				[ai](Entity e) { return ai->is_player_spotted(e); },
				std::move(sequence_active));

			return std::make_unique<WeaponMasterTree>(std::move(selector_alive));
		}

	private:
		std::unique_ptr<BTNode> m_child;
	};

	// Boss entities and its corresponding behaviour trees.
	std::unordered_map<Entity, std::unique_ptr<BTNode>> bosses;
};
