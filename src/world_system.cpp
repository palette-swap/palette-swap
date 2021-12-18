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
						 std::shared_ptr<AnimationSystem> animations,
						 std::shared_ptr<CombatSystem> combat,
						 std::shared_ptr<LootSystem> loot,
						 std::shared_ptr<MapGeneratorSystem> map,
						 std::shared_ptr<MusicSystem> music,
						 std::shared_ptr<StorySystem> story,
						 std::shared_ptr<TurnSystem> turns,
						 std::shared_ptr<TutorialSystem> tutorials,
						 std::shared_ptr<UISystem> ui,
						 std::shared_ptr<SoLoud::Soloud> so_loud)

	: debugging(debugging)
	, so_loud(std::move(so_loud))
	, rng(std::make_shared<std::default_random_engine>(std::default_random_engine(std::random_device()())))
	, animations(std::move(animations))
	, combat(std::move(combat))
	, loot(std::move(loot))
	, map_generator(std::move(map))
	, music(std::move(music))
	, story(std::move(story))
	, turns(std::move(turns))
	, tutorials(std::move(tutorials))
	, ui(std::move(ui))
{
	this->combat->init(rng, this->animations, this->loot, this->map_generator, this->tutorials);
	this->loot->init(rng, this->tutorials);
	this->loot->on_pickup([this](const Entity& item, size_t slot) { this->ui->add_to_inventory(item, slot); });
	this->combat->on_death([this](const Entity& /*entity*/) { this->ui->update_resource_count(); });
	this->ui->on_show_world([this]() { return_arrow_to_player(); });
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

	light_sword_wav.load(audio_path("sword1.wav").c_str());
	fire_spell_wav.load(audio_path("fireball.wav").c_str());
	ice_spell_wav.load(audio_path("ice.wav").c_str());
	earth_spell_wav.load(audio_path("earth.wav").c_str());
	wind_spell_wav.load(audio_path("wind.wav").c_str());


	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg)
{
	this->renderer = renderer_arg;
	ui->init(
		renderer_arg, loot, tutorials, story, [this]() { try_change_color(); }, [this]() { restart_game(); });
	animations->init(renderer_arg);

	// Set all states to default
	restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{
	// Remove debug info from the last step
	auto debug_view = registry.view<DebugComponent>();
	registry.destroy(debug_view.begin(), debug_view.end());

	// Processing the player state
	assert(registry.size<ScreenState>() <= 1);

	// Player is stunned.
	if (turns->ready_to_act(player) && combat->get_decrement_effect(player, Effect::Stun) > 0) {
		end_player_turn();
	}
	if ((registry.get<Stats>(player).health <= 0) && turns->ready_to_act(player)) {
		ui->end_game(false);
		return true;
	}
	if (end_of_game && turns->ready_to_act(player)) {
		ui->end_game(true);
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
				end_player_turn();
				return_arrow_to_player();
			} else {
				registry.destroy(entity);
			}
		}
	}

	story->step();

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game()
{

	// Debugging for memory/component leaks
	std::cout << "Alive: " << registry.alive() << std::endl;
	printf("Restarting\n");

	// Exit map editing mode
	is_editing_map = false;

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

	// Create a new Player instance and shift player onto a tile
	// player position will be updated as the level load
	player = create_player({ 0, 0 });
	map_generator->load_initial_level();
	uvec2 player_starting_point = registry.get<MapPosition>(player).position;

	// Initializes the perception of the player for what is inactive
	PlayerInactivePerception& inactive_perception = registry.get<PlayerInactivePerception>(player);
	inactive_perception.inactive = turns->get_inactive_color();

	turns->add_team_to_queue(player);
	// create camera instance
	camera = create_camera(player_starting_point);

	// Create a new player arrow instance
	vec2 player_location = MapUtility::map_position_to_world_position(player_starting_point);
	player_arrow = create_arrow(player_location);

	// Restart the LootSystem
	loot->restart_game();

	//Entity boss_entry_entity = animations->create_boss_entry_entity(EnemyType::KingMush, player_starting_point + uvec2(10, 2));
	// Restart the UISystem
	ui->restart_game();

	// Restart the StorySystem
	story->restart_game();

	// Restart the TutorialSystem
	tutorials->restart_game();
	
	turns->set_active_color(ColorState::Red);
	animations->player_red_blue_animation(player, ColorState::Red);
	animations->set_all_inactive_colours(ColorState::Blue);

}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
	// Loop over all collisions detected by the physics system
	for (auto [entity, collision, projectile] : registry.view<Collision, ActiveProjectile>().each()) {
		Entity child_entity = collision.children;
		bool did_attack = false;
		while (child_entity != entt::null) {
			const CollisionEntry& child = registry.get<CollisionEntry>(child_entity);
			Entity entity_other = child.target;
			// TODO: rename hittable container type
			// TODO: Convert all checks to if tile is walkable, currently has issue that enemies can overlay player
			// Currently, arrows can hit anything with a hittable component, which will include walls and enemies
			// Attack the other entity if it can be attacked
			// Checks if the other enemy is a red/blue enemy
			if (registry.valid(entity_other) && registry.all_of<Hittable, Stats, Enemy>(entity_other)) {
				Enemy& enemy = registry.get<Enemy>(entity_other);
				ColorState enemy_color = enemy.team;
				if (!did_attack && enemy_color != turns->get_inactive_color() && ui->has_current_attack()) {
					did_attack = combat->do_attack(
						player, ui->get_current_attack(), registry.get<MapPosition>(entity_other).position);
				}
			}
			Entity temp = child_entity;
			child_entity = child.next;
			registry.destroy(temp);
		}
		if (!did_attack) {
			combat->do_attack(player,
							  ui->get_current_attack(),
							  MapUtility::world_position_to_map_position(registry.get<WorldPosition>(entity).position));
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
	on_mouse_move(mouse_pos);
}

// On key callback
void WorldSystem::on_key(int key, int /*scancode*/, int action, int mod)
{
	// Player is stunned.
	if (turns->ready_to_act(player) && combat->get_decrement_effect(player, Effect::Stun) > 0) {
		end_player_turn();
	}

	if (check_debug_keys(key, action, mod)) {
		return;
	}
	if (turns->ready_to_act(player)) {
		// Get screen position of mouse
		dvec2 mouse_screen_pixels_pos = {};
		glfwGetCursorPos(window, &mouse_screen_pixels_pos.x, &mouse_screen_pixels_pos.y);
		ui->on_key(key, action, mod, renderer->mouse_pos_to_screen_pos(mouse_screen_pixels_pos));
	}
	if (!ui->player_can_act()) {
		return;
	}

	if (story->in_cutscene()) {
		story->on_key(key, action, mod);
		return;
	}
	// Sets player's attack/player arrow to the be the correct state

	if (turns->ready_to_act(player)) {
		if (!ui->has_current_attack()) {
			animations->player_idle_animation(player);
		} else {
			Attack& current_attack = ui->get_current_attack();
			EffectRenderRequest& arrow_render = registry.get<EffectRenderRequest>(player_arrow);

			if (current_attack.mana_cost == 0) {
				animations->player_idle_animation(player);
				arrow_render.visible = false;
			} else {
				animations->player_spellcast_animation(player);
				animations->player_toggle_spell(player_arrow, static_cast<int>(current_attack.damage_type) - 1);
				arrow_render.visible = true;
			}
		}
	}

	if (action != GLFW_RELEASE) {
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
	}

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_SPACE: {
			try_change_color();
			break;
		}
		case GLFW_KEY_LEFT_SHIFT: {
			if (!turns->ready_to_act(player)) break;
			if (loot->try_pickup_items(player)) {
				tutorials->destroy_tooltip(TutorialTooltip::ItemDropped);
				end_player_turn();
			} else if (map_generator->interact_with_surrounding_tile(player)) {
				end_player_turn();
			}
			break;
		}
		case GLFW_KEY_H: {
			if (turns->ready_to_act(player) && combat->try_drink_potion(player)) {
				ui->update_resource_count();
				end_player_turn();
			}
		}
		default:
			break;
		}
	}
}

