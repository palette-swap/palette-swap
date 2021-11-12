// Header
#include "world_system.hpp"
#include "world_init.hpp"

// For calling sleep
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

// stlib
#include <cassert>
#include <iostream>
#include <sstream>

#include "physics_system.hpp"

// Create the world
WorldSystem::WorldSystem(Debug& debugging,
						 std::shared_ptr<CombatSystem> combat,
						 std::shared_ptr<MapGeneratorSystem> map,
						 std::shared_ptr<TurnSystem> turns,
						 std::shared_ptr<AnimationSystem> animations,
						 std::shared_ptr<UISystem> ui,
						 std::shared_ptr<SoLoud::Soloud> so_loud)

	: current_weapon(entt::null) 
	, points(0)
	, debugging(debugging)
	, so_loud(std::move(so_loud))
	, bgm_red()
	, bgm_blue()
	, rng(std::make_shared<std::default_random_engine>(std::default_random_engine(std::random_device()())))
	, animations(std::move(animations))
	, combat(std::move(combat))
	, map_generator(std::move(map))
	, turns(std::move(turns))
	, ui(std::move(ui))
{
	this->combat->init(rng, this->animations);
}

WorldSystem::~WorldSystem()
{
	// Destroy all created components
	registry.clear();

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
	glfwSetWindowSizeLimits(window, GLFW_DONT_CARE, GLFW_DONT_CARE, GLFW_DONT_CARE, GLFW_DONT_CARE);
	glfwSetFramebufferSizeCallback(window, resize_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	bgm_red_wav.load(audio_path("henry_martin.wav").c_str());
	bgm_red_wav.setLooping(true);
	bgm_blue_wav.load(audio_path("famous_flower_of_serving_men.wav").c_str());
	bgm_blue_wav.setLooping(true);

	light_sword_wav.load(audio_path("sword1.wav").c_str());
	cannon_wav.load(audio_path("cannon.wav").c_str());

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg)
{
	this->renderer = renderer_arg;

	// Playing background music indefinitely
	bgm_red = so_loud->play(bgm_red_wav);
	bgm_blue = so_loud->play(bgm_blue_wav, 0.f);
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
	auto debug_view = registry.view<DebugComponent>();
	registry.destroy(debug_view.begin(), debug_view.end());

	// Processing the player state
	assert(registry.size<ScreenState>() <= 1);
	// ScreenState& screen = registry.screen_states.components[0];
	if ((registry.get<Stats>(player).health <= 0) && turns->ready_to_act(player)) {
		restart_game();
		return true;
	}
	if (end_of_game && turns->ready_to_act(player)) {
		restart_game();
		return true;
	}

	// Resolves projectiles hitting objects, stops it for a period of time before returning it to the player
	// Currently handles player arrow (as it is the only projectile that exists)
	for (auto [entity, projectile] : registry.view<ResolvedProjectile>().each()) {
		projectile.counter -= elapsed_ms_since_last_update;

		// Removes the projectile from the list of projectiles that are still waiting
		// If it is the player arrow, returns the arrow to the player's control
		// If other projectile, removes all components of it
		if (projectile.counter < 0) {
			if (entity == player_arrow) {
				registry.remove<ResolvedProjectile>(entity);
				player_arrow_fired = false;
				turns->complete_team_action(player);
				return_arrow_to_player();
			} else {
				registry.destroy(entity);
			}
		}
	}

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game()
{
	// Debugging for memory/component leaks
	std::cout << "Alive: " << registry.alive() << std::endl;
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 1.f;
	// Reset the game end
	end_of_game = false;

	// Remove the old player team
	turns->remove_team_from_queue(player);

	// Remove anything in the world
	auto map_pos_view = registry.view<MapPosition>();
	registry.destroy(map_pos_view.begin(), map_pos_view.end());

	auto screen_pos_view = registry.view<ScreenPosition>();
	registry.destroy(screen_pos_view.begin(), screen_pos_view.end());

	auto world_pos_view = registry.view<WorldPosition>();
	registry.destroy(world_pos_view.begin(), world_pos_view.end());

	// Debugging for memory/component leaks
	std::cout << "Alive: " << registry.alive() << std::endl;

	map_generator->load_initial_level();

	// a random starting position... probably need to update this
	uvec2 player_starting_point = map_generator->get_player_start_position();
	// Create a new Player instance and shift player onto a tile
	player = create_player(player_starting_point);
	// TODO: Should move into create_player (under world init) afterwards
	animations->set_player_animation(player);

	turns->add_team_to_queue(player);
	// Reset current weapon & attack
	Inventory& inventory = registry.get<Inventory>(player);
	current_weapon = inventory.equipped[static_cast<uint8>(Slot::PrimaryHand)];
	current_attack = 0;
	attack_display = registry.create();
	registry.emplace<ScreenPosition>(attack_display, vec2(0, 1));
	registry.emplace<Text>(attack_display,
						   combat->make_attack_list(current_weapon, current_attack),
						   (uint16)48,
						   Alignment::Start,
						   Alignment::End);
	registry.emplace<Color>(attack_display, vec3(1, 1, 1));
	// create camera instance
	camera = create_camera(player_starting_point);

	// Create a new player arrow instance
	vec2 player_location = MapUtility::map_position_to_world_position(player_starting_point);
	player_arrow = create_arrow(player_location);
	registry.get<RenderRequest>(player_arrow).visible
		= registry.get<Weapon>(current_weapon).given_attacks.at(current_attack).targeting_type
		== TargetingType::Projectile;

	// Restart the UISystem
	ui->restart_game();
}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
	// Loop over all collisions detected by the physics system
	for (auto [entity, collision, projectile] : registry.view<Collision, ActiveProjectile>().each()) {
		Entity child_entity = collision.children;
		while (child_entity != entt::null) {
			const Child& child = registry.get<Child>(child_entity);
			Entity entity_other = child.target;
			// TODO: rename hittable container type
			// TODO: Convert all checks to if tile is walkable, currently has issue that enemies can overlay player
			// Currently, arrows can hit anything with a hittable component, which will include walls and enemies
			// Attack the other entity if it can be attacked
			// Checks if the other enemy is a red/blue enemy
			if (registry.all_of<Hittable, Stats, Enemy>(entity_other)) {
				Enemy& enemy = registry.get<Enemy>(entity_other);
				ColorState enemy_color = enemy.team;
				if (enemy_color != turns->get_inactive_color()) {
					combat->do_attack(
						player, registry.get<Weapon>(current_weapon).given_attacks[current_attack], entity_other);
				}
			}
			Entity temp = child_entity;
			child_entity = child.next;
			registry.destroy(temp);
		}
		// Stops projectile motion, adds projectile to list of resolved projectiles
		registry.get<Velocity>(entity).speed = 0;
		registry.remove<ActiveProjectile>(entity);
		registry.emplace<ResolvedProjectile>(entity);
	}
	// Remove all collisions from this simulation step
	registry.clear<Collision>();
}

// Should the game be over?
bool WorldSystem::is_over() const { return bool(glfwWindowShouldClose(window)); }

// Returns arrow to player after firing
void WorldSystem::return_arrow_to_player()
{
	dvec2 mouse_pos = {};
	glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);
	on_mouse_move(vec2(mouse_pos));
}

