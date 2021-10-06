// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// Game configuration
bool player_arrow_fired = false;
const size_t projectile_speed = 80;

// Create the world
WorldSystem::WorldSystem()
	: points(0) {
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window(int width, int height) {
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
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
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	auto mouse_click_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_click(_0, _1, _2 ); };
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
		fprintf(stderr, "Failed to load sounds\n %s\n make sure the data directory is present",
			audio_path("overworld.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg) {
	this->renderer = renderer_arg;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
    restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {
	// Get the screen dimensions
	int screen_width, screen_height;
	glfwGetFramebufferSize(window, &screen_width, &screen_height);

	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
	    registry.remove_all_components_of(registry.debugComponents.entities.back());

	// Removing out of screen entities
	auto& motions_registry = registry.motions;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = (int)motions_registry.components.size()-1; i>=0; --i) {
	    Motion& motion = motions_registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f) {
		    registry.remove_all_components_of(motions_registry.entities[i]);
		}
	}

	// Processing the player state
	assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

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
void WorldSystem::restart_game() {
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that were created with a motion (TODO: Adjust once grid based components are introduced)
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());

	// Debugging for memory/component leaks
	registry.list_all_components();

	// Creates a single enemy instance, (TODO: needs to be updated with position based on grid)
	// Also requires naming scheme for randomly generated enemies, for later reference
	Entity enemy = createEnemy(renderer, { 680,600 });
	registry.colors.insert(enemy, { 1, 1, 1 });

	// Create a new Player instance
	player = createPlayer(renderer, { 640, 448 });
	registry.colors.insert(player, { 1, 1, 1 });

	// Create a new Player's Arrow instance 
	player_arrow = createArrow(renderer, { 600, 600 });
	registry.colors.insert(player_arrow, { 1, 1, 1 });
	
}



// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions; 
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;

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
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod) {
	int position_change = 64;
	Motion& motion = registry.motions.get(player);

	if (action != GLFW_RELEASE) {
		if (key == GLFW_KEY_RIGHT) {
			motion.position += vec2(position_change, 0);
		}

		if (key == GLFW_KEY_LEFT) {
			motion.position += vec2(-position_change, 0);
		}

		if (key == GLFW_KEY_UP) {
			motion.position += vec2(0, -position_change);
		}

		if (key == GLFW_KEY_DOWN) {
			motion.position += vec2(0, position_change);
		}
	}

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

        restart_game();
	}

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	current_speed = fmax(0.f, current_speed);
}

// Tracks position of cursor, points arrow at potential fire location
// Only enables if an arrow has not already been fired
// TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_move(vec2 mouse_position) {

	if (!player_arrow_fired) {
		Motion& arrow_motion = registry.motions.get(player_arrow);
		Motion& player_motion = registry.motions.get(player);

		//Calculated Euclidean difference between player and arrow
		vec2 eucl_diff = mouse_position - player_motion.position;

		// Calculates arrow position based on position of mouse relative to player
		vec2 new_arrow_position = normalize(eucl_diff) * 40.f + player_motion.position;
		arrow_motion.position = new_arrow_position;


		// Calculates arrow angle based on position of mouse
		arrow_motion.angle = atan2(eucl_diff.x, -eucl_diff.y);
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
			arrow_motion.velocity = {sin(arrow_motion.angle) * projectile_speed, -cos(arrow_motion.angle) * projectile_speed};
		}
	}
}