bool WorldSystem::check_debug_keys(int key, int action, int mod)
{
	// Resetting game
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_ALT) != 0 && key == GLFW_KEY_R) {
		// int w, h;
		// glfwGetWindowSize(window, &w, &h);
		restart_game();
	}

	// Toggle Lighting
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_ALT) != 0 && key == GLFW_KEY_F) {
		renderer->toggle_lighting();
	}

	// God mode
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_ALT) != 0 && key == GLFW_KEY_G) {
		Stats& stats = registry.get<Stats>(player);
		stats.evasion = 100000;
		stats.to_hit_weapons = stats.to_hit_spells = 100000;
		stats.damage_bonus.fill(100000);
		stats.damage_modifiers.fill(-100000);
	}

	// Drop loot on your current location
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_ALT) != 0 && key == GLFW_KEY_L) {
		uvec2 pos = registry.get<MapPosition>(player).position;
		loot->drop_loot(pos, -1.f);
	}

	// Give more resources
	Inventory& inventory = registry.get<Inventory>(player);
	if (key == GLFW_KEY_F1) {
		inventory.resources.at((size_t)Resource::HealthPotion)++;
		ui->update_resource_count();
	}
	if (key == GLFW_KEY_F2) {
		inventory.resources.at((size_t)Resource::ManaPotion)++;
		ui->update_resource_count();
	}
	if (key == GLFW_KEY_F3) {
		inventory.resources.at((size_t)Resource::PaletteSwap)++;
		ui->update_resource_count();
	}

	// Debugging
	if (key == GLFW_KEY_B) {
		debugging.in_debug_mode = action != GLFW_RELEASE;
	}

	// Control the current volume with `<` `>`
	if (action != GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_COMMA) {
		current_volume -= 0.1f;
	}
	if (action != GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_PERIOD) {
		current_volume += 0.1f;
	}
	current_volume = fmax(0.f, current_volume);
	so_loud->setGlobalVolume(current_volume);

	if (is_editing_map) {
		if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_M) {
			is_editing_map = false;
			map_generator->stop_editing_level();
			renderer->set_lighting(true);
			return false;
		}

		if (key == GLFW_KEY_Q && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->increment_seed();
		} else if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->decrement_seed();
		} else if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->increment_path_length();
		} else if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->decrement_path_length();
		} else if (key == GLFW_KEY_Z && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->increase_room_density();
		} else if (key == GLFW_KEY_X && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->decrease_room_density();
		} else if (key == GLFW_KEY_E && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->increase_side_rooms();
		} else if (key == GLFW_KEY_R && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->decrease_side_rooms();
		} else if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->increase_room_path_complexity();
		} else if (key == GLFW_KEY_F && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->decrease_room_path_complexity();
		} else if (key == GLFW_KEY_C && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->increase_room_traps_density();
		} else if (key == GLFW_KEY_V && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			map_generator->decrease_room_traps_density();
		} else if (key == GLFW_KEY_P && (mod & GLFW_MOD_CONTROL) != 0 && action == GLFW_RELEASE) {
			map_generator->save_level_generation_confs();
		} else if (key == GLFW_KEY_N && (action == GLFW_RELEASE)) {
			map_generator->edit_next_level();
		} else if (key == GLFW_KEY_B && (action == GLFW_RELEASE)) {
			map_generator->edit_previous_level();
		} else if (key == GLFW_KEY_T && (action == GLFW_RELEASE)) {
			map_generator->increase_room_smoothness();
		} else if (key == GLFW_KEY_Y && (action == GLFW_RELEASE)) {
			map_generator->decrease_room_smoothness();
		} else if (key == GLFW_KEY_G && (action == GLFW_RELEASE)) {
			map_generator->increase_enemy_density();
		} else if (key == GLFW_KEY_H && (action == GLFW_RELEASE)) {
			map_generator->decrease_enemy_density();
		} else if (key == GLFW_KEY_U && (action == GLFW_RELEASE)) {
			map_generator->increase_level_difficulty();
		} else if (key == GLFW_KEY_I && (action == GLFW_RELEASE)) {
			map_generator->decrease_level_difficulty();
		} 
		return true;
	}
	// for debugging levels
	if (key == GLFW_KEY_N && (mod & GLFW_MOD_CONTROL) != 0 && action == GLFW_RELEASE) {
		if (!map_generator->load_next_level()) {
			return false;
		}
		story->load_next_level();
		return_arrow_to_player();
	} else if (key == GLFW_KEY_B && (mod & GLFW_MOD_CONTROL) != 0 && action == GLFW_RELEASE) {
		if (!map_generator->load_last_level()) {
			return false;
		}
		story->load_last_level();
		return_arrow_to_player();
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_M) {
		is_editing_map = true;
		map_generator->start_editing_level();
		renderer->set_lighting(false);
		return true;
	}
	return false;
}

