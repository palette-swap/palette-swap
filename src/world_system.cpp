// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// Create the world
WorldSystem::WorldSystem(Debug& debugging,
						 std::shared_ptr<CombatSystem> combat,
						 std::shared_ptr<MapGeneratorSystem> map,
						 std::shared_ptr<TurnSystem> turns)
	: points(0)
	, debugging(debugging)
	, rng(std::make_shared<std::default_random_engine>(std::default_random_engine(std::random_device()())))
	, combat(std::move(combat))
	, map_generator(std::move(map))
	, turns(std::move(turns))
{
	this->combat->init(rng);
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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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
	auto scroll_redirect = [](GLFWwindow* wnd, double /*_0*/, double _1) {
		static_cast<WorldSystem*>(glfwGetWindowUserPointer(wnd))->on_mouse_scroll(static_cast<float>(_1));
	};
	auto resize_redirect = [](GLFWwindow* wnd, int width, int height) {
		static_cast<WorldSystem*>(glfwGetWindowUserPointer(wnd))->on_resize(width, height);
	};
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_click_redirect);
	glfwSetScrollCallback(window, scroll_redirect);
	glfwSetWindowSizeLimits(window, GLFW_DONT_CARE, GLFW_DONT_CARE, 1920, 1080);
	glfwSetFramebufferSizeCallback(window, resize_redirect);

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

	// Processing the player state
	assert(registry.screen_states.components.size() <= 1);
	// ScreenState& screen = registry.screen_states.components[0];
	if (registry.stats.get(player).health <= 0 && turns->get_active_team() == player) {
		restart_game();
		return true;
	}

	// Resolves projectiles hitting objects, stops it for a period of time before returning it to the player
	// Currently handles player arrow (as it is the only projectile that exists)
	float projectile_max_counter = 1000.f;
	for (Entity entity : registry.resolved_projectiles.entities) {
		// Gets desired projectile
		ResolvedProjectile& projectile = registry.resolved_projectiles.get(entity);
		projectile.counter -= elapsed_ms_since_last_update;
		if (projectile.counter < projectile_max_counter) {
			projectile_max_counter = projectile.counter;
		}

		// Removes the projectile from the list of projectiles that are still waiting
		// If it is the player arrow, returns the arrow to the player's control
		// If other projectile, removes all components of it
		if (projectile.counter < 0) {
			if (entity == player_arrow) {
				registry.resolved_projectiles.remove(entity);
				player_arrow_fired = false;
				turns->complete_team_action(player);
				return_arrow_to_player();
			} else {
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

	// Remove the old player team
	turns->remove_team_from_queue(player);

	while (!registry.map_positions.entities.empty()) {
		registry.remove_all_components_of(registry.map_positions.entities.back());
	}
	// Remove all entities that were created with a position
	while (!registry.world_positions.entities.empty()) {
		registry.remove_all_components_of(registry.world_positions.entities.back());
	}

	// Debugging for memory/component leaks
	registry.list_all_components();

	// Generate the levels
	map_generator->generate_levels();

	vec2 middle = { window_width_px / 2, window_height_px / 2 };

	const MapGeneratorSystem::Mapping& mapping = map_generator->current_map();
	vec2 top_left_corner_pos = middle
		- vec2(MapUtility::tile_size * MapUtility::room_size * MapUtility::map_size / 2,
			   MapUtility::tile_size * MapUtility::room_size * MapUtility::map_size / 2);
	for (size_t row = 0; row < mapping.size(); row++) {
		for (size_t col = 0; col < mapping[0].size(); col++) {
			vec2 position = top_left_corner_pos
				+ vec2(MapUtility::tile_size * MapUtility::room_size / 2,
					   MapUtility::tile_size * MapUtility::room_size / 2)
				+ vec2(col * MapUtility::tile_size * MapUtility::room_size,
					   row * MapUtility::tile_size * MapUtility::room_size);
			create_room(position, mapping.at(row).at(col));
		}
	}

	// a random starting position... probably need to update this
	uvec2 player_starting_point = uvec2(1, 1);
	// Create a new Player instance and shift player onto a tile
	player = create_player(player_starting_point);
	turns->add_team_to_queue(player);

	// create camera instance
	camera = create_camera(
		player_starting_point);

	// Create a new player arrow instance
	vec2 player_location = MapUtility::map_position_to_screen_position(player_starting_point);
	player_arrow = create_arrow(player_location);
	// Creates a single enemy instance, (TODO: needs to be updated with position based on grid)
	// Also requires naming scheme for randomly generated enemies, for later reference
	create_enemy(uvec2(12, 3));
	create_enemy(uvec2(15, 3));
	create_enemy(uvec2(15, 4), ColorState::Blue);
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
			ActiveProjectile& projectile = registry.active_projectiles.get(entity);
			// TODO: rename hittable container type
			// TODO: Convert all checks to if tile is walkable, currently has issue that enemies can overlay player
			// Currently, arrows can hit anything with a hittable component, which will include walls and enemies
			if (registry.hittables.has(entity_other)) {
				registry.velocities.get(entity).speed = 0;
				registry.active_projectiles.remove(entity);

				// Attack the other entity if it can be attacked
				if (registry.stats.has(entity_other)) {
					Stats& player_stats = registry.stats.get(player);
					Stats& enemy_stats = registry.stats.get(entity_other);
					combat->do_attack(player_stats, player_stats.base_attack, enemy_stats);
				}

				// Stops projectile motion, adds projectile to list of resolved projectiles
				registry.resolved_projectiles.emplace(entity);
			} else {
				// Checks if projectile's head has hit a wall
				uvec2 projectile_location = (MapUtility::screen_position_to_map_position(
					registry.world_positions.get(entity).position + projectile.head_offset));

				// Hacky way, using the same check for the player's "walkable"
				if (!map_generator->walkable(projectile_location)) {
					registry.velocities.get(entity).speed = 0;
					registry.active_projectiles.remove(entity);
					// Stops projectile motion, adds projectile to list of resolved projectiles
					registry.resolved_projectiles.emplace(entity);
				}
			}
		}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over?
bool WorldSystem::is_over() const { return bool(glfwWindowShouldClose(window)); }

// Returns arrow to player after firing
void WorldSystem::return_arrow_to_player()
{
	dvec2 mouse_pos;
	glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);
	on_mouse_move(vec2(mouse_pos));
}

// On key callback
void WorldSystem::on_key(int key, int /*scancode*/, int action, int mod)
{
	if (turns && action != GLFW_RELEASE) {

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
		
		if (key == GLFW_KEY_EQUAL) {
			change_color();
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

// Tracks position of cursor, points arrow at potential fire location
// Only enables if an arrow has not already been fired
// TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_move(vec2 mouse_position)
{
	if (!player_arrow_fired) {

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		vec2 position = MapUtility::map_position_to_screen_position(registry.map_positions.get(player).position);

		float left = position.x - w * renderer->screen_scale / 2.f;
		float top = position.y - h * renderer->screen_scale / 2.f;

		vec2 mouse_screen_position = mouse_position * renderer->screen_scale + vec2(left, top);

		// The mouse_position has to be adjusted to screen position (screen position might be a bad naming since it's
		// actually world position). The above code I borrowed from RenderSystem::create_projection_matrix(). In the
		// future, we can wrap left/top/right/bottom/screen_scale into camera component, so you can easily access them.

		Velocity& arrow_velocity = registry.velocities.get(player_arrow);
		WorldPosition& arrow_position = registry.world_positions.get(player_arrow);
		MapPosition& player_map_position = registry.map_positions.get(player);

		vec2 player_screen_position = MapUtility::map_position_to_screen_position(player_map_position.position);

		// Calculated Euclidean difference between player and arrow
		vec2 eucl_diff = mouse_screen_position - player_screen_position;

		// Calculates arrow position based on position of mouse relative to player
		vec2 new_arrow_position = normalize(eucl_diff) * 20.f + player_screen_position;
		arrow_position.position = new_arrow_position;

		// Calculates arrow angle based on position of mouse
		arrow_velocity.angle = atan2(eucl_diff.x, -eucl_diff.y);
	}
}

void WorldSystem::move_player(Direction direction)
{
	if (turns->get_active_team() != player) {
		return;
	}

	MapPosition& map_pos = registry.map_positions.get(player);
	WorldPosition& arrow_position = registry.world_positions.get(player_arrow);
	// TODO: this should be removed once we only use map_position
	uvec2 new_pos = map_pos.position;

	if (direction == Direction::Left && map_pos.position.x > 0) {
		new_pos = uvec2(map_pos.position.x - 1, map_pos.position.y);
	} else if (direction == Direction::Up && map_pos.position.y > 0) {
		new_pos = uvec2(map_pos.position.x, map_pos.position.y - 1);
	} else if (direction == Direction::Right
			   && map_pos.position.x < MapUtility::room_size * MapUtility::tile_size - 1) {
		new_pos = uvec2(map_pos.position.x + 1, map_pos.position.y);
	} else if (direction == Direction::Down && map_pos.position.y < MapUtility::room_size * MapUtility::tile_size - 1) {
		new_pos = uvec2(map_pos.position.x, map_pos.position.y + 1);
	}

	if (map_pos.position == new_pos || !map_generator->walkable(new_pos) || !turns->execute_team_action(player)) {
		return;
	}
	// Temp update for arrow position
	if (!player_arrow_fired) {
		arrow_position.position += (vec2(new_pos) - vec2(map_pos.position)) * MapUtility::tile_size;
	}
	map_pos.position = new_pos;
	turns->complete_team_action(player);
}

void WorldSystem::change_color() 
{ 
	switch (turns->get_active_color()) {
	case ColorState::Red:
		turns->set_active_color(ColorState::Blue);
		break;
	case ColorState::Blue:
		turns->set_active_color(ColorState::Red);
		break;
	default:
		turns->set_active_color(ColorState::Red);
		break;
	}

	//ColorState active_color = turns->get_active_color();
	//for (long long i = registry.enemy_states.entities.size() - 1; i >= 0; i--) { 
	//	const Entity& enemy_entity = registry.enemy_states.entities[i];
	//	if (((uint8_t)registry.enemy_states.get(enemy_entity).team & (uint8_t)active_color) == 0) {
	//		registry.render_requests.get(enemy_entity).visible = false;
	//	} else {
	//		registry.render_requests.get(enemy_entity).visible = true;
	//	}
	//}
}

// Fires arrow at a preset speed if it has not been fired already
// TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_click(int button, int action, int /*mods*/)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!player_arrow_fired && turns->execute_team_action(player)) {
			player_arrow_fired = true;
			// Arrow becomes a projectile the moment it leaves the player, not while it's direction is being selected
			ActiveProjectile& arrow_projectile = registry.active_projectiles.emplace(player_arrow);
			Velocity& arrow_velocity = registry.velocities.get(player_arrow);

			// Denotes arrowhead location the player's arrow, based on firing angle and current scaling
			arrow_projectile.head_offset
				= { sin(arrow_velocity.angle) * scaling_factors.at(static_cast<int>(TEXTURE_ASSET_ID::ARROW)).y / 2,
					-cos(arrow_velocity.angle) * scaling_factors.at(static_cast<int>(TEXTURE_ASSET_ID::ARROW)).x / 2 };

			arrow_velocity.speed = projectile_speed;
			// TODO: Add better arrow physics potentially?
			// arrow_velocity.velocity
			//	= { sin(arrow_motion.angle) * projectile_speed, -cos(arrow_motion.angle) * projectile_speed };
		}
	}
}

void WorldSystem::on_mouse_scroll(float offset) { this->renderer->scale_on_scroll(offset); }

// resize callback
// TODO: update to scale the scene as not changed when window is resized
void WorldSystem::on_resize(int width, int height) { renderer->on_resize(width, height); }
