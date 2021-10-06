
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include "ai_system.hpp"
#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "map_generator_system.hpp"

using Clock_t = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// Map system
	std::shared_ptr<MapGeneratorSystem> map = std::make_shared<MapGeneratorSystem>();

	// Global systems
	Debug debugging;
	WorldSystem world(debugging, map);
	RenderSystem renderer;
	PhysicsSystem physics(debugging);
	AISystem ai;

	// Initializing window
	GLFWwindow* window = world.create_window(window_width_px, window_height_px);
	if (window == nullptr) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}

	// initialize the main systems
	renderer.init(window_width_px, window_height_px, window);
	world.init(&renderer);

	// variable timestep loop
	auto t = Clock_t::now();
	while (!world.is_over()) {
		// Processes system messages, if this wasn't present the window would become
		// unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock_t::now();
		float elapsed_ms = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		world.step(elapsed_ms);
		ai.step(elapsed_ms);
		physics.step(elapsed_ms, window_width_px, window_height_px);
		world.handle_collisions();

		renderer.draw();
	}

	return EXIT_SUCCESS;
}
