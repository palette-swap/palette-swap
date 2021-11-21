#pragma once

// internal
#include "common.hpp"

// stlib
#include <memory>
#include <random>
#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#define WITH_SDL2 1
#include "soloud_wav.h"
#include "soloud_wavstream.h"

#include "animation_system.hpp"
#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "render_system.hpp"
#include "turn_system.hpp"
#include "ui_system.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem {
public:
	WorldSystem(Debug& debugging,
				std::shared_ptr<CombatSystem> combat,
				std::shared_ptr<MapGeneratorSystem> map,
				std::shared_ptr<TurnSystem> turns,
				std::shared_ptr<AnimationSystem> animations,
				std::shared_ptr<UISystem> ui,
				std::shared_ptr<SoLoud::Soloud> so_loud);

	// Creates a window
	GLFWwindow* create_window(int width, int height);

	// starts the game
	void init(RenderSystem* renderer);

	// Releases all associated resources
	~WorldSystem();
	WorldSystem(const WorldSystem&) = delete;
	WorldSystem& operator=(const WorldSystem&) = delete;
	WorldSystem(WorldSystem&&) = delete;
	WorldSystem& operator=(WorldSystem&&) = delete;

	// Steps the game ahead by ms milliseconds
	bool step(float elapsed_ms);

	// Check for collisions
	void handle_collisions();

	// Should the game be over ?
	bool is_over() const;

private:
	// Input callback functions
	void on_key(int key, int /*scancode*/, int action, int mod);
	void on_mouse_move(vec2 mouse_position);
	void on_mouse_click(int button, int action, int /*mods*/);
	void on_mouse_scroll(float offset);
	void on_resize(int width, int height);

	// Key helpers
	void check_debug_keys(int key, int action, int mod);

	// Mouse Click helpers
	void try_fire_projectile(Attack& attack);
	void try_adjacent_attack(Attack& attack);

	// restart level
	void restart_game();

	// returns arrow asset to player
	void return_arrow_to_player();

	// move the player one unit in the given direction,
	// if the tile is blocked by a wall, player won't move
	void move_player(Direction direction);

	// Flips color state.
	void try_change_color();

	// OpenGL window handle
	GLFWwindow* window = nullptr;

	// Game configuration
	bool player_arrow_fired = false;
	// TODO: Track why my projectile speed had slowed throughout
	const float projectile_speed = 200.f;

	// Game state
	RenderSystem* renderer = nullptr;
	float current_volume = 1;
	bool end_of_game = false;
	float spell_distance_from_player = 22.f;
	Entity player = registry.create();
	Entity camera = registry.create();
	Entity player_arrow = registry.create();
	Debug& debugging;

	// music references
	std::shared_ptr<SoLoud::Soloud> so_loud;
	SoLoud::WavStream bgm_red_wav;
	SoLoud::WavStream bgm_blue_wav;
	SoLoud::handle bgm_red;
	SoLoud::handle bgm_blue;

	// Sound effects
	SoLoud::Wav light_sword_wav;
	SoLoud::Wav cannon_wav;

	// C++ random number generator
	std::shared_ptr<std::default_random_engine> rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1

	std::shared_ptr<AnimationSystem> animations;
	std::shared_ptr<CombatSystem> combat;
	std::shared_ptr<MapGeneratorSystem> map_generator;
	std::shared_ptr<TurnSystem> turns;

	bool is_editing_map = false;
	std::shared_ptr<UISystem> ui;
};
