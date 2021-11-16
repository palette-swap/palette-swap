#pragma once
#include "../ext/stb_image/stb_image.h"
#include "common.hpp"

#include "map_utility.hpp"

#include <array>
#include <map>
#include <unordered_map>

#include "../ext/stb_image/stb_image.h"
#include "rapidjson/document.h"

#include "map_generator_system.hpp"

// Player component
struct Player {
};

// Camera component
struct Camera {
	uvec2 size, central;
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


// Test Texture Buffer element for enemies
// TODO: change to animated vertices after bringing player into this 3D element group
struct SmallSpriteVertex {
	vec3 position;
	vec2 texcoord;
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
	DUMMY = PALADIN + 1,
	SLIME = DUMMY + 1,
	ARMOR = SLIME + 1,
	TREEANT = ARMOR + 1,
	RAVEN = TREEANT + 1,
	WRAITH = RAVEN + 1,
	DRAKE = WRAITH + 1,
	MUSHROOM = DRAKE + 1,
	SPIDER = MUSHROOM + 1,
	CLONE = SPIDER + 1,
	CANNONBALL = CLONE + 1,
	TILE_SET = CANNONBALL + 1,
	HELP_PIC = TILE_SET + 1,
	END_PIC = HELP_PIC + 1,
	TEXTURE_COUNT = END_PIC + 1,
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

// Define the scaling factors needed for each textures
// Note: This needs to stay the same order as TEXTURE_ASSET_ID and texture_paths
static constexpr std::array<vec2, texture_count> scaling_factors = {
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size * 0.5, MapUtility::tile_size * 0.5),
	vec2(MapUtility::tile_size* MapUtility::room_size, MapUtility::tile_size* MapUtility::room_size),
	vec2(MapUtility::tile_size* MapUtility::room_size * 3, MapUtility::tile_size* MapUtility::room_size * 2),
};

enum class EFFECT_ASSET_ID {
	LINE = 0,
	RECTANGLE = LINE + 1,
	ENEMY = RECTANGLE + 1,
	PLAYER = ENEMY + 1,
	HEALTH = PLAYER + 1,
	FANCY_HEALTH = HEALTH + 1,
	TEXTURED = FANCY_HEALTH + 1,
	WATER = TEXTURED + 1,
	TILE_MAP = WATER + 1,
	EFFECT_COUNT = TILE_MAP + 1
};
constexpr int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID : uint8_t {
	SALMON = 0,
	SPRITE = SALMON + 1,
	SMALL_SPRITE = SPRITE + 1,
	HEALTH = SMALL_SPRITE + 1,
	FANCY_HEALTH = HEALTH + 1,
	LINE = FANCY_HEALTH + 1,
	DEBUG_LINE = LINE + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,
	ROOM = SCREEN_TRIANGLE + 1,
	GEOMETRY_COUNT = ROOM + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;


// Represents allowed directions for an animated sprite (e.g whether the sprite is facing left or right)
enum class Sprite_Direction : uint8_t { SPRITE_LEFT, SPRITE_RIGHT};

// Represents the position on the map,
// top left is (0,0) bottom right is (99,99)
struct MapPosition {
	uvec2 position;
	explicit MapPosition(uvec2 position)
		: position(position)
	{
		assert(position.x <= MapUtility::map_down_right.x && position.y <= MapUtility::map_down_right.y);
	};

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

// Represents the screen position,
// top left is (0,0), bottom right is (1, 1)
struct ScreenPosition {
	vec2 position;
};

// Represents the world position,
// top left is (0,0), bottom right is (window_width_px, window_height_px)
struct WorldPosition {
	vec2 position;
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

enum class ColorState { None = 0, Red = 1, Blue = 2, All = Blue + 1 };

//---------------------------------------------------------------------------
//-------------------------           AI            -------------------------
//---------------------------------------------------------------------------

// Slime (cute coward): weak stats; run away when HP is low.
// Raven (annoying bug): weak stats; large radius and fast speed.
// Armor (Immortal Hulk): normal stats; nearsighted; a certain chance to become immortal for one turn.
// TreeAnt (Super Saiyan): normal stats; long attack range; power up attack range and damage when HP is low.
// Wraith (invisible ghost): weak stats; shortest radius; a variance of Raven but invisible until active.
enum class EnemyType {
	TrainingDummy = 0,
	Slime = TrainingDummy + 1,
	Raven = Slime + 1,
	Armor = Raven + 1,
	TreeAnt = Armor + 1,
	Wraith = TreeAnt + 1,
	Drake = Wraith + 1,
	Mushroom = Drake + 1,
	Spider = Mushroom + 1,
	Clone = Spider + 1,
	EnemyCount = Clone + 1
};

enum class EnemyBehaviour { 
	Basic = 0,
	Cowardly = Basic + 1,
	Defensive = Cowardly + 1,
	Aggressive = Defensive + 1,
	EnemyBehaviourCount = Aggressive + 1,
};

const std::array<const char*, (size_t)EnemyType::EnemyCount> enemy_type_to_string = {
	"TrainingDummy",
	"Slime",
	"Raven",
	"Armor",
	"TreeAnt", 
	"Wraith",
	"Drake",
	"Mushroom",
	"Spider",
	"Clone"
};

// Slime:		Idle, Active, Flinched.
// Raven:		Idle, Actives.
// Armor:		Idle, Active, Immortal.
// TreeAnt:		Idle, Active, Powerup.
// Wraith:		A variance of Raven.
enum class EnemyState {
	Idle = 0,
	Active = Idle + 1,
	Flinched = Active + 1,
	Powerup = Flinched + 1,
	Immortal = Powerup + 1,
	EnemyStateCount = Immortal + 1
};

const std::array<int, (size_t)EnemyState::EnemyStateCount> enemy_state_to_animation_state = {
	0, // Idle
	1, // Active
	2, // Flinched
	2, // Powerup
	2, // Immortal
};

const std::array<EnemyBehaviour, (size_t)EnemyType::EnemyCount> enemy_type_to_behaviour = {
	EnemyBehaviour::Basic, 
	EnemyBehaviour::Cowardly,
	EnemyBehaviour::Basic,
	EnemyBehaviour::Defensive,
	EnemyBehaviour::Aggressive, 
	EnemyBehaviour::Basic,	   
	EnemyBehaviour::Basic, 
	EnemyBehaviour::Cowardly,
	EnemyBehaviour::Aggressive, 
	EnemyBehaviour::Defensive
};

// Structure to store enemy information.
struct Enemy {
	// Default is a slime.
	ColorState team = ColorState::Red;
	EnemyType type = EnemyType::Slime;
	EnemyBehaviour behaviour = EnemyBehaviour::Basic;
	EnemyState state = EnemyState::Idle;
	uvec2 nest_map_pos = { 0, 0 };

	uint radius = 3;
	uint speed = 1;
	uint attack_range = 1;

	void serialize(const std::string & prefix, rapidjson::Document &json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

struct RedExclusive {

};

struct BlueExclusive {

};

struct InactiveEnemy {
};

// Component that denotes what colour the player cannot see at the moment
struct PlayerInactivePerception {
	ColorState inactive = ColorState::Red;
};
//---------------------------------------------------------------------------
//-------------------------		  ANIMATIONS        -------------------------
//---------------------------------------------------------------------------

// Maps enemy types to corresponding texture asset
// Remember to add a mapping to a new texture (or use a default such as a slime)
// This will help load the animation by enemy type when you load enemies
const std::array<TEXTURE_ASSET_ID, static_cast<int>(EnemyType::EnemyCount)> enemy_type_textures {
	TEXTURE_ASSET_ID::DUMMY,
	TEXTURE_ASSET_ID::SLIME,
	TEXTURE_ASSET_ID::RAVEN,
	TEXTURE_ASSET_ID::ARMOR,
	TEXTURE_ASSET_ID::TREEANT,
	TEXTURE_ASSET_ID::WRAITH, 
	TEXTURE_ASSET_ID::DRAKE,
	TEXTURE_ASSET_ID::MUSHROOM,
	TEXTURE_ASSET_ID::SPIDER,
	TEXTURE_ASSET_ID::CLONE,
};

// Render behind other elements in its grouping
struct Background {
	bool visible = false;
};

struct RenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

	bool visible = true;
};

struct Color {
	vec3 color;
};

// Struct for denoting the frame that the rendering system should be rendering
// to the screen for a spritesheet
struct Animation {
	ColorState color = ColorState::None;
	vec4 display_color = { 1, 1, 1, 1 };
	int direction = 1;
	int frame = 0;
	int max_frames = 1;
	int state = 0;
	// Adjusts animation rate to be faster or slower than default
	// ie. faster things should change more frames. slower things should change less frames
	float speed_adjustment = 1;
	float elapsed_time = 0;
};

// Struct denoting irregular animation events (ie attacking, containing information to restore an entity's
// animations after the irregular event animation completes
struct EventAnimation {
	bool turn_trigger = false;
	float speed_adjustment = 1;
	vec4 restore_color = { 1, 1, 1, 1};

	int restore_state = 0;
	float restore_speed = 1;
	int frame = 0;
};

// Denotes that an entity has an textured asset, and should be rendered after regular assets (such as player/enemy)
struct EffectRenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

	bool visible = true;
};

struct Effects {
};
//---------------------------------------------------------------------------
//-------------------------         COMBAT          -------------------------
//---------------------------------------------------------------------------

enum class DamageType {
	Physical = 0,
	Fire = Physical + 1,
	Magical = Fire + 1,
	Count = Magical + 1,
};


enum class TargetingType {
	Adjacent = 0,
	Projectile = 1,
	Count = Projectile + 1,
};

template <typename T> using DamageTypeList = std::array<T, static_cast<size_t>(DamageType::Count)>;

struct Attack {
	std::string name;
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
	TargetingType targeting_type = TargetingType::Projectile;

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

struct Stats {

	// Health and its max
	int health = 140;
	int health_max = 140;

	// Mana and its max
	int mana = 100;
	int mana_max = 100;

	// Each time an attack is made, this flat amount is added to the attack total
	int to_hit_bonus = 10;

	// Each time damage is calculated, this flat amount is added to the damage total
	int damage_bonus = 5;

	// This number is compared to an attack total to see if it hits.
	// It hits if attack_total >= evasion
	int evasion = 16;

	// The default attack associated with this entity
	// TODO: Consider removing when multiple attacks are more readily supported
	Attack base_attack;

	// Each time damage is calculated, the modifier of the relevant type is added
	// A positive modifier is a weakness, like a straw golem being weak to fire
	// A negative modifeir is a resistance, like an iron golem being resistant to sword cuts
	DamageTypeList<int> damage_modifiers = { 0 };

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

enum class Slot {
	Weapon = 0,
	Armor = Weapon + 1,
	Spell1 = Armor + 1,
	Spell2 = Spell1 + 1,
	Ring = Spell2 + 1,
	Amulet = Ring + 1,
	Count = Amulet + 1,
};

const std::array<std::string, (size_t)Slot::Count> slot_names = {
	"Weapon", "Armor", "Spell", "Spell", "Ring", "Amulet",
};

template <typename T> using SlotList = std::array<T, static_cast<size_t>(Slot::Count)>;

struct Inventory {
	static constexpr size_t inventory_size = 12;
	std::array<Entity, inventory_size> inventory;
	SlotList<Entity> equipped;
	Inventory()
		: inventory()
		, equipped()
	{
		inventory.fill(entt::null);
		equipped.fill(entt::null);
	}

	static Entity get(Entity entity, Slot slot);
};

struct Item {
	std::string name;
	float weight = 0.f;
	int value = 0;
	SlotList<bool> allowed_slots = { false };
};

struct Weapon {
	explicit Weapon(std::vector<Attack> given_attacks)
		: given_attacks(std::move(given_attacks))
	{
	}
	// TODO: Potentially replace with intelligent direct/indirect container
	std::vector<Attack> given_attacks;
};

//---------------------------------------------------------------------------
//-------------------------		    Physics         -------------------------
//---------------------------------------------------------------------------

// Struct indicating an object is hittable (Currently limited to projectiles
struct Hittable {
};

// Stucture to store collision information
struct Collision {
	Entity children;
	static void add(Entity parent, Entity child);
};

struct Child {
	Entity parent;
	Entity next;
	Entity target;
};

// struct denoting a currently active projectile
struct ActiveProjectile {
	vec2 head_offset = { 0, 0 };
	float radius = 6;
	Entity shooter;

	explicit ActiveProjectile(const Entity& shooter)
		: shooter(shooter)
	{
	}
};

// Struct for resolving projectiles, including the arrow fired by the player
struct ResolvedProjectile {
	float counter = 150;
};

struct Velocity {
	float speed;
	float angle;
	vec2 get_direction() const { return { sin(angle), -cos(angle) }; }
	vec2 get_velocity() const { return get_direction() * speed; }
};

//---------------------------------------------------------------------------
//-------------------------           UI            -------------------------
//---------------------------------------------------------------------------

enum class Alignment {
	Start = 1,
	Center = 0,
	End = -1,
};

struct UIRectangle {
	float opacity;
	vec4 fill_color;
};

struct UIRenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

	vec2 size;
	float angle = 0;
	Alignment alignment_x;
	Alignment alignment_y;

	UIRenderRequest(TEXTURE_ASSET_ID used_texture,
					EFFECT_ASSET_ID used_effect,
					GEOMETRY_BUFFER_ID used_geometry,
					vec2 size,
					float angle,
					Alignment alignment_x,
					Alignment alignment_y)
		: used_texture(used_texture)
		, used_effect(used_effect)
		, used_geometry(used_geometry)
		, size(size)
		, angle(angle)
		, alignment_x(alignment_x)
		, alignment_y(alignment_y)
	{
	}

	UIRenderRequest(EFFECT_ASSET_ID used_effect, vec2 size, float angle)
		: used_effect(used_effect)
		, size(size)
		, angle(angle)
		, alignment_x(Alignment::Center)
		, alignment_y(Alignment::Center)
	{
		if (used_effect == EFFECT_ASSET_ID::LINE) {
			used_geometry = GEOMETRY_BUFFER_ID::LINE;
		}
	}
};

struct UIElement {
	Entity group;
	Entity next = entt::null;
	bool visible = true;
	UIElement(Entity group, bool visible)
		: group(group)
		, visible(visible)
	{
	}
};

struct UIGroup {
	bool visible = false;
	Entity first_element = entt::null;
	Entity first_text = entt::null;

	static void add_element(Entity group, Entity element, UIElement& ui_element);
	static void add_text(Entity group, Entity text, UIElement& ui_element);
};

struct UIItem {
	Entity actual_item;
};

struct UISlot {
	Entity owner = entt::null;
	Entity contents = entt::null;
};

struct InventorySlot {
	size_t slot;
};

struct EquipSlot {
	Slot slot;
};

struct Draggable {
	Entity container;
};

struct InteractArea {
	vec2 size;
};

struct Line {
	vec2 scale;
	float angle;

	Line(vec2 scale, float angle)
		: scale(scale)
		, angle(angle)
	{
	}
};

struct Text {
	std::string text;
	uint16 font_size;
	Alignment alignment_x;
	Alignment alignment_y;

	Text(std::string text, uint16 font_size, Alignment alignment_x, Alignment alignment_y)
		: text(std::move(text))
		, font_size(font_size)
		, alignment_x(alignment_x)
		, alignment_y(alignment_y)
	{
	}
};

extern bool operator==(const Text& t1, const Text& t2);

template <> struct std::hash<Text> {
	std::size_t operator()(Text const& t) const
	{
		size_t text_hash = std::hash<std::string> {}(t.text);
		size_t size_hash = std::hash<uint16> {}(t.font_size);
		// Combination as per boost https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
		return text_hash ^ (size_hash + 0x9e3779b9 + (text_hash << 6) + (text_hash >> 2));
	}
};