// Tracks position of cursor, points arrow at potential fire location
// Only enables if an arrow has not already been fired
// TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_move(vec2 mouse_position)
{
	vec2 mouse_screen_pos = renderer->mouse_pos_to_screen_pos(mouse_position);
	if (ui->player_can_act() && !player_arrow_fired) {

		vec2 mouse_world_pos = renderer->screen_position_to_world_position(mouse_screen_pos);

		Velocity& arrow_velocity = registry.get<Velocity>(player_arrow);
		WorldPosition& arrow_position = registry.get<WorldPosition>(player_arrow);
		MapPosition& player_map_position = registry.get<MapPosition>(player);

		vec2 player_screen_position = MapUtility::map_position_to_world_position(player_map_position.position);

		// Calculated Euclidean difference between player and arrow
		vec2 eucl_diff = mouse_world_pos - player_screen_position;

		// Calculates arrow position based on position of mouse relative to player
		vec2 new_arrow_position = normalize(eucl_diff) * spell_distance_from_player + player_screen_position;
		arrow_position.position = new_arrow_position;

		// Calculates arrow angle based on position of mouse
		arrow_velocity.angle = atan2(eucl_diff.x, -eucl_diff.y);
	}

	ui->on_mouse_move(mouse_screen_pos);
}

void WorldSystem::move_player(Direction direction)
{
	if (turns->get_active_team() != player) {
		return;
	}

	// Player is immobilized.
	if (turns->ready_to_act(player) && combat->get_decrement_effect(player, Effect::Immobilize) > 0) {
		end_player_turn();
	}

	MapPosition& map_pos = registry.get<MapPosition>(player);
	uvec2 new_pos = map_pos.position;

	if (direction == Direction::Left && map_pos.position.x > 0) {
		new_pos = uvec2(map_pos.position.x - 1, map_pos.position.y);
		animations->set_sprite_direction(player, Sprite_Direction::SPRITE_LEFT);

	} else if (direction == Direction::Up && map_pos.position.y > 0) {
		new_pos = uvec2(map_pos.position.x, map_pos.position.y - 1);
	} else if (direction == Direction::Right
			   && (float)map_pos.position.x < MapUtility::room_size * MapUtility::tile_size - 1) {
		new_pos = uvec2(map_pos.position.x + 1, map_pos.position.y);

		animations->set_sprite_direction(player, Sprite_Direction::SPRITE_RIGHT);
	} else if (direction == Direction::Down
			   && (float)map_pos.position.y < MapUtility::room_size * MapUtility::tile_size - 1) {
		new_pos = uvec2(map_pos.position.x, map_pos.position.y + 1);
	}

	if (map_pos.position == new_pos || !map_generator->walkable_and_free(player, new_pos)
		|| !turns->execute_team_action(player)) {
		return;
	}

	// Allows player to run if all checks have been passed, inputs running direction as an animation event
	animations->player_running_animation(player, map_pos.position, new_pos);

	MapGeneratorSystem::MoveState move_ret = map_generator->move_player_to_tile(map_pos.position, new_pos);
	if (move_ret == MapGeneratorSystem::MoveState::Failed) {
		return;
	}

	end_player_turn();

	return_arrow_to_player();

	// TODO: move the logics to map generator system
	if (move_ret == MapGeneratorSystem::MoveState::NextLevel || move_ret == MapGeneratorSystem::MoveState::LastLevel) {
		story->load_next_level();
	}
	story->check_cutscene();
}