// On key callback
void WorldSystem::on_key(int key, int /*scancode*/, int action, int mod)
{
	ui->on_key(key, action, mod);
	if (turns && action != GLFW_RELEASE) {

		if (key == GLFW_KEY_D) {
			move_player(Direction::Right);
		}
		if (key == GLFW_KEY_A) {
			move_player(Direction::Left);
		}
		if (key == GLFW_KEY_W) {
			move_player(Direction::Up);
		}
		if (key == GLFW_KEY_S) {
			move_player(Direction::Down);
		}

		// Change weapon
		if (key == GLFW_KEY_E) {
			equip_next_weapon();
		}

		// Change spell
		if (key == GLFW_KEY_K) {
			animations->player_toggle_spell(player_arrow);
		}

		// Change attack
		// TODO: Generalize for many attacks, check out of bounds
		// Key Codes for 1-9
		if (49 <= key && key <= 57) {
			const std::vector<Attack>& attacks = registry.get<Weapon>(current_weapon).given_attacks;
			if (key - 49 < (int) attacks.size()) {
				current_attack = ((size_t) key) - 49;
				const Attack& attack = attacks[current_attack];
				printf("%s Attack: %i-%i attack, %i-%i dmg, %s \n",
					   attack.name.c_str(),
					   attack.to_hit_min,
					   attack.to_hit_max,
					   attack.damage_min,
					   attack.damage_max,
					   attack.targeting_type == TargetingType::Projectile ? "projectile" : "adjacent");
				registry.get<RenderRequest>(player_arrow).visible = attack.targeting_type == TargetingType::Projectile;
				registry.get<Text>(attack_display).text = combat->make_attack_list(current_weapon, current_attack);
			}
		}
	}

	if (action == GLFW_RELEASE && key == GLFW_KEY_SPACE) {
		change_color();
	}

	check_debug_keys(key, action, mod);
}

