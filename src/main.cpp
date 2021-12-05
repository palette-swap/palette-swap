
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include "ai_system.hpp"
#include "animation_system.hpp"
#include "combat_system.hpp"
#include "lighting_system.hpp"
#include "map_generator_system.hpp"
#include "physics_system.hpp"
#include "render_system.hpp"
#include "story_system.hpp"
#include "turn_system.hpp"
#include "tutorial_system.hpp"
#include "ui_system.hpp"
#include "world_system.hpp"


using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// Audio core
	std::shared_ptr<SoLoud::Soloud> so_loud = std::make_shared<SoLoud::Soloud>();
	so_loud->init();

	// Combat System
	std::shared_ptr<CombatSystem> combat = std::make_shared<CombatSystem>();

	// Turn System
	std::shared_ptr<TurnSystem> turns = std::make_shared<TurnSystem>();

	// Animation System
	std::shared_ptr<AnimationSystem> animations = std::make_shared<AnimationSystem>();

	// UI System
	std::shared_ptr<UISystem> ui = std::make_shared<UISystem>();

	// Map system
	std::shared_ptr<MapGeneratorSystem> map = std::make_shared<MapGeneratorSystem>(turns, ui);

	// Story System
	std::shared_ptr<StorySystem> stories = std::make_shared<StorySystem>(animations, map);

	// Tutorial System
	std::shared_ptr<TutorialSystem> tutorials = std::make_shared<TutorialSystem>();

	// Global systems
	Debug debugging;
	WorldSystem world(debugging, combat, map, turns, animations, ui, so_loud, stories, tutorials);
	LightingSystem lighting;
	RenderSystem renderer(debugging, lighting);
	PhysicsSystem physics(debugging, map);
	AISystem ai(debugging, combat, map, turns, animations, so_loud);

	// Initializing window
	GLFWwindow* window = world.create_window(window_width_px, window_height_px);
	if (window == nullptr) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}

	// initialize the main systems
	renderer.init(window_width_px, window_height_px, window, map);
	world.init(&renderer);
	lighting.init(map);

	// variable timestep loop
	auto t = Clock::now();
	while (!world.is_over()) {
		// Processes system messages, if this wasn't present the window would become
		// unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		world.step(elapsed_ms);
		ai.step(elapsed_ms);
		physics.step(elapsed_ms, window_width_px, window_height_px);
		world.handle_collisions();
		animations->update_animations(elapsed_ms, turns->get_inactive_color());
		map->step(elapsed_ms);
		turns->step();
		lighting.step(elapsed_ms);
		renderer.draw();
	}

	return EXIT_SUCCESS;
}
