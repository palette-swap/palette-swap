#pragma once
#include "../ext/stb_image/stb_image.h"
#include "common.hpp"

#include <array>
#include <unordered_map>

#include "../ext/stb_image/stb_image.h"

#include <predefined_room.hpp>

// Player component
struct Player {
};

// Camera component
struct Camera {
	uvec2 size, central;
};

// struct denoting a currently active projectile
struct ActiveProjectile {
	vec2 head_offset = { 0, 0 };
};

// Struct indicating an object is hittable (Currently limited to projectiles
struct Hittable {
};

// Stucture to store collision information
struct Collision {
	// Maintenance Note: make sure initializer list is applied here.
	// Otherwise, id_count in Entity quickly runs out (overflow) during collision checks.

	// Note, the first object is stored in the ECS container.entities
	Entity other; // the second object involved in the collision
	Collision(const Entity& other)
		: other(other)
	{
		this->other = other;
	};
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = false;
	bool in_freeze_mode = false;
};

// Sets the brightness of the screen
struct ScreenState {
	float darken_screen_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent {
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer {
	float counter_ms = 3000;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & salmon.vs.glsl)
struct ColoredVertex {
	vec3 position = { 0, 0, 0 };
	vec3 color = { 0, 0, 0 };
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex {
	vec3 position;
	vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh {
	static bool load_from_obj_file(const std::string& obj_path,
								   std::vector<ColoredVertex>& out_vertices,
								   std::vector<uint16_t>& out_vertex_indices,
								   vec2& out_size);
	vec2 original_size = { 1, 1 };
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

// Struct for resolving projectiles, including the arrow fired by the player
struct ResolvedProjectile {
	float counter = 2000;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID : uint8_t {
	PALADIN = 0,
	SLIME = PALADIN + 1,
	SLIME_ALERT = SLIME + 1,
	SLIME_FLINCHED = SLIME_ALERT + 1,
	ARROW = SLIME_FLINCHED + 1,
	TILE_SET = ARROW + 1,
	TEXTURE_COUNT = TILE_SET + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

// Define the scaling factors needed for each textures
// Note: This needs to stay the same order as TEXTURE_ASSET_ID and texture_paths
static constexpr std::array<vec2, texture_count> scaling_factors = {
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size * 0.5, MapUtility::tile_size * 0.5),
	vec2(MapUtility::tile_size* MapUtility::room_size, MapUtility::tile_size* MapUtility::room_size),
};

enum class EFFECT_ASSET_ID {
	LINE = 0,
	TEXTURED = LINE + 1,
	WATER = TEXTURED + 1,
	TILE_MAP = WATER + 1,
	EFFECT_COUNT = TILE_MAP + 1
};
constexpr int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID : uint8_t {
	SALMON = 0,
	SPRITE = SALMON + 1,
	LINE = SPRITE + 1,
	DEBUG_LINE = LINE + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,

	// Note: Keep ROOM at the bottom because of hacky implementation,
	// this is somewhat hacky, this is actually a single geometry related to a room, but
	// we don't want to update the Enum every time we add a new room. It's MapUtility::num_room - 1
	// because we want to bind vertex buffer for each room but not for the ROOM enum, it's
	// just a placeholder to tell us it's a room geometry, which geometry will be defined
	// by the room struct
	ROOM = SCREEN_TRIANGLE + 1,
	GEOMETRY_COUNT = ROOM + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT + MapUtility::num_room - 1;

struct RenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};

// Represent four directions, that could have many uses, e.g. moving player
enum class Direction : uint8_t {
	Left,
	Up,
	Right,
	Down,
};

// Represents the position on the map,
// top left is (0,0) bottom right is (99,99)
struct MapPosition {
	uvec2 position;
	MapPosition(uvec2 position)
		: position(position)
	{
		assert(position.x < 99 && position.y < 99);
	};
};

// Represents the world position,
// top left is (0,0), bottom right is (window_width_px, window_height_px)
struct WorldPosition {
	vec2 position;
	WorldPosition(vec2 position)
		: position(position)
	{ };
};

struct Velocity {
	float speed;
	float angle;
	Velocity(float speed, float angle)
		: speed(speed), angle(angle) { }
	vec2 get_velocity() { return { sin(angle) * speed, -cos(angle) * speed };
	}
};

struct Room {
	// use 0xff to indicate uninitialized value
	// this can have potential bug if we have up to 255 rooms, but we probably won't...
	MapUtility::RoomType type = 0xff;
};

// For TileMap vertex buffers, we need a separate tile_texture float because we want
// to be able to specify different textures for a room
struct TileMapVertex {
	vec3 position = vec3(0);
	vec2 texcoord = vec3(0);
};

//---------------------------------------------------------------------------
//-------------------------           AI            -------------------------
//---------------------------------------------------------------------------

// Simple 3-state state machine for enemy AI: IDEL, ACTIVE, FLINCHED.
enum class ENEMY_STATE_ID { Idle = 0, ACTIVE = Idle + 1, FLINCHED = ACTIVE + 1 };

// Structure to store enemy state.
struct EnemyState {
	ENEMY_STATE_ID current_state = ENEMY_STATE_ID::Idle;
};

// Structure to store enemy nest position.
struct EnemyNestPosition {
	uvec2 position;
	EnemyNestPosition(const uvec2& position)
		: position(position)
	{
		assert(position.x < MapUtility::map_size * MapUtility::room_size && position.y < MapUtility::map_size * MapUtility::room_size);
	};
};

//---------------------------------------------------------------------------
//-------------------------         COMBAT          -------------------------
//---------------------------------------------------------------------------

enum class DamageType {
	Physical = 0,
	Magical = Physical + 1,
	Count = Magical + 1,
};

template <typename T> using DamageTypeList = std::array<T, static_cast<size_t>(DamageType::Count)>;

struct Attack {
	// Each time an attack is made, a random number is chosen uniformly from [to_hit_min, to_hit_max]
	// This is added to the attack total
	int to_hit_min = 1;
	int to_hit_max = 20;

	// Each time damage is calculated, a random number is chosen uniformly from [damage_min, damage_max]
	// This is added to the damage total
	int damage_min = 10;
	int damage_max = 25;

	// This is used when calculating damage to work out if any of the target's damage_modifiers should apply
	DamageType damage_type = DamageType::Physical;
};

struct Stats {

	// Health and its max
	int health = 100;
	int health_max = 100;

	// Mana and its max
	int mana = 100;
	int mana_max = 100;

	// Each time an attack is made, this flat amount is added to the attack total
	int to_hit_bonus = 10;

	// Each time damage is calculated, this flat amount is added to the damage total
	int damage_bonus = 5;

	// This number is compared to an attack total to see if it hits.
	// It hits if attack_total >= evasion
	int evasion = 15;

	// The default attack associated with this entity
	// TODO: Consider removing when multiple attacks are more readily supported
	Attack base_attack;

	// Each time damage is calculated, the modifier of the relevant type is added
	// A positive modifier is a weakness, like a straw golem being weak to fire
	// A negative modifeir is a resistance, like an iron golem being resistant to sword cuts
	DamageTypeList<int> damage_modifiers = { 0 };
};