void WorldSystem::check_debug_keys(int key, int action, int mod)
{

	// Resetting game
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_ALT) != 0 && key == GLFW_KEY_R) {
		// int w, h;
		// glfwGetWindowSize(window, &w, &h);

		restart_game();
	}

	// God mode
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_ALT) != 0 && key == GLFW_KEY_G) {
		Stats& stats = registry.get<Stats>(player);
		stats.evasion = 100000;
		stats.to_hit_bonus = 100000;
		stats.damage_bonus = 100000;
	}

	// Debugging
	if (key == GLFW_KEY_B) {
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

		vec2 mouse_world_position = renderer->screen_position_to_world_position(mouse_position);

		Velocity& arrow_velocity = registry.get<Velocity>(player_arrow);
		WorldPosition& arrow_position = registry.get<WorldPosition>(player_arrow);
		MapPosition& player_map_position = registry.get<MapPosition>(player);

		vec2 player_screen_position = MapUtility::map_position_to_world_position(player_map_position.position);

		// Calculated Euclidean difference between player and arrow
		vec2 eucl_diff = mouse_world_position - player_screen_position;

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

	MapPosition& map_pos = registry.get<MapPosition>(player);
	WorldPosition& arrow_position = registry.get<WorldPosition>(player_arrow);
	uvec2 new_pos = map_pos.position;

	if (map_pos.position == new_pos || !map_generator->walkable_and_free(new_pos)
		|| !turns->execute_team_action(player)) {
		return;
	}

	if (direction == Direction::Left && map_pos.position.x > 0) {
		new_pos = uvec2(map_pos.position.x - 1, map_pos.position.y);

		animations->set_sprite_direction(player, Sprite_Direction::SPRITE_LEFT);
		animations->player_running_animation(player);
	} else if (direction == Direction::Up && map_pos.position.y > 0) {
		new_pos = uvec2(map_pos.position.x, map_pos.position.y - 1);
		animations->player_running_animation(player);
	} else if (direction == Direction::Right
			   && (float)map_pos.position.x < MapUtility::room_size * MapUtility::tile_size - 1) {
		new_pos = uvec2(map_pos.position.x + 1, map_pos.position.y);

		animations->set_sprite_direction(player, Sprite_Direction::SPRITE_RIGHT);
		animations->player_running_animation(player);
	} else if (direction == Direction::Down
			   && (float)map_pos.position.y < MapUtility::room_size * MapUtility::tile_size - 1) {
		new_pos = uvec2(map_pos.position.x, map_pos.position.y + 1);
		animations->player_running_animation(player);
	}

	// Temp update for arrow position
	if (!player_arrow_fired) {
		arrow_position.position += (vec2(new_pos) - vec2(map_pos.position)) * MapUtility::tile_size;
	}

	map_pos.position = new_pos;
	turns->complete_team_action(player);

	if (map_generator->is_next_level_tile(new_pos)) {
		if (map_generator->is_last_level()) {
			end_of_game = true;
			return;
		}

		map_generator->load_next_level();
		registry.get<MapPosition>(player).position = map_generator->get_player_start_position();
	} else if (map_generator->is_last_level_tile(new_pos)) {
		map_generator->load_last_level();
		registry.get<MapPosition>(player).position = map_generator->get_player_end_position();
	}
}

void WorldSystem::equip_next_weapon()
{
	if (turns->get_active_team() != player) {
		return;
	}
	Inventory& inventory = registry.get<Inventory>(player);
	Entity& curr = inventory.equipped[static_cast<uint8>(Slot::PrimaryHand)];
	auto next = inventory.inventory.upper_bound(registry.get<Item>(curr).name);
	if (next == inventory.inventory.end()) {
		next = inventory.inventory.begin();
	}
	while (next->second != curr) {
		if (registry.get<Item>(next->second).allowed_slots[static_cast<uint8>(Slot::PrimaryHand)]) {
			break;
		}
		if (next == inventory.inventory.end()) {
			next = inventory.inventory.begin();
		} else {
			next++;
		}
	};
	if (curr != next->second && turns->execute_team_action(player)) {
		curr = next->second;
		current_weapon = curr;
		current_attack = 0;
		registry.get<RenderRequest>(player_arrow).visible
			= registry.get<Weapon>(current_weapon).given_attacks.at(current_attack).targeting_type
			== TargetingType::Projectile;
		registry.get<Text>(attack_display).text = combat->make_attack_list(current_weapon, current_attack);
		printf("Switched weapon to %s\n", registry.get<Item>(curr).name.c_str());
		animations->player_toggle_weapon(player);
		turns->complete_team_action(player);
	}
}

