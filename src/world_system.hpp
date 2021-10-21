#pragma once

// internal
#include "common.hpp"

// stlib
#include <memory>
#include <random>
#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "render_system.hpp"
#include "turn_system.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem {
public:
	WorldSystem(Debug& debugging,
				std::shared_ptr<CombatSystem> combat,
				std::shared_ptr<MapGeneratorSystem> map,
				std::shared_ptr<TurnSystem> turns);

	// Creates a window
	GLFWwindow* create_window(int width, int height);

	// starts the game
	void init(RenderSystem* renderer);

	// Releases all associated resources
	~WorldSystem();

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

	// restart level
	void restart_game();

	// returns arrow asset to player
	void return_arrow_to_player();

	// move the player one unit in the given direction,
	// if the tile is blocked by a wall, player won't move
	void move_player(Direction direction);

	// Flips color state.
	void change_color();

	// OpenGL window handle
	GLFWwindow* window = nullptr;

	// Number of fish eaten by the salmon, displayed in the window title
	unsigned int points;

	// Game configuration
	bool player_arrow_fired = false;
	// TODO Track why my projectile speed had slowed throughout
	const size_t projectile_speed = 500;

	// Game state
	RenderSystem* renderer = nullptr;
	float current_speed = 0;
	Entity player;
	Entity camera;
	Entity player_arrow;
	Debug& debugging;

	// music references
	Mix_Music* background_music = nullptr;
	Mix_Chunk* salmon_dead_sound = nullptr;

	// C++ random number generator
	std::shared_ptr <std::default_random_engine> rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1

	std::shared_ptr<CombatSystem> combat;
	std::shared_ptr<MapGeneratorSystem> map_generator;
	std::shared_ptr<TurnSystem> turns;
};