void WorldSystem::end_player_turn()
{
	if (turns->get_active_team() != player) {
		return;
	}
	combat->apply_decrement_per_turn_effects(player);
	if (turns->ready_to_act(player)) {
		turns->skip_team_action(player);
	} else {
		turns->complete_team_action(player);
	}
}

void WorldSystem::try_change_color()
{
	if (!turns->ready_to_act(player)) {
		return;
	}
	Inventory& inventory = registry.get<Inventory>(player);
	if (inventory.resources.at((size_t)Resource::PaletteSwap) == 0) {
		return;
	}
	MapPosition player_pos = registry.get<MapPosition>(player);

	if (map_generator->walkable_and_free(player, player_pos.position, false)) {
		ColorState inactive_color = turns->get_inactive_color();
		turns->set_active_color(inactive_color);

		inventory.resources.at((size_t)Resource::PaletteSwap)--;
		ui->update_resource_count();

		music->set_world((inactive_color == ColorState::Red) ? MusicSystem::MusicState::RedWorld
																: MusicSystem::MusicState::BlueWorld);

		animations->player_red_blue_animation(player, inactive_color);
	}

	animations->set_all_inactive_colours(turns->get_inactive_color());
}

// Fires arrow at a preset speed if it has not been fired already
// TODO: Integrate into turn state to only enable if player's turn is on
void WorldSystem::on_mouse_click(int button, int action, int /*mods*/)
{
	// Player is stunned.
	if (turns->ready_to_act(player) && combat->get_decrement_effect(player, Effect::Stun) > 0) {
		end_player_turn();
	}


	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		// Get screen position of mouse
		dvec2 mouse_screen_pixels_pos = {};
		glfwGetCursorPos(window, &mouse_screen_pixels_pos.x, &mouse_screen_pixels_pos.y);
		bool used = ui->on_left_click(action, renderer->mouse_pos_to_screen_pos(mouse_screen_pixels_pos));

		if (!used && ui->player_can_act() && action == GLFW_PRESS) {
			if (turns->get_active_team() != player || !ui->has_current_attack()) {
				return;
			}
			Attack& attack = ui->get_current_attack();
			if (attack.targeting_type == TargetingType::Projectile) {
				try_fire_projectile(attack);
			} else if (attack.targeting_type == TargetingType::Adjacent) {
				try_adjacent_attack(attack);
			}
		}
	}
}

