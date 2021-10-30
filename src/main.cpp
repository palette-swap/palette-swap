
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include "ai_system.hpp"
#include "animation_system.hpp"
#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "physics_system.hpp"
#include "render_system.hpp"
#include "turn_system.hpp"
#include "world_system.hpp"


using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// Combat System
	std::shared_ptr<CombatSystem> combat = std::make_shared<CombatSystem>();

	// Map system
	std::shared_ptr<MapGeneratorSystem> map = std::make_shared<MapGeneratorSystem>();

	// Turn System
	std::shared_ptr<TurnSystem> turns = std::make_shared<TurnSystem>();

	//// Animation System
	std::shared_ptr < AnimationSystem > animations = std::make_shared<AnimationSystem>();

	// Global systems
	Debug debugging;
	WorldSystem world(debugging, combat, map, turns);
	RenderSystem renderer;
	PhysicsSystem physics(debugging, map);
	AISystem ai(debugging, combat, map, turns);

	// Initializing window
	GLFWwindow* window = world.create_window(window_width_px, window_height_px);
	if (window == nullptr) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}

	// initialize the main systems
	so_loud.init();
	renderer.init(window_width_px, window_height_px, window, map);
	world.init(&renderer);
	animations->init();

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
		animations->update_animations(elapsed_ms);
		renderer.draw();
		turns->step();
	}
	// Destroy music components
	so_loud.deinit();

	return EXIT_SUCCESS;
}
