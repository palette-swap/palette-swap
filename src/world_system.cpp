// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// Game configuration
bool player_arrow_fired = false;
const size_t projectile_speed = 5;

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
	auto mouse_click_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) {
		static_cast<WorldSystem*>(glfwGetWindowUserPointer(wnd))->on_mouse_click(_0, _1, _2);
	};
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_click_redirect);

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

	background_music = Mix_LoadMUS(audio_path("overworld.wav").c_str());
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

	// Processing the player state
	assert(registry.screen_states.components.size() <= 1);
	ScreenState& screen = registry.screen_states.components[0];

	// Resolves projectiles hitting objects, stops it for a period of time before returning it to the player
	// Currently handles player arrow (as it is the only projectile that exists)
    float projectile_max_counter = 2000.f;
	for (Entity entity : registry.resolved_projectiles.entities) {
		// Gets desired projectile
		ResolvedProjectile& projectile = registry.resolved_projectiles.get(entity);
		projectile.counter -= elapsed_ms_since_last_update;
		if(projectile.counter < projectile_max_counter){
		    projectile_max_counter = projectile.counter;
		}

		// Removes the projectile from the list of projectiles that are still waiting
		// If it is the player arrow, returns the arrow to the player's control
		// If other projectile, removes all components of it
		if (projectile.counter < 0) {
			if (entity == player_arrow) {
				player_arrow_fired = false;
				registry.resolved_projectiles.remove(entity);
			}
			else {
				registry.remove_all_components_of(entity);
			}
			
		}
		projectile_max_counter = 2000;
	}

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
	uvec2 player_starting_point = uvec2(51, 51);
	// Create a new Player instance and shift player onto a tile
	player = create_player(renderer, player_starting_point);

	registry.colors.insert(player, { 1, 1, 1 });

	// Create a new player arrow instance
	vec2 player_location = map_position_to_screen_position(player_starting_point);
	player_arrow = create_arrow(renderer, player_location);
	// Creates a single enemy instance, (TODO: needs to be updated with position based on grid)
	// Also requires naming scheme for randomly generated enemies, for later reference
	uvec2 enemy_starting_point = uvec2(53, 54);
	Entity enemy = create_enemy(renderer, enemy_starting_point);
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

		// collisions involving projectiles
		if (registry.active_projectiles.has(entity)) {
			//Currently, arrows can hit anything with a hittable component, which will include walls and enemies
			//TODO: rename hittable container type 
			//TODO: resolve with arrow 
			if (registry.hittables.has(entity_other)) {
				registry.motions.get(entity).velocity = { 0, 0 };
				registry.active_projectiles.remove(entity);		
				// Stops projectile motion, adds projectile to list of resolved projectiles
				registry.resolved_projectiles.emplace(entity);
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

 //Tracks position of cursor, points arrow at potential fire location
 //Only enables if an arrow has not already been fired
 //TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_move(vec2 mouse_position) {

	if (!player_arrow_fired) {
		Motion& arrow_motion = registry.motions.get(player_arrow);
		MapPosition& player_map_position= registry.map_positions.get(player);

		vec2 player_screen_position = map_position_to_screen_position(player_map_position.position);

		//Calculated Euclidean difference between player and arrow
		vec2 eucl_diff = mouse_position - player_screen_position;

		// Calculates arrow position based on position of mouse relative to player
		vec2 new_arrow_position = normalize(eucl_diff) * 20.f + player_screen_position;
		arrow_motion.position = new_arrow_position;


		// Calculates arrow angle based on position of mouse
		arrow_motion.angle = atan2(eucl_diff.x, -eucl_diff.y);
	}
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
	}
	else if (direction == Direction::Up && map_pos.position.y > 0) {
		uvec2 new_pos = uvec2(map_pos.position.x, map_pos.position.y - 1);
		if (map_generator->walkable(new_pos)) {
			map_pos.position = new_pos;
		}
	}
	else if (direction == Direction::Right && map_pos.position.x < room_size * tile_size - 1) {
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

// Fires arrow at a preset speed if it has not been fired already
// TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_click(int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!player_arrow_fired) {
			player_arrow_fired = true;
			// Arrow becomes a projectile the moment it leaves the player, not while it's direction is being selected
			registry.active_projectiles.emplace(player_arrow);
			Motion& arrow_motion = registry.motions.get(player_arrow);

			// TODO: Add better arrow physics potentially?
			arrow_motion.velocity = { sin(arrow_motion.angle) * projectile_speed, -cos(arrow_motion.angle) * projectile_speed };
		}
	}
}