void WorldSystem::on_mouse_scroll(float offset)
{
	if (ui->player_can_act()) {
		this->renderer->scale_on_scroll(offset);
		return_arrow_to_player();
	}
}

// resize callback
// TODO: update to scale the scene as not changed when window is resized
void WorldSystem::on_resize(int width, int height) { renderer->on_resize(width, height); }

void WorldSystem::try_fire_projectile(Attack& attack)
{
	if (player_arrow_fired || registry.get<Stats>(player).mana < attack.mana_cost
		|| !turns->execute_team_action(player)) {
		return;
	}
	tutorials->trigger_tooltip(TutorialTooltip::UseResource);
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
	switch (ui->get_current_attack().damage_type)
	{
	case DamageType::Fire :
		so_loud->play(fire_spell_wav);
		break;
	case DamageType::Cold :
		so_loud->play(ice_spell_wav);
		break;
	case DamageType::Earth :
		so_loud->play(earth_spell_wav);
		break;
	case DamageType::Wind :
		so_loud->play(wind_spell_wav);
		break;
	default:
		break;
	}

	animations->player_specific_spell(player, ui->get_current_attack().damage_type);
}

void WorldSystem::try_adjacent_attack(Attack& attack)
{
	if (!turns->ready_to_act(player)) {
		return;
	}
	// Get position of mouse
	dvec2 mouse_pos = {};
	glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);

	// Convert to world pos
	vec2 mouse_world_pos = renderer->screen_position_to_world_position(renderer->mouse_pos_to_screen_pos(mouse_pos));

	// Get map_positions to compare
	uvec2 mouse_map_pos = MapUtility::world_position_to_map_position(mouse_world_pos);
	if (!combat->is_valid_attack(player, attack, mouse_map_pos) || !turns->execute_team_action(player)) {
		return;
	}
	if (combat->do_attack(player, attack, mouse_map_pos)) {
		so_loud->play(light_sword_wav);
	}
	end_player_turn();
}
