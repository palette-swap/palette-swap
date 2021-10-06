// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// Game configuration

// Create the world
WorldSystem::WorldSystem(Debug& debugging, std::shared_ptr<MapGeneratorSystem> map)
	: points(0)
	, debugging(debugging)
{
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());

	// Instantiate MapGeneratorSystem class
	map_generator = std::move(map);
}

WorldSystem::~WorldSystem()
{
	// Destroy music components
	if (background_music != nullptr) {
		Mix_FreeMusic(background_music);
	}
	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
void glfw_err_cb(int error, const char* desc) { fprintf(stderr, "%d: %s", error, desc); }
} // namespace

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window(int width, int height)
{
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (glfwInit() == GLFW_FALSE) {
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(width, height, "Palette Swap", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) {
		static_cast<WorldSystem*>(glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3);
	};
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) {
		static_cast<WorldSystem*>(glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 });
	};
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return nullptr;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	background_music = Mix_LoadMUS(audio_path("henry_martin.wav").c_str());
	// Example left to demonstrate loading of WAV instead of MUS, need to check difference
	salmon_dead_sound = Mix_LoadWAV(audio_path("salmon_dead.wav").c_str());

	if (background_music == nullptr) {
		fprintf(stderr,
				"Failed to load sounds\n %s\n make sure the data directory is present",
				audio_path("overworld.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg)
{
	this->renderer = renderer_arg;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
	restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{
	// Get the screen dimensions

	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (!registry.debug_components.entities.empty()) {
		registry.remove_all_components_of(registry.debug_components.entities.back());
	}

	// Removing out of screen entities
	auto& motions_registry = registry.motions;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = (int)motions_registry.components.size() - 1; i >= 0; --i) {
		Motion& motion = motions_registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f) {
			registry.remove_all_components_of(motions_registry.entities[i]);
		}
	}

	// Processing the player state
	assert(registry.screen_states.components.size() <= 1);
	ScreenState& screen = registry.screen_states.components[0];

	float min_counter_ms = 3000.f;
	for (Entity entity : registry.death_timers.entities) {
		// progress timer
		DeathTimer& counter = registry.death_timers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;
		if (counter.counter_ms < min_counter_ms) {
			min_counter_ms = counter.counter_ms;
		}

		// restart the game once the death timer expired
		if (counter.counter_ms < 0) {
			registry.death_timers.remove(entity);
			screen.darken_screen_factor = 0;
			restart_game();
			return true;
		}
	}
	// reduce window brightness if any of the present salmons is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game()
{
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that were created with a motion (TODO: Adjust once grid based components are introduced)
	while (!registry.motions.entities.empty()) {
		registry.remove_all_components_of(registry.motions.entities.back());
	}

	// Debugging for memory/component leaks
	registry.list_all_components();

	// Generate the levels
	map_generator->generate_levels();

	vec2 middle = { window_width_px / 2, window_height_px / 2 };

	const MapGeneratorSystem::Mapping& mapping = map_generator->current_map();
	vec2 top_left_corner = middle - vec2(tile_size * room_size * map_size / 2, tile_size * room_size * map_size / 2);
	for (size_t row = 0; row < mapping.size(); row++) {
		for (size_t col = 0; col < mapping[0].size(); col++) {
			vec2 position = top_left_corner +
				vec2(tile_size * room_size / 2, tile_size * room_size / 2) +
				vec2(col * tile_size * room_size, row * tile_size * room_size);
			create_room(renderer, position, mapping.at(row).at(col));
		}
	}

	// a random starting position... probably need to update this
	vec2 player_starting_point = uvec2(51, 51);
	// Create a new Player instance and shift player onto a tile
	player = create_player(renderer, player_starting_point);

	registry.colors.insert(player, { 1, 0.8f, 0.8f });


	// Creates a single enemy instance, (TODO: needs to be updated with position based on grid)
	// Also requires naming scheme for randomly generated enemies, for later reference
	Entity enemy = create_enemy(renderer, { 680, 600 });
	registry.colors.insert(enemy, { 1, 1, 1 });
}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
	// Loop over all collisions detected by the physics system
	auto& collisions_registry = registry.collisions;
	for (uint i = 0; i < collisions_registry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisions_registry.entities[i];
		Entity entity_other = collisions_registry.components[i].other;

		// For now, we are only interested in collisions that involve the salmon
		if (registry.players.has(entity)) {
			// Player& player = registry.players.get(entity);

			// Example of how system currently handles collisions with a certain type of entity,
			// will be replaced with other collisions types
			if (registry.hard_shells.has(entity_other)) {
				// initiate death unless already dying
				if (!registry.death_timers.has(entity)) {
					// Scream, reset timer, and make the salmon sink
					registry.death_timers.emplace(entity);
					registry.motions.get(entity).velocity = { 0, 80 };
				}
			}
		}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const { return bool(glfwWindowShouldClose(window)); }

// On key callback
void WorldSystem::on_key(int key, int /*scancode*/, int action, int mod)
{
	if (action != GLFW_RELEASE) {
		if (key == GLFW_KEY_RIGHT) {
			move_player(Direction::Right);
		}

		if (key == GLFW_KEY_LEFT) {
			move_player(Direction::Left);
		}

		if (key == GLFW_KEY_UP) {
			move_player(Direction::Up);
		}

		if (key == GLFW_KEY_DOWN) {
			move_player(Direction::Down);
		}
	}

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		// int w, h;
		// glfwGetWindowSize(window, &w, &h);

		restart_game();
	}

	// Debugging
	if (key == GLFW_KEY_D) {
		debugging.in_debug_mode = action != GLFW_RELEASE;
	}

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	current_speed = fmax(0.f, current_speed);
}

void WorldSystem::move_player(Direction direction)
{
	MapPosition& map_pos = registry.map_positions.get(player);
	// TODO: this should be removed once we only use map_position

	if (direction == Direction::Left && map_pos.position.x > 0) {
		uvec2 new_pos = uvec2(map_pos.position.x - 1, map_pos.position.y);
		if (map_generator->walkable(new_pos)) {
			map_pos.position = new_pos;
		}
	} else if (direction == Direction::Up && map_pos.position.y > 0) {
		uvec2 new_pos = uvec2(map_pos.position.x, map_pos.position.y - 1);
		if (map_generator->walkable(new_pos)) {
			map_pos.position = new_pos;
		}
	} else if (direction == Direction::Right && map_pos.position.x < room_size * tile_size - 1) {
		uvec2 new_pos = uvec2(map_pos.position.x + 1, map_pos.position.y);
		if (map_generator->walkable(new_pos)) {
			map_pos.position = new_pos;
		}
	} else if (direction == Direction::Down && map_pos.position.y < room_size * tile_size - 1) {
		uvec2 new_pos = uvec2(map_pos.position.x, map_pos.position.y + 1);
		if (map_generator->walkable(new_pos)) {
			map_pos.position = new_pos;
		}
	}
}

void WorldSystem::on_mouse_move(vec2 /*mouse_position*/) { }
