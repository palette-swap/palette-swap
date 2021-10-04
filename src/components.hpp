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

// Turtles and pebbles have a hard shell
struct HardShell {
};

// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0, 0 };
	float angle = 0;
	vec2 velocity = { 0, 0 };
	vec2 scale = { 10, 10 };
};

// Stucture to store collision information
struct Collision {
	// Note, the first object is stored in the ECS container.entities
	Entity other; // the second object involved in the collision
	Collision(Entity& other) { this->other = other; };
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = 0;
	bool in_freeze_mode = 0;
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
	static bool loadFromOBJFile(const std::string& obj_path,
								std::vector<ColoredVertex>& out_vertices,
								std::vector<uint16_t>& out_vertex_indices,
								vec2& out_size);
	vec2 original_size = { 1, 1 };
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
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
	SLUG = PALADIN + 1,
	WALKABLE1 = SLUG + 1,
	WALL1 = WALKABLE1 + 1,
	TEXTURE_COUNT = WALL1 + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

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
	// we don't want to update the Enum every time we add a new room. It's numRoom - 1
	// because we want to bind vertex buffer for each room but not for the ROOM enum, it's
	// just a placeholder to tell us it's a room geometry, which geometry will be defined
	// by the room struct
	ROOM = SCREEN_TRIANGLE + 1,
	GEOMETRY_COUNT = ROOM + 1
};

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

// Represents the position on the world map,
// top left is (0,0) bottom right is (99,99)
// TODO: with this, we probably won't need position from
// Motion, the rendered postion can be calculate from MapPosition
struct MapPosition {
	uvec2 position;
	MapPosition(uvec2 pos) : position(pos) {};
};

/**
* Map-related resources
*/

// RoomType is just a uint8_t
using RoomType = uint8_t;

// Each tile is 32x32 pixels
static constexpr float TILE_SIZE = 32.f;
// Each room is 10x10 tiles
static constexpr int ROOM_SIZE = 10;
// Each map is 10x10 rooms
static constexpr int MAP_SIZE = 10;

struct Room {
	// use 0xff to indicate uninitialized value
	// this can have potential bug if we have up to 255 rooms, but we probably won't...
	RoomType type = 0xff;
};

// Manages and store the generated maps
struct MapGenerator {
private:
	int currentLevel = -1;
public:
	using mapping = std::array<std::array<RoomType, ROOM_SIZE>, ROOM_SIZE>;

	// the (procedural) generated levels, each level contains a full map(max 10*10 rooms)
	std::vector<mapping> levels;

	void generatorLevels();
	const mapping& currentMap();

	// Check if a position on the map is walkable for the player
	bool walkable(uvec2 pos);
};

// TileMapVertex used for vertex buffers, we need a separate tile_texture because we want
// to be able to specify different textures for a room
struct TileMapVertex
{
	vec3 position;
	vec2 texcoord;

	// each tile texture corresponds to a 32*32 png
	// TODO: modify this once we support texture atlas
	float tile_texture = 0;
};

static constexpr int numRoom = 3;
static constexpr std::array<std::array<std::array<RoomType, 10>, 10>, numRoom> roomLayouts = {
    room_left_right_1,
    room_top_down_1,
	room_all_direction_1,
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT + numRoom - 1;

// TODO: This will probably overflow the supported number of textures at some point, replace this once we support
// texture atlas
static constexpr int num_tile_textures = 2;
static constexpr TEXTURE_ASSET_ID tile_textures[2] = { TEXTURE_ASSET_ID::WALKABLE1, TEXTURE_ASSET_ID::WALL1 };

static const std::set<uint8_t> WalkableTiles = { 0 };

// Simple 3-state state machine for enemy AI: IDEL, ACTIVE, FLINCHED.
enum class ENEMY_STATE_ID { IDLE = 0, ACTIVE = IDLE + 1, FLINCHED = ACTIVE + 1 };

// Structure to store enemy state.
struct EnemyState {
	ENEMY_STATE_ID current_state = ENEMY_STATE_ID::IDLE;
};