void WorldSystem::change_color()
{
	if (!turns->ready_to_act(player)) {
		return;
	}
	ColorState active_color = turns->get_active_color();
	switch (active_color) {
	case ColorState::Red:
		turns->set_active_color(ColorState::Blue);
		so_loud->fadeVolume(bgm_blue, -1, .25);
		so_loud->fadeVolume(bgm_red, 0, .25);
		animations->player_red_blue_animation(player, ColorState::Blue);
		break;
	case ColorState::Blue:
		turns->set_active_color(ColorState::Red);
		so_loud->fadeVolume(bgm_red, -1, .25);
		so_loud->fadeVolume(bgm_blue, 0, .25);
		animations->player_red_blue_animation(player, ColorState::Red);
		break;
	default:
		turns->set_active_color(ColorState::Red);
		break;
	}

	// ColorState active_color = turns->get_active_color();
	// for (long long i = registry.enemies.entities.size() - 1; i >= 0; i--) {
	//	const Entity& enemy_entity = registry.enemies.entities[i];
	//	if (((uint8_t)registry.get<Enemy>(enemy_entity).team & (uint8_t)active_color) == 0) {
	//		registry.get<RenderRequest>(enemy_entity).visible = false;
	//	} else {
	//		registry.get<RenderRequest>(enemy_entity).visible = true;
	//	}
	//}
}

// Fires arrow at a preset speed if it has not been fired already
// TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_click(int button, int action, int /*mods*/)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// Get screen position of mouse
		dvec2 mouse_screen_pos = {};
		glfwGetCursorPos(window, &mouse_screen_pos.x, &mouse_screen_pos.y);
		ui->on_left_click(mouse_screen_pos);
		if (turns->get_active_team() != player) {
			return;
		}
		Attack& attack = registry.get<Weapon>(current_weapon).given_attacks[current_attack];
		if (attack.targeting_type == TargetingType::Projectile) {
			try_fire_projectile();
		} else if (attack.targeting_type == TargetingType::Adjacent) {
			try_adjacent_attack(attack);
		}
	}
}

void WorldSystem::on_mouse_scroll(float offset) { this->renderer->scale_on_scroll(offset); }

// resize callback
// TODO: update to scale the scene as not changed when window is resized
void WorldSystem::on_resize(int width, int height) { renderer->on_resize(width, height); }

void WorldSystem::try_fire_projectile()
{
	if (player_arrow_fired || !turns->execute_team_action(player)) {
		return;
	}
	player_arrow_fired = true;
	// Arrow becomes a projectile the moment it leaves the player, not while it's direction is being selected
	ActiveProjectile& arrow_projectile = registry.emplace<ActiveProjectile>(player_arrow, player);
	Velocity& arrow_velocity = registry.get<Velocity>(player_arrow);

	// Denotes arrowhead location the player's arrow, based on firing angle and current scaling
	arrow_projectile.head_offset
		= { sin(arrow_velocity.angle) * scaling_factors.at(static_cast<int>(TEXTURE_ASSET_ID::CANNONBALL)).y / 2,
			-cos(arrow_velocity.angle) * scaling_factors.at(static_cast<int>(TEXTURE_ASSET_ID::CANNONBALL)).x / 2 };

	arrow_velocity.speed = projectile_speed;
	// TODO: Add better arrow physics potentially?
	// arrow_velocity.velocity
	//	= { sin(arrow_motion.angle) * projectile_speed, -cos(arrow_motion.angle) * projectile_speed };
	so_loud->play(cannon_wav);
}

void WorldSystem::try_adjacent_attack(Attack& attack)
{
	if (!turns->ready_to_act(player)) {
		return;
	}
	// Get screen position of mouse
	dvec2 mouse_screen_pos = {};
	glfwGetCursorPos(window, &mouse_screen_pos.x, &mouse_screen_pos.y);

	// Denotes whether a player was able to complete their turn or not (false be default)
	bool combat_success = false;
	// Convert to world pos
	vec2 mouse_world_pos = renderer->screen_position_to_world_position(mouse_screen_pos);

	// Get map_positions to compare
	uvec2 mouse_map_pos = MapUtility::world_position_to_map_position(mouse_world_pos);
	uvec2 player_pos = registry.get<MapPosition>(player).position;
	ivec2 distance = mouse_map_pos - player_pos;
	if (abs(distance.x) > 1 || abs(distance.y) > 1 || distance == ivec2(0, 0) || turns->get_active_team() != player) {
		return;
	}
	for (const auto& target : registry.view<Enemy, Stats>()) {
		if (registry.get<MapPosition>(target).position == mouse_map_pos) {
			Enemy& enemy = registry.get<Enemy>(target);
			ColorState inactive_color = turns->get_inactive_color();
			if (enemy.team == inactive_color || !turns->execute_team_action(player)) {
				continue;
			}

			combat->do_attack(player, attack, target);
			combat_success = true;
			break;
		}
	}
	if (combat_success) {
		so_loud->play(light_sword_wav);
		turns->complete_team_action(player);
	}
}
